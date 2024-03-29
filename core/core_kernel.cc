/* Copyright 2020 Stanford University
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cassert>
#include <cmath>
#include <thread>

#if (__AVX2__ == 1) || (__AVX__ == 1)
#include <immintrin.h>
#endif

#include <string.h>

#include "core.h"
#include "core_kernel.h"
#include "core_random.h"

#define STARPU_USE_CUDA

#ifdef STARPU_USE_CUDA
#include <cublas_v2.h>
#include <cuda.h>
#include <cuda_runtime.h>
static cublasHandle_t handle;
// #include <starpu_cublas_v2.h>
#endif

#ifdef USE_BLAS_KERNEL
#include <mkl.h>
#endif

void init_cublas() {
  cublasCreate(&handle);
}

void execute_kernel_empty(const Kernel &kernel)
{
  // Do nothing...
}

long long execute_kernel_busy_wait(const Kernel &kernel)
{
  long long acc = 113;
  for (long iter = 0; iter < kernel.iterations; iter++) {
    acc = acc * 139 % 2147483647;
  }
  return acc;
}

void copy(char *scratch_ptr, size_t scratch_bytes)
{
  assert(scratch_bytes % 2 == 0);

  char *base_ptr = reinterpret_cast<char *>(scratch_ptr);
  intptr_t base_ptr_addr = reinterpret_cast<intptr_t>(base_ptr);
  size_t prolog_padding = 0;
  char *aligned_ptr = base_ptr;
  char *prolog_ptr = base_ptr;
  char *epilog_ptr = base_ptr;
  int i;
  
  assert(base_ptr_addr % 2 == 0);
  
  // compute prolog
  if (base_ptr_addr % 32 != 0) {
    prolog_padding = 32 - (base_ptr_addr % 32);
    aligned_ptr = base_ptr + prolog_padding;
  }
  
  // 32-byte alignment, 64 is because it will be divided into src and dst
  size_t nbytes_aligned = (scratch_bytes - prolog_padding) / 64 * 64;
  
  // epilog
  size_t epilog_padding = scratch_bytes - prolog_padding - nbytes_aligned;
  epilog_ptr = aligned_ptr + nbytes_aligned;
  
  // calculate src and dst for each part
  char *prolog_src = prolog_ptr;
  char *prolog_dst = prolog_ptr + prolog_padding / 2;
  size_t prolog_cp_size = prolog_padding / 2;
  
  size_t nb_m256 = nbytes_aligned / 64;
  
  char *epilog_src = epilog_ptr;
  char *epilog_dst = epilog_ptr + epilog_padding / 2;
  size_t epilog_cp_size = epilog_padding / 2;
  
  assert (prolog_padding + nbytes_aligned + epilog_padding == scratch_bytes);
  
  // copy src to dst
  if (prolog_padding != 0) {
    memcpy(prolog_dst, prolog_src, prolog_cp_size);
  }
  char *aligned_src = aligned_ptr;
  char *aligned_dst = aligned_ptr + nbytes_aligned / 2;
  assert ((intptr_t)aligned_src % 32 == 0);
  assert ((intptr_t)aligned_dst % 32 == 0);
#if (__AVX2__ == 1) || (__AVX__ == 1)
  for (i = 0; i < nb_m256; i++) {
    __m256d *dst_m256 = reinterpret_cast<__m256d *>(aligned_dst);
    *dst_m256 = _mm256_load_pd(reinterpret_cast<double *>(aligned_src));
    aligned_src += 32;
    aligned_dst += 32;
  }
#else
  memcpy(aligned_dst, aligned_src, nbytes_aligned/2);
#endif
  if (epilog_padding != 0) {
    memcpy(epilog_dst, epilog_src, epilog_cp_size);
  }
}

void execute_kernel_memory(const Kernel &kernel,
                           char *scratch_ptr, size_t scratch_bytes,
                           long timestep)
{
#if 1
  long iter = 0;

  size_t sample_bytes = scratch_bytes / kernel.samples;

  // Prologue
  {
    long start_idx = (timestep * kernel.iterations + iter) % kernel.samples;
    long stop_idx = std::min((long)kernel.samples, start_idx + kernel.iterations);
    long num_iter = stop_idx - start_idx;

    if (num_iter > 0) {
      char *sample_ptr = scratch_ptr + start_idx * sample_bytes;

      copy(sample_ptr, num_iter * sample_bytes);

      iter += num_iter;
    }
  }

  // Body
  for ( ; iter + kernel.samples <= kernel.iterations; iter += kernel.samples) {
    long start_idx = (timestep * kernel.iterations + iter) % kernel.samples;
    long num_iter = kernel.samples;

    char *sample_ptr = scratch_ptr + start_idx * sample_bytes;

    copy(sample_ptr, num_iter * sample_bytes);
  }

  // Epilogue
  {
    long start_idx = (timestep * kernel.iterations + iter) % kernel.samples;
    long stop_idx = start_idx + (kernel.iterations - iter);
    long num_iter = stop_idx - start_idx;

    if (num_iter > 0) {
      char *sample_ptr = scratch_ptr + start_idx * sample_bytes;

      copy(sample_ptr, num_iter * sample_bytes);

      iter += num_iter;
    }
  }

  assert(iter == kernel.iterations);
#else
  long long N = scratch_bytes / 2;
  char *src = reinterpret_cast<char *>(scratch_ptr);
  char *dst = reinterpret_cast<char *>(scratch_ptr + N);
  for (long iter = 0; iter < kernel.iterations; iter++) {
    memcpy(dst, src, N);
  }
#endif
}

void execute_kernel_dgemm(const Kernel &kernel,
                           char *scratch_ptr, size_t scratch_bytes)
{
#ifdef USE_BLAS_KERNEL
  long long N = scratch_bytes / (3 * sizeof(double));
  int m, n, p;
  double alpha, beta;

  m = n = p = sqrt(N);
  alpha = 1.0; beta = 1.0;

  double *A = reinterpret_cast<double *>(scratch_ptr);
  double *B = reinterpret_cast<double *>(scratch_ptr + N * sizeof(double));
  double *C = reinterpret_cast<double *>(scratch_ptr + 2 * N * sizeof(double));

  for (long iter = 0; iter < kernel.iterations; iter++) {
    cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, 
                m, n, p, alpha, A, p, B, n, beta, C, n);
  }
#else
  fprintf(stderr, "STARPU_USE_CUDA is not enabled\n");
  fflush(stderr);
  abort();
#endif
}

void checkCublasStatus(cublasStatus_t status) {
    switch (status) {
        case CUBLAS_STATUS_SUCCESS:
            printf("CUBLAS_STATUS_SUCCESS\n");
            break;
        case CUBLAS_STATUS_NOT_INITIALIZED:
            printf("CUBLAS_STATUS_NOT_INITIALIZED\n");
            break;
        case CUBLAS_STATUS_ALLOC_FAILED:
            printf("CUBLAS_STATUS_ALLOC_FAILED\n");
            break;
        case CUBLAS_STATUS_INVALID_VALUE:
            printf("CUBLAS_STATUS_INVALID_VALUE\n");
            break;
        case CUBLAS_STATUS_ARCH_MISMATCH:
            printf("CUBLAS_STATUS_ARCH_MISMATCH\n");
            break;
        case CUBLAS_STATUS_MAPPING_ERROR:
            printf("CUBLAS_STATUS_MAPPING_ERROR\n");
            break;
        case CUBLAS_STATUS_EXECUTION_FAILED:
            printf("CUBLAS_STATUS_EXECUTION_FAILED\n");
            break;
        case CUBLAS_STATUS_INTERNAL_ERROR:
            printf("CUBLAS_STATUS_INTERNAL_ERROR\n");
            break;
        case CUBLAS_STATUS_NOT_SUPPORTED:
            printf("CUBLAS_STATUS_NOT_SUPPORTED\n");
            break;
        case CUBLAS_STATUS_LICENSE_ERROR:
            printf("CUBLAS_STATUS_LICENSE_ERROR\n");
            break;
        default:
            printf("Unknown CUBLAS status\n");
    }
}

void execute_kernel_dgemm_cuda(const GPUKernel &kernel,
                           char *scratch_ptr, size_t scratch_bytes, cublasHandle_t inhandle)
{
#ifdef STARPU_USE_CUDA
  long long N = scratch_bytes / (3 * sizeof(double));
  int m, n, p;
  double alpha, beta;

  m = n = p = sqrt(N);
  alpha = 1.0; beta = 1.0;
  float* d_ptr; // 设备指针
  cudaError_t err = cudaMalloc((void**)&d_ptr, scratch_bytes);
  if (err != cudaSuccess) {
      printf("Error allocating GPU memory: %s\n", cudaGetErrorString(err));
      return;
  }

  // 将数据从CPU复制到GPU
  err = cudaMemcpy(d_ptr, scratch_ptr, 3 * N * sizeof(double), cudaMemcpyHostToDevice);
  if (err != cudaSuccess) {
      printf("Error copying data to GPU: %s\n", cudaGetErrorString(err));
      cudaFree(d_ptr); // 释放之前分配的GPU内存
      return;
  }

  double *A = reinterpret_cast<double *>(d_ptr);
  double *B = reinterpret_cast<double *>(d_ptr + N * sizeof(double));
  double *C = reinterpret_cast<double *>(d_ptr + 2 * N * sizeof(double));
  cublasStatus_t status;
  // A[0] = 1.0;
  for (long iter = 0; iter < kernel.iterations; iter++) {
    status = cublasDgemm(inhandle, CUBLAS_OP_N, CUBLAS_OP_N, 
                m, n, p, &alpha, A, p, B, n, &beta, C, n);
    // printf("after call cublasDgemm %d\n", iter);
    // checkCublasStatus(status);
    // assert(status == CUBLAS_STATUS_SUCCESS);
  }
  cudaFree(d_ptr);
#else
  fprintf(stderr, "STARPU_USE_CUDA is not enabled\n");
  fflush(stderr);
  abort();
#endif
}

void execute_kernel_daxpy(const Kernel &kernel,
                          char *scratch_large_ptr, size_t scratch_large_bytes,
                          long timestep)
{
#ifdef USE_BLAS_KERNEL
  for (long iter = 0; iter < kernel.iterations; iter++) {  
    size_t scratch_bytes = scratch_large_bytes / kernel.samples;
    int idx = (timestep * kernel.iterations + iter) % kernel.samples;
    char *scratch_ptr = scratch_large_ptr + idx * scratch_bytes;
  
    int N = scratch_bytes / (2 * sizeof(double));
    double alpha;

    alpha = 1.0;

    double *X = reinterpret_cast<double *>(scratch_ptr);
    double *Y = reinterpret_cast<double *>(scratch_ptr + N * sizeof(double));

    cblas_daxpy(N, alpha, X, 1, Y, 1);
  }
#else
  fprintf(stderr, "No BLAS is detected\n");
  fflush(stderr);
  abort();
#endif
}

void execute_kernel_daxpy_cuda(const GPUKernel &kernel,
                          char *scratch_large_ptr, size_t scratch_large_bytes,
                          long timestep)
{
#ifdef STARPU_USE_CUDA
  for (long iter = 0; iter < kernel.iterations; iter++) {  
    size_t scratch_bytes = scratch_large_bytes / kernel.samples;
    int idx = (timestep * kernel.iterations + iter) % kernel.samples;
    char *scratch_ptr = scratch_large_ptr + idx * scratch_bytes;
  
    int N = scratch_bytes / (2 * sizeof(double));
    double alpha;

    alpha = 1.0;
    float* d_ptr; // 设备指针
    cudaError_t err = cudaMalloc((void**)&d_ptr, scratch_bytes); 
    if (err != cudaSuccess) {
        printf("Error allocating GPU memory: %s\n", cudaGetErrorString(err));
        return;
    }

    // 将数据从CPU复制到GPU
    err = cudaMemcpy(d_ptr, scratch_ptr, 2 * N * sizeof(double), cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        printf("Error copying data to GPU: %s\n", cudaGetErrorString(err));
        cudaFree(d_ptr); // 释放之前分配的GPU内存
        return;
    }

    double *X = reinterpret_cast<double *>(d_ptr);
    double *Y = reinterpret_cast<double *>(d_ptr + N * sizeof(double));
    
    cublasDaxpy(handle, N, &alpha, X, 1, Y, 1);
    cudaFree(d_ptr);
  }
#else
  fprintf(stderr, "No BLAS is detected\n");
  fflush(stderr);
  abort();
#endif
}

double execute_kernel_compute(const Kernel &kernel)
{
  double A[64];
  double D[64];
  int n = 64;
  for (int i = 0; i < 64; i++) {
    A[i] = 1.2345;
  }
  // float d = 0.0;
  // for (long iter = 0; iter < kernel.iterations; iter++) {
  //   cublasDdot(handle, n, A, 1, A, 1, D);
  //   float alpha = 1.0;
  //   cublasDaxpy(handle, n, &alpha, A, 1, D, 1);
  // }
  
  for (long iter = 0; iter < kernel.iterations; iter++) {
    for (int i = 0; i < 64; i++) {
        A[i] = A[i] * A[i] + A[i];
    }
  } 
  double *C = (double *)A;
  double dot = 1.0;
  for (int i = 0; i < 64; i++) {
    dot *= C[i];
  }
  return dot;  
}



double execute_kernel_compute_cuda(const GPUKernel &kernel) {

  return 0.0f;
}

double execute_kernel_compute2(const Kernel &kernel)
{
  constexpr size_t N = 32;
  double A[N] = {0};
  double B[N] = {0};
  double C[N] = {0};

  for (size_t i = 0; i < N; ++i) {
    A[i] = 1.2345;
    B[i] = 1.010101;
  }

  for (long iter = 0; iter < kernel.iterations; iter++) {
    for (size_t i = 0; i < N; ++i) {
      C[i] = C[i] + (A[i] * B[i]);
    }
  }

  double sum = 0;
  for (size_t i = 0; i < N; ++i) {
    sum += C[i];
  }
  return sum;
}

void execute_kernel_io(const Kernel &kernel)
{
  assert(false);
}

long select_imbalance_iterations(const Kernel &kernel,
                                 long graph_index, long timestep, long point)
{
  long seed[3] = {graph_index, timestep, point};
  double value = random_uniform(&seed[0], sizeof(seed));

  long iterations = (long)round((1 + (value - 0.5)*kernel.imbalance) * kernel.iterations);
  assert(iterations >= 0);
  return iterations;
}

double execute_kernel_imbalance(const Kernel &kernel,
                                long graph_index, long timestep, long point)
{
  long iterations = select_imbalance_iterations(kernel, graph_index, timestep, point);
  Kernel k(kernel);
  k.iterations = iterations;
  // printf("iteration %ld\n", iterations);
  return execute_kernel_compute(k);
}

void execute_kernel_customize(const Kernel &kernel, double expect_runtime)
{
  // Need to sleep for the expected time, use milliseconds
  std::this_thread::sleep_for(std::chrono::microseconds((long)(expect_runtime * 1000)));
}

void execute_kernel_customize_cuda(const GPUKernel &kernel, double expect_runtime)
{
  // Need to sleep for the expected time, use milliseconds
  std::this_thread::sleep_for(std::chrono::microseconds((long)(expect_runtime * 1000)));
}

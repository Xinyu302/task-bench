/* Copyright 2020 Los Alamos National Laboratory
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <iostream>
#include <mpi.h>
#include <math.h>
#include <array>
#include <starpu_mpi.h>
#include <starpu_profiling.h>
#include <starpu_cublas_v2.h>
#include "data.h"
#include "core.h"
#include "timer.h"

#include <unistd.h>

#define DEBUG 0

#define VERBOSE_LEVEL 0

#define USE_CORE_VERIFICATION
#define ENABLE_PRUNE_MPI_TASK_INSERT

char **extra_local_memory;

typedef struct payload_s {
  int graph_id;
  int i;
  int j;
  const TaskGraph *graph;
}payload_t;

static void init_extra_local_memory(void *arg)
{
  size_t max_scratch_bytes_per_task = (uintptr_t) arg;
  int tid = starpu_worker_get_id();

  extra_local_memory[tid] = (char*)malloc(sizeof(char)*max_scratch_bytes_per_task);
  TaskGraph::prepare_scratch(extra_local_memory[tid], sizeof(char)*max_scratch_bytes_per_task);
}


static void task1(void *descr[], void *cl_arg, int starpu_cuda)
{
  float *out;
  payload_t payload;
  out = (float *)STARPU_MATRIX_GET_PTR(descr[0]);
  starpu_codelet_unpack_args(cl_arg, &payload);
  
  int tid = starpu_worker_get_id();
  
#if defined (USE_CORE_VERIFICATION) 
  const TaskGraph *graph = payload.graph;
  char *output_ptr = (char*)out;
  size_t output_bytes= graph->output_bytes_per_task;
  const char *input_data[] = {
    (const char*) out,
  };
  size_t input_bytes[] = {
    output_bytes,
  };
  cublasHandle_t handle = starpu_cublas_get_local_handle(); 
  graph->execute_point_common(starpu_cuda, payload.i, payload.j, output_ptr, output_bytes,
                       input_data, input_bytes, 1, extra_local_memory[tid], graph->scratch_bytes_per_task, handle);
#else
  int rank;
  starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);

  *out = 0.0;
  printf("Graph %d, Task1, [%d, %d], rank %d, core %d, out %.2f, local_mem %p\n", payload.graph_id, payload.i, payload.j, rank, tid, *out, extra_local_memory[tid]);
#endif
}

static void task2(void *descr[], void *cl_arg, int starpu_cuda)
{
  float *in1, *out;
  payload_t payload;
  in1 = (float *)STARPU_MATRIX_GET_PTR(descr[0]);
  out = (float *)STARPU_MATRIX_GET_PTR(descr[1]);
  starpu_codelet_unpack_args(cl_arg, &payload);
  
  int tid = starpu_worker_get_id();
  
#if defined (USE_CORE_VERIFICATION)   
  const TaskGraph *graph = payload.graph;
  char *output_ptr = (char*)out;
  size_t output_bytes= graph->output_bytes_per_task;
  const char *input_data[] = {
    (const char*) in1,
  };
  size_t input_bytes[] = {
    output_bytes,
  };
  cublasHandle_t handle = starpu_cublas_get_local_handle(); 
  graph->execute_point_common(starpu_cuda, payload.i, payload.j, output_ptr, output_bytes,
                       input_data, input_bytes, 2, extra_local_memory[tid], graph->scratch_bytes_per_task, handle);
#else  
  int rank;
  starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);
  
  *out = *in1 + 1.0;
  printf("Graph %d, Task2, [%d, %d], rank %d, core %d, in1 %.2f, out %.2f, local_mem %p\n", payload.graph_id, payload.i, payload.j, rank, tid, *in1, *out, extra_local_memory[tid]);
#endif
}

static void task3(void *descr[], void *cl_arg, int starpu_cuda)
{
  float *in1, *in2, *out;
  payload_t payload;
  in1 = (float *)STARPU_MATRIX_GET_PTR(descr[0]);
  in2 = (float *)STARPU_MATRIX_GET_PTR(descr[1]);
  out = (float *)STARPU_MATRIX_GET_PTR(descr[2]);
  starpu_codelet_unpack_args(cl_arg, &payload);
  
  int tid = starpu_worker_get_id();

#if defined (USE_CORE_VERIFICATION)  
  const TaskGraph *graph = payload.graph;
  char *output_ptr = (char*)out;
  size_t output_bytes= graph->output_bytes_per_task;
  const char *input_data[] = {
    (const char*) in1,
    (const char*) in2,
  };
  size_t input_bytes[] = {
    output_bytes,
    output_bytes,
  };
  cublasHandle_t handle = starpu_cublas_get_local_handle();
  graph->execute_point_common(starpu_cuda, payload.i, payload.j, output_ptr, output_bytes,
                       input_data, input_bytes, 2, extra_local_memory[tid], graph->scratch_bytes_per_task, handle);
#else
  int rank;
  starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);

  *out = *in1 + *in2 + 1.0;
  printf("Graph %d, Task3, [%d, %d], rank %d, core %d, in1 %.2f, in2 %.2f, out %.2f, local_mem %p\n", payload.graph_id, payload.i, payload.j, rank, tid, *in1, *in2, *out, extra_local_memory[tid]);
#endif
}

static void task4(void *descr[], void *cl_arg, int starpu_cuda)
{
  float *in1, *in2, *in3, *out;
  payload_t payload;
  in1 = (float *)STARPU_MATRIX_GET_PTR(descr[0]);
  in2 = (float *)STARPU_MATRIX_GET_PTR(descr[1]);
  in3 = (float *)STARPU_MATRIX_GET_PTR(descr[2]);
  out = (float *)STARPU_MATRIX_GET_PTR(descr[3]);
  starpu_codelet_unpack_args(cl_arg, &payload);
  
  int tid = starpu_worker_get_id();

#if defined (USE_CORE_VERIFICATION)  
  const TaskGraph *graph = payload.graph;
  char *output_ptr = (char*)out;
  size_t output_bytes= graph->output_bytes_per_task;
  const char *input_data[] = {
    (const char*) in1,
    (const char*) in2,
    (const char*) in3,
  };
  size_t input_bytes[] = {
    output_bytes,
    output_bytes,
    output_bytes,
  };
  cublasHandle_t handle = starpu_cublas_get_local_handle();
  graph->execute_point_common(starpu_cuda, payload.i, payload.j, output_ptr, output_bytes,
                       input_data, input_bytes, 3, extra_local_memory[tid], graph->scratch_bytes_per_task, handle);

#else
  int rank;
  starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);

  *out = *in1 + *in2 + *in3 + 1.0;
  printf("Graph %d, Task4, [%d, %d], rank %d, core %d, in1 %.2f, in2 %.2f, in3 %.2f, out %.2f, local_mem %p\n", payload.graph_id, payload.i, payload.j, rank, tid, *in1, *in2, *in3, *out, extra_local_memory[tid]);
#endif
}

static void task5(void *descr[], void *cl_arg, int starpu_cuda)
{
  float *in1, *in2, *in3, *in4, *out;
  payload_t payload;
  in1 = (float *)STARPU_MATRIX_GET_PTR(descr[0]);
  in2 = (float *)STARPU_MATRIX_GET_PTR(descr[1]);
  in3 = (float *)STARPU_MATRIX_GET_PTR(descr[2]);
  in4 = (float *)STARPU_MATRIX_GET_PTR(descr[3]);
  out = (float *)STARPU_MATRIX_GET_PTR(descr[4]);
  starpu_codelet_unpack_args(cl_arg, &payload);
  
  int tid = starpu_worker_get_id();

#if defined (USE_CORE_VERIFICATION)  
  const TaskGraph *graph = payload.graph;
  char *output_ptr = (char*)out;
  size_t output_bytes= graph->output_bytes_per_task;
  const char *input_data[] = {
    (const char*) in1,
    (const char*) in2,
    (const char*) in3,
    (const char*) in4,
  };
  size_t input_bytes[] = {
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
  };
  cublasHandle_t handle = starpu_cublas_get_local_handle();
  graph->execute_point_common(starpu_cuda, payload.i, payload.j, output_ptr, output_bytes,
                       input_data, input_bytes, 4, extra_local_memory[tid], graph->scratch_bytes_per_task, handle);

#else
  int rank;
  starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);

  *out = *in1 + *in2 + *in3 + *in4 + 1.0;
  printf("Graph %d, Task5, [%d, %d], rank %d, core %d, in1 %.2f, in2 %.2f, in3 %.2f, in4 %.2f, out %.2f, local_mem %p\n", payload.graph_id, payload.i, payload.j, rank, tid, *in1, *in2, *in3, *in4, *out, extra_local_memory[tid]);
#endif
}

static void task6(void *descr[], void *cl_arg, int starpu_cuda)
{
  float *in1, *in2, *in3, *in4, *in5, *out;
  payload_t payload;
  in1 = (float *)STARPU_MATRIX_GET_PTR(descr[0]);
  in2 = (float *)STARPU_MATRIX_GET_PTR(descr[1]);
  in3 = (float *)STARPU_MATRIX_GET_PTR(descr[2]);
  in4 = (float *)STARPU_MATRIX_GET_PTR(descr[3]);
  in5 = (float *)STARPU_MATRIX_GET_PTR(descr[4]);
  out = (float *)STARPU_MATRIX_GET_PTR(descr[5]);
  starpu_codelet_unpack_args(cl_arg, &payload);
  
  int tid = starpu_worker_get_id();

#if defined (USE_CORE_VERIFICATION)  
  const TaskGraph *graph = payload.graph;
  char *output_ptr = (char*)out;
  size_t output_bytes= graph->output_bytes_per_task;
  const char *input_data[] = {
    (const char*) in1,
    (const char*) in2,
    (const char*) in3,
    (const char*) in4,
    (const char*) in5,
  };
  size_t input_bytes[] = {
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
  };
  cublasHandle_t handle = starpu_cublas_get_local_handle();
  graph->execute_point_common(starpu_cuda, payload.i, payload.j, output_ptr, output_bytes,
                       input_data, input_bytes, 5, extra_local_memory[tid], graph->scratch_bytes_per_task, handle);

#else
  int rank;
  starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);

  *out = *in1 + *in2 + *in3 + *in4 + *in5 + 1.0;
  printf("Graph %d, Task6, [%d, %d], rank %d, core %d, in1 %.2f, in2 %.2f, in3 %.2f, in4 %.2f, in5 %.2f, out %.2f, local_mem %p\n", payload.graph_id, payload.i, payload.j, rank, tid, *in1, *in2, *in3, *in4, *in5, *out, extra_local_memory[tid]);
#endif
}

static void task7(void *descr[], void *cl_arg, int starpu_cuda)
{
  float *in1, *in2, *in3, *in4, *in5, *in6, *out;
  payload_t payload;
  in1 = (float *)STARPU_MATRIX_GET_PTR(descr[0]);
  in2 = (float *)STARPU_MATRIX_GET_PTR(descr[1]);
  in3 = (float *)STARPU_MATRIX_GET_PTR(descr[2]);
  in4 = (float *)STARPU_MATRIX_GET_PTR(descr[3]);
  in5 = (float *)STARPU_MATRIX_GET_PTR(descr[4]);
  in6 = (float *)STARPU_MATRIX_GET_PTR(descr[5]);
  out = (float *)STARPU_MATRIX_GET_PTR(descr[6]);
  starpu_codelet_unpack_args(cl_arg, &payload);
  
  int tid = starpu_worker_get_id();

#if defined (USE_CORE_VERIFICATION)  
  const TaskGraph *graph = payload.graph;
  char *output_ptr = (char*)out;
  size_t output_bytes= graph->output_bytes_per_task;
  const char *input_data[] = {
    (const char*) in1,
    (const char*) in2,
    (const char*) in3,
    (const char*) in4,
    (const char*) in5,
    (const char*) in6,
  };
  size_t input_bytes[] = {
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
  };
  cublasHandle_t handle = starpu_cublas_get_local_handle();
  graph->execute_point_common(starpu_cuda, payload.i, payload.j, output_ptr, output_bytes,
                       input_data, input_bytes, 6, extra_local_memory[tid], graph->scratch_bytes_per_task, handle);

#else
  int rank;
  starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);

  *out = *in1 + *in2 + *in3 + *in4 + *in5 + *in6 + 1.0;
  printf("Graph %d, Task6, [%d, %d], rank %d, core %d, in1 %.2f, in2 %.2f, in3 %.2f, in4 %.2f, in5 %.2f, in6 %.2f, out %.2f, local_mem %p\n", 
    payload.graph_id, payload.i, payload.j, rank, tid, *in1, *in2, *in3, *in4, *in5, *in6, *out, extra_local_memory[tid]);
#endif
}

static void task8(void *descr[], void *cl_arg, int starpu_cuda)
{
  float *in1, *in2, *in3, *in4, *in5, *in6, *in7, *out;
  payload_t payload;
  in1 = (float *)STARPU_MATRIX_GET_PTR(descr[0]);
  in2 = (float *)STARPU_MATRIX_GET_PTR(descr[1]);
  in3 = (float *)STARPU_MATRIX_GET_PTR(descr[2]);
  in4 = (float *)STARPU_MATRIX_GET_PTR(descr[3]);
  in5 = (float *)STARPU_MATRIX_GET_PTR(descr[4]);
  in6 = (float *)STARPU_MATRIX_GET_PTR(descr[5]);
  in7 = (float *)STARPU_MATRIX_GET_PTR(descr[6]);
  out = (float *)STARPU_MATRIX_GET_PTR(descr[7]);
  starpu_codelet_unpack_args(cl_arg, &payload);
  
  int tid = starpu_worker_get_id();

#if defined (USE_CORE_VERIFICATION)  
  const TaskGraph *graph = payload.graph;
  char *output_ptr = (char*)out;
  size_t output_bytes= graph->output_bytes_per_task;
  const char *input_data[] = {
    (const char*) in1,
    (const char*) in2,
    (const char*) in3,
    (const char*) in4,
    (const char*) in5,
    (const char*) in6,
    (const char*) in7,
  };
  size_t input_bytes[] = {
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
  };
  
  cublasHandle_t handle = starpu_cublas_get_local_handle(); 
  graph->execute_point_common(starpu_cuda, payload.i, payload.j, output_ptr, output_bytes,
                       input_data, input_bytes, 7, extra_local_memory[tid], graph->scratch_bytes_per_task, handle);

#else
  int rank;
  starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);

  *out = *in1 + *in2 + *in3 + *in4 + *in5 + *in6 + *in7 + 1.0;
  printf("Graph %d, Task6, [%d, %d], rank %d, core %d, in1 %.2f, in2 %.2f, in3 %.2f, in4 %.2f, in5 %.2f, in6 %.2f, in7 %.2f, out %.2f, local_mem %p\n", 
    payload.graph_id, payload.i, payload.j, rank, tid, *in1, *in2, *in3, *in4, *in5, *in6, *in7, *out, extra_local_memory[tid]);
#endif
}

static void task9(void *descr[], void *cl_arg, int starpu_cuda)
{
  float *in1, *in2, *in3, *in4, *in5, *in6, *in7, *in8, *out;
  payload_t payload;
  in1 = (float *)STARPU_MATRIX_GET_PTR(descr[0]);
  in2 = (float *)STARPU_MATRIX_GET_PTR(descr[1]);
  in3 = (float *)STARPU_MATRIX_GET_PTR(descr[2]);
  in4 = (float *)STARPU_MATRIX_GET_PTR(descr[3]);
  in5 = (float *)STARPU_MATRIX_GET_PTR(descr[4]);
  in6 = (float *)STARPU_MATRIX_GET_PTR(descr[5]);
  in7 = (float *)STARPU_MATRIX_GET_PTR(descr[6]);
  in8 = (float *)STARPU_MATRIX_GET_PTR(descr[7]);
  out = (float *)STARPU_MATRIX_GET_PTR(descr[8]);
  starpu_codelet_unpack_args(cl_arg, &payload);
  
  int tid = starpu_worker_get_id();

#if defined (USE_CORE_VERIFICATION)  
  const TaskGraph *graph = payload.graph;
  char *output_ptr = (char*)out;
  size_t output_bytes= graph->output_bytes_per_task;
  const char *input_data[] = {
    (const char*) in1,
    (const char*) in2,
    (const char*) in3,
    (const char*) in4,
    (const char*) in5,
    (const char*) in6,
    (const char*) in7,
    (const char*) in8,
  };
  size_t input_bytes[] = {
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
  };
  cublasHandle_t handle = starpu_cublas_get_local_handle();
  graph->execute_point_common(starpu_cuda, payload.i, payload.j, output_ptr, output_bytes,
                       input_data, input_bytes, 8, extra_local_memory[tid], graph->scratch_bytes_per_task, handle);

#else
  int rank;
  starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);

  *out = *in1 + *in2 + *in3 + *in4 + *in5 + *in6 + *in7 + *in8 + 1.0;
  printf("Graph %d, Task6, [%d, %d], rank %d, core %d, in1 %.2f, in2 %.2f, in3 %.2f, in4 %.2f, in5 %.2f, in6 %.2f, in7 %.2f, in8 %.2f, out %.2f, local_mem %p\n", 
    payload.graph_id, payload.i, payload.j, rank, tid, *in1, *in2, *in3, *in4, *in5, *in6, *in7, *in8, *out, extra_local_memory[tid]);
#endif
}

static void task10(void *descr[], void *cl_arg, int starpu_cuda)
{
  float *in1, *in2, *in3, *in4, *in5, *in6, *in7, *in8, *in9, *out;
  payload_t payload;
  in1 = (float *)STARPU_MATRIX_GET_PTR(descr[0]);
  in2 = (float *)STARPU_MATRIX_GET_PTR(descr[1]);
  in3 = (float *)STARPU_MATRIX_GET_PTR(descr[2]);
  in4 = (float *)STARPU_MATRIX_GET_PTR(descr[3]);
  in5 = (float *)STARPU_MATRIX_GET_PTR(descr[4]);
  in6 = (float *)STARPU_MATRIX_GET_PTR(descr[5]);
  in7 = (float *)STARPU_MATRIX_GET_PTR(descr[6]);
  in8 = (float *)STARPU_MATRIX_GET_PTR(descr[7]);
  in9 = (float *)STARPU_MATRIX_GET_PTR(descr[8]);
  out = (float *)STARPU_MATRIX_GET_PTR(descr[9]);
  starpu_codelet_unpack_args(cl_arg, &payload);
  
  int tid = starpu_worker_get_id();

#if defined (USE_CORE_VERIFICATION)  
  const TaskGraph *graph = payload.graph;
  char *output_ptr = (char*)out;
  size_t output_bytes= graph->output_bytes_per_task;
  const char *input_data[] = {
    (const char*) in1,
    (const char*) in2,
    (const char*) in3,
    (const char*) in4,
    (const char*) in5,
    (const char*) in6,
    (const char*) in7,
    (const char*) in8,
    (const char*) in9,
  };
  size_t input_bytes[] = {
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
    output_bytes,
  };
  cublasHandle_t handle = starpu_cublas_get_local_handle();
  graph->execute_point_common(starpu_cuda, payload.i, payload.j, output_ptr, output_bytes,
                       input_data, input_bytes, 9, extra_local_memory[tid], graph->scratch_bytes_per_task, handle);

#else
  int rank;
  starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);

  *out = *in1 + *in2 + *in3 + *in4 + *in5 + *in6 + *in7 + *in8 + *in9 + 1.0;
  printf("Graph %d, Task6, [%d, %d], rank %d, core %d, in1 %.2f, in2 %.2f, in3 %.2f, in4 %.2f, in5 %.2f, in6 %.2f, in7 %.2f, in8 %.2f, in9 %.2f, out %.2f, local_mem %p\n", 
    payload.graph_id, payload.i, payload.j, rank, tid, *in1, *in2, *in3, *in4, *in5, *in6, *in7, *in8, *in9, *out, extra_local_memory[tid]);
#endif
}

static void mysync() 
{
    cudaError_t err = cudaStreamSynchronize(starpu_cuda_get_local_stream());
    if (err != cudaSuccess) {
      printf("Error synchronizing the stream: %s\n", cudaGetErrorString(err));
    }
}

static void check_stream() 
{
    cudaStream_t stream = starpu_cuda_get_local_stream();
    cudaError_t status = cudaStreamQuery(stream);
    if (status == cudaSuccess) {
        printf("All operations in the stream are complete.\n");
    } else if (status == cudaErrorNotReady) {
        printf("Operations in the stream are still in progress.\n");
    } else {
        printf("Error querying the stream: %s\n", cudaGetErrorString(status));
    }
}

// TODO:
// Here may use template to generate to make it look better
#ifdef STARPU_USE_CUDA
void task1_cuda(void *descr[], void *cl_arg) 
{
  task1(descr, cl_arg, 1);
  // check_stream();
  // mysync();
}

void task2_cuda(void *descr[], void *cl_arg) 
{
  task2(descr, cl_arg, 1);
  // check_stream();
  // mysync();
}

void task3_cuda(void *descr[], void *cl_arg) 
{
  task3(descr, cl_arg, 1);
  // check_stream();
  // mysync();
}

void task4_cuda(void *descr[], void *cl_arg) 
{
  task4(descr, cl_arg, 1);
  // check_stream();
  // mysync();
}

void task5_cuda(void *descr[], void *cl_arg) 
{
  task5(descr, cl_arg, 1);
  // check_stream();
  // mysync();
}

void task6_cuda(void *descr[], void *cl_arg) 
{
  task6(descr, cl_arg, 1);
  // check_stream();
  // mysync();
}

void task7_cuda(void *descr[], void *cl_arg) 
{
  task7(descr, cl_arg, 1);
  // check_stream();
  // mysync();
}

void task8_cuda(void *descr[], void *cl_arg) 
{
  task8(descr, cl_arg, 1);
  // check_stream();
  // mysync();
}

void task9_cuda(void *descr[], void *cl_arg) 
{
  task9(descr, cl_arg, 1);
  // check_stream();
  // mysync();
}

void task10_cuda(void *descr[], void *cl_arg) 
{
  task10(descr, cl_arg, 1);
  // check_stream();
  // mysync();
}
#endif

void task1_cpu(void *descr[], void *cl_arg) 
{
  task1(descr, cl_arg, 0);
}

void task2_cpu(void *descr[], void *cl_arg) 
{
  task2(descr, cl_arg, 0);
}

void task3_cpu(void *descr[], void *cl_arg) 
{
  task3(descr, cl_arg, 0);
}

void task4_cpu(void *descr[], void *cl_arg) 
{
  task4(descr, cl_arg, 0);
}

void task5_cpu(void *descr[], void *cl_arg) 
{
  task5(descr, cl_arg, 0);
}

void task6_cpu(void *descr[], void *cl_arg) 
{
  task6(descr, cl_arg, 0);
}

void task7_cpu(void *descr[], void *cl_arg) 
{
  task7(descr, cl_arg, 0);
}

void task8_cpu(void *descr[], void *cl_arg) 
{
  task8(descr, cl_arg, 0);
}

void task9_cpu(void *descr[], void *cl_arg) 
{
  task9(descr, cl_arg, 0);
}

void task10_cpu(void *descr[], void *cl_arg) 
{
  task10(descr, cl_arg, 0);
}

using TaskFunc = decltype(&task1_cpu);
static std::vector<TaskFunc> task_func_list = {
  task1_cpu,
  task2_cpu,
  task3_cpu,
  task4_cpu,
  task5_cpu,
  task6_cpu,
  task7_cpu,
  task8_cpu,
  task9_cpu,
  task10_cpu,
};


struct starpu_codelet cl_task1; 
struct starpu_codelet cl_task2;
struct starpu_codelet cl_task3;
struct starpu_codelet cl_task4;
struct starpu_codelet cl_task5;
struct starpu_codelet cl_task6;
struct starpu_codelet cl_task7;
struct starpu_codelet cl_task8;
struct starpu_codelet cl_task9;
struct starpu_codelet cl_task10;

typedef struct matrix_s {
  int MT;
  int NT;
  starpu_ddesc_t *ddescA;
}matrix_t;

struct StarPUApp : public App {
public:
  StarPUApp(int argc, char **argv);
  ~StarPUApp();
  void execute_main_loop();
  void execute_timestep(size_t idx, long t);
private:
  void insert_task(int num_args, payload_t &payload, std::array<starpu_data_handle_t, 10> &args);
  void insert_task_custom(int num_args, payload_t &payload, std::array<starpu_data_handle_t, 10> &args, int priority, int abi = 0, int efi = 0);
  void parse_argument(int argc, char **argv);
  void debug_printf(int verbose_level, const char *format, ...);
private:
  struct starpu_conf *conf;
  int rank;
  int world;
  int nb_cores;
  int P;
  int Q;
  int MB;
  char *starpu_schedule;
  char *custom_dag_file = nullptr;
  char *task_type_runtime_file = nullptr;
  char *priority_file = nullptr;
  char *efficiency_file = nullptr;
  char *ability_file = nullptr;
  int n_gpu;
  int auto_opt;
  matrix_t mat_array[10];
};

static char* getTaskNameChar(const std::string& s) {
  static std::unordered_map<std::string, std::string> _store;
  if (_store.find(s) == _store.end()) {
    _store[s] = s;
  }
  return (char*)_store[s].c_str();
}

static std::map<std::string, starpu_codelet> task_name_to_codelet;
static std::map<int, starpu_codelet> task_num_to_codelet;

void StarPUApp::insert_task_custom(int num_args, payload_t &payload, std::array<starpu_data_handle_t, 10> &args, int priority, int abi, int efi)
{
  void (*callback)(void*) = NULL;
  starpu_ddesc_t *descA = mat_array[payload.graph_id].ddescA;
  int t = payload.i;
  int p = payload.j;
  CustomTaskInfo *task_info = payload.graph->get_task_info();
  std::string task_name;
  if (task_info != nullptr) {
    task_name = payload.graph->getTaskTypeAtPoint(t, p);
  } else {
    task_name = "task_" + std::to_string(num_args);
  }
  starpu_codelet* cl_task;
  if (payload.graph->dependence == DependenceType::USER_DEFINED) {
    cl_task = &task_name_to_codelet[task_name];
  } else {
    assert (false && "Not implemented");
  }
  char* task_name_char = getTaskNameChar(task_name);
  assert (strcmp(starpu_schedule, "dmdap") == 0);
  // if schedule == dmdap, check if there is priority_file
  if (strcmp(starpu_schedule, "dmdap") == 0) {
    assert(priority_file != nullptr);
  }
  
  switch(num_args) {
  case 1:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        STARPU_PRIORITY, priority,
        0);
    break;
  case 2:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        STARPU_PRIORITY, priority,
        0);
    break;
  case 3:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        STARPU_PRIORITY, priority,
        0);
    break;
  case 4:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        STARPU_PRIORITY, priority,
        0);
    break;
  case 5:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        STARPU_PRIORITY, priority,
        0);
    break;
  case 6:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_R, args[5],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        STARPU_PRIORITY, priority,
        0);
    break;
  case 7:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_R, args[5],
        STARPU_R, args[6],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        STARPU_PRIORITY, priority,
        0);
    break;
  case 8:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_R, args[5],
        STARPU_R, args[6],
        STARPU_R, args[7],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        STARPU_PRIORITY, priority,
        0);
    break;
  case 9:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_R, args[5],
        STARPU_R, args[6],
        STARPU_R, args[7],
        STARPU_R, args[8],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        STARPU_PRIORITY, priority,
        0);
    break;
  case 10:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_R, args[5],
        STARPU_R, args[6],
        STARPU_R, args[7],
        STARPU_R, args[8],
        STARPU_R, args[9],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        STARPU_PRIORITY, priority,
        0);
    break;
  default:
    assert(false && "unexpected num_args");
  };
}


void StarPUApp::insert_task(int num_args, payload_t &payload, std::array<starpu_data_handle_t, 10> &args)
{
  void (*callback)(void*) = NULL;
  starpu_ddesc_t *descA = mat_array[payload.graph_id].ddescA;
  int t = payload.i;
  int p = payload.j;
  CustomTaskInfo *task_info = payload.graph->get_task_info();
  std::string task_name;
  if (task_info != nullptr) {
    task_name = payload.graph->getTaskTypeAtPoint(t, p);
  } else {
    task_name = "task_" + std::to_string(num_args);
  }
  starpu_codelet* cl_task;
  if (payload.graph->dependence == DependenceType::USER_DEFINED) {
    cl_task = &task_name_to_codelet[task_name];
  } else {
    assert (false && "Not implemented");
  }
  char* task_name_char = getTaskNameChar(task_name);
  if (DEBUG) {
    std::cout << "insert " << task_name_char << std::endl;
    std::cout << "cl_task.name " << cl_task->name << std::endl;
    std::cout << "cl_task.n_buffers " << cl_task->nbuffers << std::endl;
    for (int i = 0; i < 10; i++) {
      if (cl_task->cpu_funcs[0] == task_func_list[i]) {
        std::cout << "cl_task.cpu_funcs[0] == " << i << std::endl;
        break;
      }
    }
    std::cout << "num_args " << num_args << std::endl;
    assert (cl_task->cpu_funcs[0] == task_func_list[num_args - 1]);
  }
  switch(num_args) {
  case 1:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        0);
    break;
  case 2:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        0);
    break;
  case 3:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        0);
    break;
  case 4:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        0);
    break;
  case 5:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        0);
    break;
  case 6:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_R, args[5],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        0);
    break;
  case 7:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_R, args[5],
        STARPU_R, args[6],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        0);
    break;
  case 8:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_R, args[5],
        STARPU_R, args[6],
        STARPU_R, args[7],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        0);
    break;
  case 9:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_R, args[5],
        STARPU_R, args[6],
        STARPU_R, args[7],
        STARPU_R, args[8],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        0);
    break;
  case 10:
    starpu_mpi_insert_task(
        MPI_COMM_WORLD, cl_task,
        STARPU_VALUE,    &payload, sizeof(payload_t),
        STARPU_R, args[1],
        STARPU_R, args[2],
        STARPU_R, args[3],
        STARPU_R, args[4],
        STARPU_R, args[5],
        STARPU_R, args[6],
        STARPU_R, args[7],
        STARPU_R, args[8],
        STARPU_R, args[9],
        STARPU_RW, args[0],
        STARPU_NAME, task_name_char,
        0);
    break;
  default:
    assert(false && "unexpected num_args");
  };
}

void StarPUApp::parse_argument(int argc, char **argv)
{
  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "-mb")) {
      MB = atoi(argv[++i]);
    }
    if (!strcmp(argv[i], "-core")) {
      nb_cores = atoi(argv[++i]);
    }
    if (!strcmp(argv[i], "-ngpu")) {
      n_gpu = atoi(argv[++i]);
    }
    if (!strcmp(argv[i], "-p")) {
      P = atoi(argv[++i]);
    }
    if (!strcmp(argv[i], "-S")) {
      starpu_enable_supertiling = true;
    }
    if (!strcmp(argv[i], "-schedule")) {
      starpu_schedule = argv[++i];
    }
    if (!strcmp(argv[i], "-custom_dag")) {
      custom_dag_file = argv[++i];
    }
    if (!strcmp(argv[i], "-task_type_runtime")) {
      task_type_runtime_file = argv[++i];
    }
    if (!strcmp(argv[i], "-priority")) {
      priority_file = argv[++i];
    }
    if (!strcmp(argv[i], "-efficiency")) {
      efficiency_file = argv[++i];
    }
    if (!strcmp(argv[i], "auto_opt")) {
      auto_opt = atoi(argv[++i]);
    }
  }
}

static starpu_perfmodel perfmodels[10];
char *symbol[10] = {"task1_pm", "task2_pm", "task3_pm", "task4_pm", "task5_pm", "task6_pm", "task7_pm", "task8_pm", "task9_pm", "task10_pm"};

static starpu_perfmodel * init_starpu_perfmodel(int index) {
  struct starpu_perfmodel* model = &perfmodels[index];
  model->symbol = symbol[index];
	model->type = STARPU_HISTORY_BASED;

	// starpu_perfmodel_init(model);
  return model;
}

static starpu_perfmodel* get_history_based_perfmodel(const std::string& task_type, int index) {
  static std::map<std::string, starpu_perfmodel*> task_type_to_perfmodel;
  starpu_perfmodel* model;
  if (task_type_to_perfmodel.find(task_type) == task_type_to_perfmodel.end()) {
    model = new starpu_perfmodel();
    model->symbol = getTaskNameChar(task_type);
    model->type = STARPU_HISTORY_BASED;
    task_type_to_perfmodel[task_type] = model;
  } else {
    model = task_type_to_perfmodel[task_type];
  }
  return model;
}

static void init_task_cl_custom(int input_num, const std::string& task_name) {
  static std::map<int, TaskFunc> task_num_to_cpu_func = {
    {1, task1_cpu},
    {2, task2_cpu},
    {3, task3_cpu},
    {4, task4_cpu},
    {5, task5_cpu},
    {6, task6_cpu},
    {7, task7_cpu},
    {8, task8_cpu},
    {9, task9_cpu},
    {10, task10_cpu},
  };
  static std::map<int, TaskFunc> task_num_to_cuda_func = {
    {1, task1_cuda},
    {2, task2_cuda},
    {3, task3_cuda},
    {4, task4_cuda},
    {5, task5_cuda},
    {6, task6_cuda},
    {7, task7_cuda},
    {8, task8_cuda},
    {9, task9_cuda},
    {10, task10_cuda},
  };
  if (task_name_to_codelet.find(task_name) == task_name_to_codelet.end()) {
    starpu_codelet codelet = starpu_codelet();

    codelet = starpu_codelet();
    codelet.where     = STARPU_CPU | STARPU_CUDA;                                   
    codelet.cpu_funcs[0] = task_num_to_cpu_func[input_num + 1]; // because has output
    codelet.cpu_funcs_name[0] = getTaskNameChar(task_name),
    codelet.cuda_funcs[0]  = task_num_to_cuda_func[input_num + 1];
    codelet.nbuffers  = input_num + 1;                                           
    codelet.name      = getTaskNameChar(task_name);
    codelet.model    = get_history_based_perfmodel(task_name, input_num + 1);

    task_name_to_codelet[task_name] = codelet;
  }
}

static void init_task_default() {
  cl_task1.where     = STARPU_CPU | STARPU_CUDA;                                   
  cl_task1.cpu_funcs[0]  = task1_cpu;                                       
  cl_task1.cpu_funcs_name[0] = "task1_cpu",
  cl_task1.cuda_funcs[0]  = task1_cuda;
  cl_task1.nbuffers  = 1;                                           
  cl_task1.name      = "task1";
  cl_task1.model    = init_starpu_perfmodel(0);
  
  cl_task2.where     = STARPU_CPU | STARPU_CUDA;                                   
  cl_task2.cpu_funcs[0]  = task2_cpu;                                       
  cl_task2.cpu_funcs_name[0] = "task2_cpu",
  cl_task2.cuda_funcs[0]  = task2_cuda;
  cl_task2.nbuffers  = 2;                                           
  cl_task2.name      = "task2";
  cl_task2.model    = init_starpu_perfmodel(1);
  
  cl_task3.where     = STARPU_CPU | STARPU_CUDA;                                   
  cl_task3.cpu_funcs[0]  = task3_cpu;                                       
  cl_task3.cpu_funcs_name[0] = "task3_cpu",
  cl_task3.cuda_funcs[0]  = task3_cuda;
  cl_task3.nbuffers  = 3;                                           
  cl_task3.name      = "task3";
  cl_task3.model    = init_starpu_perfmodel(2);
  
  cl_task4.where     = STARPU_CPU | STARPU_CUDA;                                   
  cl_task4.cpu_funcs[0]  = task4_cpu;                                       
  cl_task4.cpu_funcs_name[0] = "task4_cpu",
  cl_task4.cuda_funcs[0]  = task4_cuda;
  cl_task4.nbuffers  = 4;                                           
  cl_task4.name      = "task4";
  cl_task4.model    = init_starpu_perfmodel(3);
  
  cl_task5.where     = STARPU_CPU | STARPU_CUDA;                                   
  cl_task5.cpu_funcs[0]  = task5_cpu;                                       
  cl_task5.cpu_funcs_name[0] = "task5_cpu",
  cl_task5.cuda_funcs[0]  = task5_cuda;
  cl_task5.nbuffers  = 5;                                           
  cl_task5.name      = "task5";
  cl_task5.model    = init_starpu_perfmodel(4);  

  cl_task6.where     = STARPU_CPU | STARPU_CUDA;                                   
  cl_task6.cpu_funcs[0]  = task6_cpu;                                       
  cl_task6.cpu_funcs_name[0] = "task6_cpu",
  cl_task6.cuda_funcs[0]  = task6_cuda;
  cl_task6.nbuffers  = 6;                                           
  cl_task6.name      = "task6";
  cl_task6.model    = init_starpu_perfmodel(5);
  
  cl_task7.where     = STARPU_CPU | STARPU_CUDA;                                   
  cl_task7.cpu_funcs[0]  = task7_cpu;                                       
  cl_task7.cpu_funcs_name[0] = "task7_cpu",
  cl_task7.cuda_funcs[0]  = task7_cuda;
  cl_task7.nbuffers  = 7;                                           
  cl_task7.name      = "task7";
  cl_task7.model    = init_starpu_perfmodel(6);

  cl_task8.where     = STARPU_CPU | STARPU_CUDA;                                   
  cl_task8.cpu_funcs[0]  = task8_cpu;                                       
  cl_task8.cpu_funcs_name[0] = "task8_cpu",
  cl_task8.cuda_funcs[0]  = task8_cuda;
  cl_task8.nbuffers  = 8;                                           
  cl_task8.name      = "task8";
  cl_task8.model    = init_starpu_perfmodel(7);
  
  cl_task9.where     = STARPU_CPU | STARPU_CUDA;                                   
  cl_task9.cpu_funcs[0]  = task9_cpu;                                       
  cl_task9.cpu_funcs_name[0] = "task9_cpu",
  cl_task9.cuda_funcs[0]  = task9_cuda;
  cl_task9.nbuffers  = 9;                                           
  cl_task9.name      = "task9";
  cl_task9.model    = init_starpu_perfmodel(8);
  
  cl_task10.where     = STARPU_CPU | STARPU_CUDA;                                   
  cl_task10.cpu_funcs[0]  = task10_cpu;                                       
  cl_task10.cpu_funcs_name[0] = "task10_cpu",
  cl_task10.cuda_funcs[0]  = task10_cuda;
  cl_task10.nbuffers  = 10;                                           
  cl_task10.name      = "task10";
  cl_task10.model    = init_starpu_perfmodel(9);
}

StarPUApp::StarPUApp(int argc, char **argv)
  : App(argc, argv)
{
  init(); 
  
  int i;
  
  P = 1;
  MB = 2;
  nb_cores = 1;
  n_gpu = 0;

  starpu_schedule = "lws";
  
  parse_argument(argc, argv);

  if (custom_dag_file != nullptr) {
    for (int i = 0; i < graphs.size(); i++) {
      TaskGraph &graph = graphs[i];
      CustomTaskInfo *custom_task_info;
      assert(graph.dependence == DependenceType::USER_DEFINED);
      if (priority_file != nullptr) {
        assert(ability_file == nullptr);
        ability_file = priority_file;
        if (efficiency_file == nullptr)
          efficiency_file = priority_file;
      }
      if (priority_file != nullptr && efficiency_file != nullptr) {
        custom_task_info = new CustomTaskInfo(custom_dag_file, task_type_runtime_file, priority_file, ability_file, efficiency_file);
      } else if (priority_file == nullptr && task_type_runtime_file != nullptr) {
        custom_task_info = new CustomTaskInfo(custom_dag_file, task_type_runtime_file);
      } else if (priority_file != nullptr && task_type_runtime_file == nullptr) {
        custom_task_info = new CustomTaskInfo(custom_dag_file, priority_file, ability_file, efficiency_file);
      } else {
        custom_task_info = new CustomTaskInfo(custom_dag_file);
      }
      graph.set_task_info(custom_task_info);
    }
  } else {
    for (int i = 0; i < graphs.size(); i++) {
      TaskGraph &graph = graphs[i];
      assert(graph.dependence != DependenceType::USER_DEFINED);
    }
  }

  for (int i = 0; i < graphs.size(); i++) {
    TaskGraph &graph = graphs[i];
    // if dependency is not user_defined, init task_cl using init_task_default
    if (graph.dependence != DependenceType::USER_DEFINED) {
      init_task_default();
      continue;
    }
    // if dependency is user_defined, init task_cl using init_task_cl_custom
    // should traverse all the task
    for (int t = 0; t < graph.timesteps; t++) {
      int width = graph.getUserDefineWidthAtTimestep(t);
      for (int p = 0; p < width; p++) {
        const std::string& task_type = graph.getTaskTypeAtPoint(t, p);
        int num_args = graph.user_defined_dependencies(t, p).size();
        init_task_cl_custom(num_args, task_type);
      }
    }
  }
  
  conf =  (struct starpu_conf *)malloc (sizeof(struct starpu_conf));
  starpu_conf_init( conf );

  conf->ncpus = nb_cores;
  conf->ncuda = n_gpu;
  conf->nopencl = 0;
  conf->sched_policy_name = starpu_schedule;
  conf->auto_opt = auto_opt;
  
  int ret;
  ret = starpu_init(conf);
  STARPU_CHECK_RETURN_VALUE(ret, "starpu_init");
  ret = starpu_mpi_init(&argc, &argv, 1);
  STARPU_CHECK_RETURN_VALUE(ret, "starpu_mpi_init");
  starpu_cublas_init();
  starpu_mpi_comm_rank(MPI_COMM_WORLD, &rank);
  starpu_mpi_comm_size(MPI_COMM_WORLD, &world);
  
  Q = world/P;
  assert(P*Q == world);  
  
  size_t max_scratch_bytes_per_task = 0;
  
  int MB_cal = 0;
  
  for (i = 0; i < graphs.size(); i++) {
    TaskGraph &graph = graphs[i];
    matrix_t &mat = mat_array[i];
    
    MB_cal = sqrt(graph.output_bytes_per_task / sizeof(float));
    if (MB_cal > MB) {
      MB = MB_cal;
    }
    // std::cout << "MB_cal " << MB_cal << std::endl;
    // std::cout << "MB " << MB << std::endl;

    // std:: cout << "graph.output_bytes_per_task " << graph.output_bytes_per_task << std::endl;
    // // print MB * MB * sizeof(float)
    // std::cout << "MB * MB * sizeof(float) " << MB * MB * sizeof(float) << std::endl;
    
    mat.NT = graph.max_width;
    mat.MT = graph.nb_fields;
  
    debug_printf(0, "mb %d, mt %d, nt %d, timesteps %d, enable_supertiling %d, nb_fields %d\n", MB, mat.MT, mat.NT, graph.timesteps, starpu_enable_supertiling, graph.nb_fields);
    assert (graph.output_bytes_per_task <= sizeof(float) * MB * MB);

    mat.ddescA = create_and_distribute_data(rank, world, MB, MB, mat.MT, mat.NT, P, Q, i);
    
    if (graph.scratch_bytes_per_task > max_scratch_bytes_per_task) {
      max_scratch_bytes_per_task = graph.scratch_bytes_per_task;
    }
  }
   
  extra_local_memory = (char**)malloc(sizeof(char*) * nb_cores);
  assert(extra_local_memory != NULL);
  for (i = 0; i < nb_cores; i++) {
    extra_local_memory[i] = NULL;
  }
  if (max_scratch_bytes_per_task > 0) {
    starpu_execute_on_each_worker_ex(init_extra_local_memory, (void*) (uintptr_t) max_scratch_bytes_per_task, STARPU_CPU | STARPU_CUDA, "init_scratch");
  }
  debug_printf(0, "max_scratch_bytes_per_task %lld\n", max_scratch_bytes_per_task);
}

StarPUApp::~StarPUApp()
{
  int i;
  for (i = 0; i < nb_cores; i++) {
    if (extra_local_memory[i] != NULL) {
      free(extra_local_memory[i]);
      extra_local_memory[i] = NULL;
    }
  }
  free(extra_local_memory);
  extra_local_memory = NULL;
 
  for (i = 0; i < graphs.size(); i++) {
    matrix_t &mat = mat_array[i];
    destroy_data(mat.ddescA);
  }  
  if (conf != NULL) {
    free (conf);
  } 
  starpu_mpi_shutdown();
  starpu_shutdown();
}


void StarPUApp::execute_main_loop()
{
  if (rank == 0) {
    display();
  }

  void (*callback)(void*) = NULL;

  int x, y;

  /* Initialize data structures before measurement */
  for (int i = 0; i < graphs.size(); i++) {
    const TaskGraph &g = graphs[i];
    // if DependencyType::USER_DEFINED, we need to initialize the data for the first timestep
    // Now, graph.size() always equals to 1
    // TODO: implement this. Read from file, and call setDependenceFromPreSet

    for (y = 0; y < g.timesteps; y++) {
      long offset = g.offset_at_timestep(y);
      long width = g.width_at_timestep(y);
      if (DEBUG)
        std::cout << "timestep " << y << ", offset " << offset << ", width " << width << std::endl;
      matrix_t &mat = mat_array[i];
      int nb_fields = g.nb_fields;

      for (int x = offset; x <= offset+width-1; x++)
        starpu_desc_getaddr( mat.ddescA, y%nb_fields, x );
    }
  }
  /* start timer */
  starpu_mpi_barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    Timer::time_start();
  }
  
  for (int i = 0; i < graphs.size(); i++) {
    const TaskGraph &g = graphs[i];

    for (y = 0; y < g.timesteps; y++) {
      execute_timestep(i, y);
    }
  }

  starpu_task_wait_for_all();
  starpu_mpi_barrier(MPI_COMM_WORLD);
  if (rank == 0) {
    double elapsed = Timer::time_end();
    report_timing(elapsed);
  }

  for (int i = 0; i < graphs.size(); i++) { 
    const TaskGraph &g = graphs[i]; 
    if (g.dependence == DependenceType::USER_DEFINED) {
      g.destroy_task_info();
    }
  }
}

void StarPUApp::execute_timestep(size_t idx, long t)
{
  const TaskGraph &g = graphs[idx];
  long offset = g.offset_at_timestep(t);
  long width = g.width_at_timestep(t);
  long dset = g.dependence_set_at_timestep(t);
  matrix_t &mat = mat_array[idx];
  int nb_fields = g.nb_fields;
  
  std::array<starpu_data_handle_t, 10> args;
  payload_t payload;
  
  debug_printf(1, "ts %d, offset %d, width %d, offset+width-1 %d\n", t, offset, width, offset+width-1);
  for (int x = offset; x <= offset+width-1; x++) {
    std::vector<std::pair<long, long> > deps;
    if (g.dependence != DependenceType::USER_DEFINED) {
      deps = g.dependencies(dset, x);
    } else {
      deps = g.user_defined_dependencies(t, x);
    }
    int num_args; 
    starpu_data_handle_t output = starpu_desc_getaddr( mat.ddescA, t%nb_fields, x );
#ifdef ENABLE_PRUNE_MPI_TASK_INSERT
    int has_task = 0;   
    
    if(desc_islocal(mat.ddescA, t%nb_fields, x)) {
      has_task = 1;
    } else if (starpu_mpi_cached_receive(output)) {
      /* We need to invalidate our copy */
      has_task = 1;
    }
    
    if (deps.size() != 0 && t != 0 && has_task != 1) {
      for (std::pair<long, long> dep : deps) {
        for (int i = dep.first; i <= dep.second; i++) {
          if(desc_islocal(mat.ddescA, (t-1)%nb_fields, i)) {
            has_task = 1;
            break;
          }
        }
        if (has_task)
          break;
      }
    }

    debug_printf(1, "rank: %d, has_task: %d, x: %d, t: %d, task_id: %d\n", rank , has_task, x, t, mat.NT * t + x + 1);
    
    if (has_task == 0) {
      continue;
    }

#endif
    
    num_args = 0;
    if (deps.size() == 0) {
      args[num_args++] = output;
      if (DEBUG) {
        std::cout << "t = " << t << " p = "<<  x << " dep size = " << deps.size() << std::endl;
        std::cout << "------------------" << std::endl;
      }
      debug_printf(1, "%d[%d] ", x, num_args);
    } else {
      if (t == 0) {
        args[num_args++] = output;
        debug_printf(1, "%d[%d] ", x, num_args);
      } else {
        args[num_args++] = output;
        long last_offset = g.offset_at_timestep(t-1);
        long last_width = g.width_at_timestep(t-1);
        if (g.dependence == DependenceType::USER_DEFINED) {
          if (DEBUG) {
            std::cout << "t = " << t << " p = "<<  x << " dep size = " << deps.size() << std::endl;
          }
          for (std::pair<long, long> dep : deps) {
            long last_time_step = dep.first;
            long last_point = dep.second;
            if (DEBUG) {
              std::cout << "last_time_step = " << last_time_step << ", last_point = " << last_point << std::endl;
              std::cout << "------------------" << std::endl;
            }
            args[num_args++] = starpu_desc_getaddr( mat.ddescA, (last_time_step)%nb_fields, last_point);
          }
        } else {
          for (std::pair<long, long> dep : deps) {
            for (int i = dep.first; i <= dep.second; i++) {
              if (i >= last_offset && i < last_offset + last_width) {
                args[num_args++] = starpu_desc_getaddr( mat.ddescA, (t-1)%nb_fields, i );
              }
            }
            debug_printf(1, "%d[%d, %d, %d] ", x, num_args, dep.first, dep.second); 
          }
        }
      }
    }
    
    payload.i = t;
    payload.j = x;
    payload.graph = &g;
    payload.graph_id = idx;
    if (strcmp(starpu_schedule, "dmdap") == 0) {
      int priority = 0;
      if (g.dependence == DependenceType::USER_DEFINED) {
        CustomTaskInfo *custom_task_info = (CustomTaskInfo*)g.get_task_info();
        priority = custom_task_info->getTaskPriorityAtPoint(t, x);
      } else {
        assert(false && "dmdap with non-user-defined dependence is not supported yet");
      }
      insert_task_custom(num_args, payload, args, priority, 0, 0);
    } else {
      if (DEBUG) {
        std::cout << "t = " << t << " p = "<<  x << " dep size = " << deps.size() << std::endl;
      }
      insert_task(num_args, payload, args);
    }
    // insert_task(num_args, payload, args); 
  }
  debug_printf(1, "\n");
}

void StarPUApp::debug_printf(int verbose_level, const char *format, ...)
{
  if (verbose_level > VERBOSE_LEVEL) {
    return;
  }
  if (rank == 0) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
  }
}


int main(int argc, char **argv)
{
 // printf("pid %d, %d\n", getpid(), STARPU_NMAXBUFS);
 // sleep(10); 

  
  StarPUApp app(argc, argv);
  app.execute_main_loop();

  return 0;
}

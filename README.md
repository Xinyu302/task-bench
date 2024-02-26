# A Task Benchmark [![CI](https://github.com/StanfordLegion/task-bench/actions/workflows/main.yml/badge.svg)](https://github.com/StanfordLegion/task-bench/actions/workflows/main.yml)

**Please contact the authors before publishing any results obtained
with Task Bench.**

Corresponding authors:

  * Elliott Slaughter <slaughter@cs.stanford.edu>
  * Wei Wu <weiwu@nvidia.com>

Task Bench is a configurable benchmark for evaluating the efficiency
and performance of parallel and distributed programming models,
runtimes, and languages. It is primarily intended for evaluating
task-based models, in which the basic unit of execution is a task, but
it can be implemented in any parallel system. The benchmark consists
of a configurable task graph (which can be thought of as an iteration
space with tasks at every point), with configurable dependencies
between tasks. Tasks execute a configurable set of kernels, which
allow the execution to be compute-bound, memory-bound,
communication-bound, or runtime overhead-bound (with empty tasks).

The following configurations are currently supported:

Implementations:
[Charm++](charm++),
[Chapel](chapel),
[Dask](dask),
[HPX](hpx),
[Legion](legion),
[MPI](mpi),
[MPI+OpenMP](mpi_openmp),
[OmpSs](ompss),
[OpenMP](openmp),
[PaRSEC](parsec),
[Pygion](pygion),
[Realm](realm),
[Regent](regent),
[Spark](spark),
[StarPU](starpu),
[Swift/T](swift),
[TensorFlow](tensorflow),
[X10](x10)

Dependence patterns:
trivial,
no_comm,
stencil_1d,
stencil_1d_periodic,
dom,
tree,
fft,
all_to_all,
nearest,
spread,
random_nearest

Kernels:
compute-bound,
memory-bound,
load-imbalanced compute-bound,
empty

## Quickstart

To build and run just the Legion implementation, run:

```
git clone https://github.com/StanfordLegion/task-bench.git
cd task-bench
DEFAULT_FEATURES=0 USE_LEGION=1 ./get_deps.sh
./build_all.sh
```

(Remove `DEFAULT_FEATURES=0` to build all the implementations.)

The benchmark supports various dependence types:

```
./legion/task_bench -steps 4 -width 4 -type trivial
./legion/task_bench -steps 4 -width 4 -type no_comm
./legion/task_bench -steps 4 -width 4 -type stencil_1d
./legion/task_bench -steps 4 -width 4 -type fft
./legion/task_bench -steps 9 -width 4 -type dom
./legion/task_bench -steps 4 -width 4 -type all_to_all
```

And different kernels can be plugged in to the execution:

```
./legion/task_bench -kernel compute_bound -iter 1024
./legion/task_bench -kernel memory_bound -scratch 8192 -iter 16
./legion/task_bench -kernel load_imbalance -iter 1024
```

## Experimental Configuration

For detailed instructions on configuring task bench for performance
experiments, see [EXPERIMENT.md](EXPERIMENT.md).

## Task-bench增强

### task-bench的不足之处

1. 任务同构，每个任务的执行时间相同，且没有GPU任务，没有考虑异构设备之间的数据拷贝以及任务在不同设备上执行速度不同产生的影响

2. 计算图依赖模式较为规则，只能在相邻的时间步之间产生依赖，不能模拟依赖较为复杂DAG图（例如cholesky）。

3. Tree模式下一层的节点增长必须是上一层的倍数，且没有规约过程。

4. 处理的数据通过对应任务在任务矩阵中的位置指定，即task-bench在设计上会让每个任务产生一块新的数据（与之对比，真实的cholesky会有多个任务写同一块数据），造成任务规模大时内存溢出

### 增强
增加了自定义依赖、自定义任务执行时间以及对GPU的支持。目前仅实现了对StarPU的支持，在对其他运行时系统进行支持时，仅需改用其他运行时系统的**插入任务API**即可。

### 随机任务图生成
根据论文"Neural Topological Ordering for Computation Graphs"中给出的算法，使用下面的方法构造出符合实际应用的任务图。

给出总共的节点数N
边密度edge_density
不相邻的层之间连接边的密度skip_connection_density等信息
1. 生成一个图，图的节点数为N，图的宽度为W，图的层数为L

2. 返回图的节点类型(以输入的数量作为分类标准)

3. 返回节点的输出节点

4. 约定第一层为输入层，最后一层为输出层，输入层最好只有一个节点，输出层最好只有一个节点。

**运行**
```bash
cd generator
python3 run_generate.py --N {N} --edge_density {edge_density} --skip_connection_density {skip_connection_density}
```

如果不传递任何参数，会按照默认值生成节点数为5000,13000,21000,29000的图

```bash
python3 run_generate.py
```

生成的图的格式为：
```
任务类型0: [任务编号0, 任务编号1...]
...
任务类型N: [任务编号m, 任务编号k, ...]

任务编号0: (任务0的所有子节点)
...
任务编号k: (任务k的所有子节点)
```

在生成好任务图后，调用dag2prio.py可以生成任务图中的每个任务的优先级。

```bash
python3 dag2prio.py
```

### 改进后的task-bench的使用方式
```bash
./build_all.sh
```

在starpu目录下，按照下面的命令进行调用：
```bash
./main -type user_defined -kernel customize -schedule dmda -custom_dag dag_dot_prof_file_3840_dmda.txt -task_type_runtime cholesky2.runtime  -core 3 -ngpu 1 -output 3686400 
```

-type需要指定成user_defined
-kernel需要指定成customize
-schedule指定starpu使用的调度方法
-custom_dag指定任务图，其格式与生成的图的格式相同
-task_type_runtime指定任务自定义任务的在CPU上和在GPU上的运行时间
-core指定CPU核数
-ngpu指定GPU个数 

task_type_runtime的格式为:
```
任务种类0: (CPU_RUNTIME, GPU_RUNTIME)
...
```

例如，在cholesky中，任务的执行时间如下所示：
```bash
GEMM: (0.08921855,   21.18881)
POTRF:  (19.02467,    5.798251)
SYRK: (0.1317194,  12.89934)
TRSM: (0.3274957,  12.29771)
```

另外，可以使用-priority指定自定义任务的优先级：
```bash
./main -type user_defined -kernel customize -schedule dmdap -custom_dag dag_dot_prof_file_30720_dmda.txt -task_type_runtime cholesky2.runtime -priority 30720.txt -core 14 -ngpu 1 -output 3686400
```

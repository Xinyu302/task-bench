# Task Bench

## Enhancements to Task-bench

### Shortcomings of task-bench

1. Tasks are homogeneous, with each task having the same execution time, and there are no GPU tasks. The impact of data copying between heterogeneous devices and the difference in execution speed of tasks on different devices has not been considered.
2. The dependency pattern of the computation graph is rather regular, only allowing dependencies between adjacent time steps and cannot simulate complex dependency DAG graphs (e.g., cholesky).
3. In the Tree pattern, the growth of nodes in one layer must be a multiple of the previous layer, and there is no reduction process.
4. The data processed is specified by the position of the corresponding task in the task matrix. That is, task-bench is designed to generate new data for each task (in contrast, real cholesky will have multiple tasks writing to the same data), resulting in memory overflow when the task scale is large.

### Enhancements

Support for custom dependencies, custom task execution times, and GPU support has been added. Currently, only support for StarPU has been implemented. When supporting other runtime systems, simply switch to the **insert task API** of the other runtime system.

### Random Task Graph Generation

Using the algorithm provided in the paper "Neural Topological Ordering for Computation Graphs," the following method constructs task graphs that meet the requirements of practical applications.

Given the total number of nodes N, edge density edge_density, skip connection density skip_connection_density, and other information about connections between non-adjacent layers:

1. Generate a graph with N nodes, a width of W, and L layers.
2. Return the types of nodes in the graph (using the number of inputs as a classification criterion).
3. Return the output nodes of the nodes.
4. The first layer is designated as the input layer, and the last layer is the output layer. It is best to have only one node in the input layer and one node in the output layer.

**Execution**

```bash
cd generator
python3 run_generate.py --N {N} --edge_density {edge_density} --skip_connection_density {skip_connection_density}
```

If no parameters are passed, graphs with node numbers of 5000, 13000, 21000, and 29000 will be generated using default values.

```bash
python3 run_generate.py
```

The format of the generated graph is:

```bash
Task Type 0: [Task ID 0, Task ID 1...]
...
Task Type N: [Task ID m, Task ID k, ...]

Task ID 0: (All child nodes of Task 0)
...
Task ID k: (All child nodes of Task k)
```

After generating the task graph, calling dag2prio.py will generate the priority of each task in the task graph.

```bash
python3 dag2prio.py
```

### Improved Usage of task-bench

To build and run just the StaPU implementation, run:

```bash
git clone https://github.com/Xinyu302/task-bench.git
cd task-bench
DEFAULT_FEATURES=0 USE_STARPU=1 ./get_deps.sh
./build_all.sh
```

If you want to integrate INSPIRIT, and use INSPIRIT instead of the default StarPU, you can do it easily in this way.

```bash
rm deps/starpu/starpu-1.3.4 -rf
cp -r path_to_INSPIRIT deps/starpu/starpu-1.3.4
```

Then use build_all.sh to build TaskBench and the runtime you choose.

```bash
./build_all.sh
```

Under the StarPU directory, use the following command:

```bash
./main -type user_defined -kernel customize -schedule dmda -custom_dag dag_dot_prof_file_3840_dmda.txt -task_type_runtime cholesky2.runtime  -core 3 -ngpu 1 -output 3686400 
```

-type should be specified as user_defined

-kernel should be specified as customize

-schedule specifies the scheduling method used by StarPU 

-custom_dag specifies the task graph, in the same format as the generated graph 

-task_type_runtime specifies the runtime of custom tasks on the CPU and GPU, and the format of task_type_runtime is:

```
Task Type 0: (CPU_RUNTIME, GPU_RUNTIME)
...
```

For example, in cholesky, the execution time of tasks is as follows:

```
GEMM: (0.08921855,   21.18881)
POTRF:  (19.02467,    5.798251)
SYRK: (0.1317194,  12.89934)
TRSM: (0.3274957,  12.29771)
```

-core specifies the number of CPU cores 

-ngpu specifies the number of GPUs

Additionally, you can use -priority to specify the priority of custom tasks:

```bash
./main -type user_defined -kernel customize -schedule dmdap -custom_dag dag_dot_prof_file_30720_dmda.txt -task_type_runtime cholesky2.runtime -priority 30720.txt -core 14 -ngpu 1 -output 3686400
```

Three file mentioned above are located in directory **starpu**.
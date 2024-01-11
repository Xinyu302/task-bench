#ifndef CUSTOMTASKINFO_H
#define CUSTOMTASKINFO_H

#include <vector>
#include <algorithm>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>

#define DEBUG 0

class InitializeState {
public:
    InitializeState();
    InitializeState(bool initialized);

    void setInitialized();
    void setUninitialized();
    bool isInitialized() const;

private:
    bool initialized;
};

class TaskDepInfo : public InitializeState {
public:
    using TaskIndexType = long;
    TaskDepInfo(const std::string& dag_file);
    TaskDepInfo();

    void ParseTaskDepInfo();

    using TaskDep = std::vector<std::pair<TaskIndexType, TaskIndexType>>;
    using LayerDep = std::vector<TaskDep>;
    using GraphDep = std::vector<LayerDep>;

    GraphDep get_dep();

    int get_timestamp() const;
    int get_width_of_timestamp(int t) const;
    int get_max_width() const;
    const std::vector<std::string>& get_task_of_timestamp(int t) const;

    friend std::ostream& operator<<(std::ostream& os, const TaskDepInfo& task_info);

private:
    std::string dag_file;
    std::vector<std::string> all_task;
    std::unordered_set<std::string> input_task;
    std::unordered_set<std::string> output_task;
    std::unordered_map<std::string, std::vector<std::string>> task2output;
    std::unordered_map<std::string, std::string> tag2task;
    std::unordered_map<std::string, std::string> task2tag;
    std::unordered_set<std::string> has_input;
    std::unordered_map<std::string, std::vector<std::string>> task2input;
    std::unordered_map<std::string, std::vector<std::string>> task2input_copy;
    std::vector<std::vector<std::string>> layer_topo_order;
    std::unordered_map<std::string, std::pair<TaskIndexType, TaskIndexType>> task2index;

    void parse_dag();
    void topological_sort();
    TaskDep get_dep(const std::string& task) const;
    void reindex_task();
    std::string trim(const std::string& str, const std::string& delimiters = " \f\n\r\t\v") const;
};

class TaskPriority : public InitializeState {
public:
    TaskPriority(const std::string& pri, const std::string& efi);
    TaskPriority();

    void parse(std::string file, std::vector<int>& vec);
    void parseTaskPriority();

    int get_priority(int task_id) const;

private:
    std::string priority_file;
    std::string efficiency_file;

    std::vector<int> priority;
    std::vector<int> efficiency;
};

class TaskExecTime : public InitializeState {
public:
    TaskExecTime(const std::string& file);
    std::pair<double, double> get_exec_time(const std::string& task_type) const;
    std::pair<double, double> get_exec_time(const std::string& task_type, bool use_gpu) const;
    TaskExecTime();

    void parseTaskExecTime();
    void setDefaultExecTime();

private:
    std::unordered_map<std::string, std::pair<double, double>> taskType2execTime;
    std::string exec_time_file;
};

class CustomTaskInfo : public TaskDepInfo, public TaskPriority, public TaskExecTime {
public:
    CustomTaskInfo(const std::string& dag_file, const std::string& pri, const std::string& efi, const std::string& task_exec_time);
    CustomTaskInfo(const std::string& dag_file, const std::string& pri, const std::string& efi);
    CustomTaskInfo(const std::string& dag_file, const std::string& task_exec_time);
    CustomTaskInfo(const std::string& dag_file);

    bool taskDepInfoInitialized() const;
    bool taskPriorityInitialized() const;
    bool taskExecTimeInitialized() const;
};

#endif // CUSTOMTASKINFO_H

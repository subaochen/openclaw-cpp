#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <chrono>

namespace openclaw {

// 任务优先级
enum class TaskPriority {
    LOW = 0,
    MEDIUM,
    HIGH,
    CRITICAL
};

// 任务状态
enum class TaskStatus {
    PENDING = 0,
    SCHEDULED,
    RUNNING,
    COMPLETED,
    FAILED,
    CANCELLED,
    TIMEOUT
};

// 任务类型
enum class TaskType {
    UNKNOWN = 0,
    DEVELOPMENT,
    TESTING,
    ARCHITECTURE,
    PROJECT_MANAGEMENT,
    CUSTOM
};

// 资源需求
struct ResourceRequirements {
    size_t memoryMB{0};
    size_t cpuCores{1};
    double cpuUsage{50.0}; // 百分比
};

// 任务配置
struct TaskConfig {
    std::string id;
    std::string name;
    std::string description;
    TaskType type{TaskType::UNKNOWN};
    TaskPriority priority{TaskPriority::MEDIUM};
    std::unordered_map<std::string, std::string> parameters;
    std::vector<std::string> dependencies;
    ResourceRequirements resourceRequirements;
    std::string assignedAgentId;
    size_t timeoutSeconds{300}; // 默认5分钟
    size_t maxRetries{3};
    
    bool validate() const;
};

// 任务结果
struct TaskResult {
    bool success{false};
    std::string errorMessage;
    std::unordered_map<std::string, std::string> output;
    std::chrono::milliseconds executionTime{0};
    std::string logPath;
    
    TaskResult() = default;
    explicit TaskResult(bool ok) : success(ok) {}
};

// 任务执行信息
struct TaskExecutionInfo {
    std::string taskId;
    std::string agentId;
    TaskStatus status{TaskStatus::PENDING};
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    std::chrono::milliseconds elapsedTime{0};
    size_t retryCount{0};
    std::string currentPhase;
    double progress{0.0}; // 0-100%
};

// 任务
class Task {
public:
    Task(const TaskConfig& config);
    ~Task() = default;
    
    // 获取任务信息
    std::string getId() const { return config_.id; }
    std::string getName() const { return config_.name; }
    TaskType getType() const { return config_.type; }
    TaskPriority getPriority() const { return config_.priority; }
    TaskStatus getStatus() const { return status_; }
    const TaskConfig& getConfig() const { return config_; }
    TaskExecutionInfo getExecutionInfo() const { return executionInfo_; }
    
    // 状态管理
    void setStatus(TaskStatus status);
    void setAssignedAgent(const std::string& agentId);
    void setProgress(double progress);
    void setPhase(const std::string& phase);
    
    // 执行控制
    void markStarted();
    void markCompleted(const TaskResult& result);
    void markFailed(const std::string& error);
    void markCancelled();
    void markTimeout();
    
    // 依赖检查
    bool areDependenciesMet(const std::vector<std::string>& completedTasks) const;
    const std::vector<std::string>& getDependencies() const { return config_.dependencies; }
    
    // 资源检查
    bool canResourceRequirementsBeMet(const ResourceRequirements& available) const;

private:
    TaskConfig config_;
    TaskStatus status_{TaskStatus::PENDING};
    TaskExecutionInfo executionInfo_;
};

// 任务比较器（用于优先级队列）
struct TaskPriorityComparator {
    bool operator()(const std::shared_ptr<Task>& a, const std::shared_ptr<Task>& b) const {
        // 高优先级在前
        return static_cast<int>(a->getPriority()) < static_cast<int>(b->getPriority());
    }
};

} // namespace openclaw

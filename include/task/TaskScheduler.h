#pragma once

#include "Task.h"
#include "../agent/Agent.h"
#include "../agent/AgentManager.h"
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace openclaw {

// 任务队列
class TaskQueue {
public:
    using TaskPtr = std::shared_ptr<Task>;
    
    TaskQueue(size_t maxSize = 1000);
    ~TaskQueue() = default;
    
    // 禁用拷贝
    TaskQueue(const TaskQueue&) = delete;
    TaskQueue& operator=(const TaskQueue&) = delete;
    
    // 添加设置最大大小的方法
    void setMaxSize(size_t maxSize) { maxSize_ = maxSize; }
    
    // 任务操作
    bool push(TaskPtr task);
    TaskPtr pop();
    bool remove(const std::string& taskId);
    bool contains(const std::string& taskId) const;
    
    // 查询
    size_t size() const;
    bool empty() const;
    size_t maxSize() const { return maxSize_; }
    std::vector<TaskPtr> getAllTasks() const;
    
    // 按优先级获取任务
    TaskPtr getHighestPriorityPendingTask() const;
    
    // 清理
    size_t cleanupCompletedTasks();

private:
    mutable std::mutex mutex_;
    size_t maxSize_;
    
    // 使用优先级比较器的队列
    std::priority_queue<TaskPtr, std::vector<TaskPtr>, TaskPriorityComparator> queue_;
    std::unordered_map<std::string, TaskPtr> taskMap_;
};

// 任务调度策略
enum class SchedulingStrategy {
    FIFO = 0,          // 先进先出
    PRIORITY,        // 优先级优先
    ROUND_ROBIN,     // 轮询
    LOAD_BALANCED    // 负载均衡
};

// 执行策略
class ExecutionStrategy {
public:
    virtual ~ExecutionStrategy() = default;
    
    // 选择要执行的任务
    virtual std::vector<std::shared_ptr<Task>> selectTasksToExecute(
        const TaskQueue& queue,
        const std::vector<Agent::Ptr>& availableAgents) = 0;
    
    // 为任务选择智能体
    virtual Agent::Ptr selectAgentForTask(
        const std::shared_ptr<Task>& task,
        const std::vector<Agent::Ptr>& availableAgents) = 0;
};

// 默认执行策略
class DefaultExecutionStrategy : public ExecutionStrategy {
public:
    std::vector<std::shared_ptr<Task>> selectTasksToExecute(
        const TaskQueue& queue,
        const std::vector<Agent::Ptr>& availableAgents) override;
    
    Agent::Ptr selectAgentForTask(
        const std::shared_ptr<Task>& task,
        const std::vector<Agent::Ptr>& availableAgents) override;
};

// 任务调度器
class TaskScheduler {
public:
    using TaskPtr = std::shared_ptr<Task>;
    using TaskCallback = std::function<void(const TaskPtr&)>;
    
    TaskScheduler(AgentManager& agentManager);
    ~TaskScheduler();
    
    // 禁用拷贝
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
    
    // 配置
    void configure(SchedulingStrategy strategy, size_t maxConcurrentTasks = 10);
    void setTaskQueueMaxSize(size_t maxSize);
    void setExecutionStrategy(std::unique_ptr<ExecutionStrategy> strategy);
    
    // 任务调度
    void scheduleTask(const TaskConfig& config);
    bool cancelTask(const std::string& taskId);
    TaskPtr getTask(const std::string& taskId);
    TaskStatus getTaskStatus(const std::string& taskId);
    
    // 任务查询
    std::vector<TaskPtr> getAllTasks() const;
    std::vector<TaskPtr> getTasksByStatus(TaskStatus status) const;
    std::vector<TaskPtr> getTasksByAgent(const std::string& agentId) const;
    
    // 调度控制
    void start();
    void stop();
    void pause();
    void resume();
    bool isRunning() const;
    
    // 统计监控
    struct SchedulerStats {
        size_t totalTasksScheduled;
        size_t totalTasksCompleted;
        size_t totalTasksFailed;
        size_t totalTasksCancelled;
        size_t currentPendingCount;
        size_t currentRunningCount;
        double averageExecutionTimeMs;
        double taskCompletionRate;
    };
    SchedulerStats getStats() const;
    
    // 回调设置
    void setTaskStartedCallback(TaskCallback callback);
    void setTaskCompletedCallback(TaskCallback callback);
    void setTaskFailedCallback(TaskCallback callback);

private:
    AgentManager& agentManager_;
    TaskQueue taskQueue_{1000};
    SchedulingStrategy strategy_{SchedulingStrategy::PRIORITY};
    size_t maxConcurrentTasks_{10};
    std::unique_ptr<ExecutionStrategy> executionStrategy_;
    
    // 运行状态
    std::atomic<bool> running_{false};
    std::atomic<bool> paused_{false};
    std::thread schedulerThread_;
    
    // 任务状态管理
    mutable std::mutex tasksMutex_;
    std::unordered_map<std::string, TaskPtr> allTasks_;
    std::unordered_set<std::string> runningTasks_;
    std::unordered_map<std::string, std::string> taskAgentMap_; // taskId -> agentId
    
    // 统计
    mutable std::mutex statsMutex_;
    struct Stats {
        size_t totalScheduled{0};
        size_t totalCompleted{0};
        size_t totalFailed{0};
        size_t totalCancelled{0};
        double totalExecutionTimeMs{0};
    } stats_;
    
    // 回调
    TaskCallback taskStartedCallback_;
    TaskCallback taskCompletedCallback_;
    TaskCallback taskFailedCallback_;
    
    // 私有方法
    void schedulerLoop();
    void processSchedulingRound();
    void executeTask(TaskPtr task, Agent::Ptr agent);
    bool canScheduleMoreTasks() const;
    std::vector<Agent::Ptr> getAvailableAgents() const;
    void updateStats(const TaskPtr& task, bool completed);
};

// 任务调度事件
struct TaskScheduledEvent : public Event {
    std::string taskId;
    std::string taskName;
    TaskPriority priority;
    
    TaskScheduledEvent(const std::string& id, const std::string& name, TaskPriority p)
        : Event(EventType::TASK_SCHEDULED), taskId(id), taskName(name), priority(p) {}
};

struct TaskAssignedEvent : public Event {
    std::string taskId;
    std::string agentId;
    TaskType taskType;
    
    TaskAssignedEvent(const std::string& tid, const std::string& aid, TaskType type)
        : Event(EventType::TASK_ASSIGNED), taskId(tid), agentId(aid), taskType(type) {}
};

struct TaskCompletedEvent : public Event {
    std::string taskId;
    std::string agentId;
    std::chrono::milliseconds executionTime;
    bool success;
    
    TaskCompletedEvent(const std::string& tid, const std::string& aid, 
                        std::chrono::milliseconds time, bool ok)
        : Event(EventType::TASK_COMPLETED), taskId(tid), agentId(aid), 
                executionTime(time), success(ok) {}
};

} // namespace openclaw

#include "task/TaskScheduler.h"
#include "logging/Logger.h"
#include "events/EventDispatcher.h"
#include <chrono>

namespace openclaw {

// TaskQueue 实现
TaskQueue::TaskQueue(size_t maxSize) : maxSize_(maxSize) {}

bool TaskQueue::push(TaskPtr task) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (taskMap_.size() >= maxSize_) {
        Logger::getInstance().warning("TaskQueue", "Queue is full, cannot add task: " + task->getId());
        return false;
    }
    
    if (taskMap_.find(task->getId()) != taskMap_.end()) {
        Logger::getInstance().warning("TaskQueue", "Task already exists: " + task->getId());
        return false;
    }
    
    queue_.push(task);
    taskMap_[task->getId()] = task;
    
    return true;
}

TaskQueue::TaskPtr TaskQueue::pop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (queue_.empty()) {
        return nullptr;
    }
    
    auto task = queue_.top();
    queue_.pop();
    taskMap_.erase(task->getId());
    
    return task;
}

bool TaskQueue::remove(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = taskMap_.find(taskId);
    if (it == taskMap_.end()) {
        return false;
    }
    
    // 重建队列（不支持直接删除）
    std::priority_queue<TaskPtr, std::vector<TaskPtr>, TaskPriorityComparator> newQueue;
    while (!queue_.empty()) {
        auto task = queue_.top();
        queue_.pop();
        if (task->getId() != taskId) {
            newQueue.push(task);
        }
    }
    queue_ = std::move(newQueue);
    taskMap_.erase(it);
    
    return true;
}

bool TaskQueue::contains(const std::string& taskId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return taskMap_.find(taskId) != taskMap_.end();
}

size_t TaskQueue::size() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

bool TaskQueue::empty() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.empty();
}

std::vector<TaskQueue::TaskPtr> TaskQueue::getAllTasks() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<TaskPtr> tasks;
    for (const auto& pair : taskMap_) {
        tasks.push_back(pair.second);
    }
    return tasks;
}

TaskQueue::TaskPtr TaskQueue::getHighestPriorityPendingTask() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (queue_.empty()) {
        return nullptr;
    }
    
    for (const auto& pair : taskMap_) {
        if (pair.second->getStatus() == TaskStatus::PENDING) {
            return pair.second;
        }
    }
    
    return nullptr;
}

size_t TaskQueue::cleanupCompletedTasks() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    std::vector<std::string> toRemove;
    
    for (const auto& pair : taskMap_) {
        auto status = pair.second->getStatus();
        if (status == TaskStatus::COMPLETED || 
            status == TaskStatus::FAILED || 
            status == TaskStatus::CANCELLED ||
            status == TaskStatus::TIMEOUT) {
            toRemove.push_back(pair.first);
            count++;
        }
    }
    
    for (const auto& id : toRemove) {
        taskMap_.erase(id);
    }
    
    // 重建队列
    std::priority_queue<TaskPtr, std::vector<TaskPtr>, TaskPriorityComparator> newQueue;
    for (const auto& pair : taskMap_) {
        newQueue.push(pair.second);
    }
    queue_ = std::move(newQueue);
    
    return count;
}

// DefaultExecutionStrategy 实现
std::vector<std::shared_ptr<Task>> DefaultExecutionStrategy::selectTasksToExecute(
    const TaskQueue& queue,
    const std::vector<Agent::Ptr>& availableAgents) {
    
    std::vector<std::shared_ptr<Task>> selectedTasks;
    auto tasks = queue.getAllTasks();
    
    for (const auto& task : tasks) {
        if (task->getStatus() == TaskStatus::PENDING) {
            selectedTasks.push_back(task);
            if (selectedTasks.size() >= availableAgents.size()) {
                break;
            }
        }
    }
    
    return selectedTasks;
}

Agent::Ptr DefaultExecutionStrategy::selectAgentForTask(
    const std::shared_ptr<Task>& task,
    const std::vector<Agent::Ptr>& availableAgents) {
    
    if (availableAgents.empty()) {
        return nullptr;
    }
    
    // 简单的轮询策略
    static size_t roundRobinIndex = 0;
    auto agent = availableAgents[roundRobinIndex % availableAgents.size()];
    roundRobinIndex++;
    
    return agent;
}

// TaskScheduler 实现
TaskScheduler::TaskScheduler(AgentManager& agentManager) 
    : agentManager_(agentManager) {
    executionStrategy_ = std::make_unique<DefaultExecutionStrategy>();
}

TaskScheduler::~TaskScheduler() {
    stop();
}

void TaskScheduler::configure(SchedulingStrategy strategy, size_t maxConcurrentTasks) {
    strategy_ = strategy;
    maxConcurrentTasks_ = maxConcurrentTasks;
}

void TaskScheduler::setTaskQueueMaxSize(size_t maxSize) {
    taskQueue_.setMaxSize(maxSize);
}

void TaskScheduler::setExecutionStrategy(std::unique_ptr<ExecutionStrategy> strategy) {
    executionStrategy_ = std::move(strategy);
}

void TaskScheduler::scheduleTask(const TaskConfig& config) {
    if (!config.validate()) {
        Logger::getInstance().error("TaskScheduler", "Invalid task config");
        return;
    }
    
    auto task = std::make_shared<Task>(config);
    
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        allTasks_[config.id] = task;
    }
    
    if (!taskQueue_.push(task)) {
        Logger::getInstance().error("TaskScheduler", "Failed to queue task: " + config.id);
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        stats_.totalScheduled++;
    }
    
    // 发布调度事件
    EventDispatcher::getInstance().dispatchEvent(EventType::TASK_SCHEDULED);
    
    Logger::getInstance().info("TaskScheduler", 
        "Scheduled task: " + config.id + " (" + config.name + ")");
}

bool TaskScheduler::cancelTask(const std::string& taskId) {
    auto task = getTask(taskId);
    if (!task) {
        return false;
    }
    
    task->markCancelled();
    
    std::lock_guard<std::mutex> lock(tasksMutex_);
    runningTasks_.erase(taskId);
    taskQueue_.remove(taskId);
    
    {
        std::lock_guard<std::mutex> statsLock(statsMutex_);
        stats_.totalCancelled++;
    }
    
    Logger::getInstance().info("TaskScheduler", "Cancelled task: " + taskId);
    
    return true;
}

TaskScheduler::TaskPtr TaskScheduler::getTask(const std::string& taskId) {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    auto it = allTasks_.find(taskId);
    if (it != allTasks_.end()) {
        return it->second;
    }
    return nullptr;
}

TaskStatus TaskScheduler::getTaskStatus(const std::string& taskId) {
    auto task = getTask(taskId);
    if (task) {
        return task->getStatus();
    }
    return TaskStatus::PENDING;
}

std::vector<TaskScheduler::TaskPtr> TaskScheduler::getAllTasks() const {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    std::vector<TaskPtr> tasks;
    for (const auto& pair : allTasks_) {
        tasks.push_back(pair.second);
    }
    return tasks;
}

std::vector<TaskScheduler::TaskPtr> TaskScheduler::getTasksByStatus(TaskStatus status) const {
    std::vector<TaskPtr> result;
    auto tasks = getAllTasks();
    for (const auto& task : tasks) {
        if (task->getStatus() == status) {
            result.push_back(task);
        }
    }
    return result;
}

std::vector<TaskScheduler::TaskPtr> TaskScheduler::getTasksByAgent(const std::string& agentId) const {
    std::vector<TaskPtr> result;
    std::lock_guard<std::mutex> lock(tasksMutex_);
    
    for (const auto& pair : allTasks_) {
        if (pair.second->getExecutionInfo().agentId == agentId) {
            result.push_back(pair.second);
        }
    }
    
    return result;
}

void TaskScheduler::start() {
    if (running_.exchange(true)) {
        return; // 已在运行
    }
    
    schedulerThread_ = std::thread(&TaskScheduler::schedulerLoop, this);
    
    Logger::getInstance().info("TaskScheduler", "Task scheduler started");
}

void TaskScheduler::stop() {
    if (!running_.exchange(false)) {
        return; // 已停止
    }
    
    if (schedulerThread_.joinable()) {
        schedulerThread_.join();
    }
    
    Logger::getInstance().info("TaskScheduler", "Task scheduler stopped");
}

void TaskScheduler::pause() {
    paused_ = true;
    Logger::getInstance().info("TaskScheduler", "Task scheduler paused");
}

void TaskScheduler::resume() {
    paused_ = false;
    Logger::getInstance().info("TaskScheduler", "Task scheduler resumed");
}

bool TaskScheduler::isRunning() const {
    return running_.load();
}

TaskScheduler::SchedulerStats TaskScheduler::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    SchedulerStats stats;
    stats.totalTasksScheduled = stats_.totalScheduled;
    stats.totalTasksCompleted = stats_.totalCompleted;
    stats.totalTasksFailed = stats_.totalFailed;
    stats.totalTasksCancelled = stats_.totalCancelled;
    
    {
        std::lock_guard<std::mutex> tasksLock(tasksMutex_);
        stats.currentPendingCount = taskQueue_.size();
        stats.currentRunningCount = runningTasks_.size();
    }
    
    if (stats_.totalCompleted > 0) {
        stats.averageExecutionTimeMs = stats_.totalExecutionTimeMs / stats_.totalCompleted;
    }
    
    if (stats_.totalScheduled > 0) {
        stats.taskCompletionRate = static_cast<double>(stats_.totalCompleted) / stats_.totalScheduled * 100.0;
    }
    
    return stats;
}

void TaskScheduler::setTaskStartedCallback(TaskCallback callback) {
    taskStartedCallback_ = callback;
}

void TaskScheduler::setTaskCompletedCallback(TaskCallback callback) {
    taskCompletedCallback_ = callback;
}

void TaskScheduler::setTaskFailedCallback(TaskCallback callback) {
    taskFailedCallback_ = callback;
}

void TaskScheduler::schedulerLoop() {
    while (running_) {
        if (!paused_) {
            processSchedulingRound();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void TaskScheduler::processSchedulingRound() {
    if (paused_ || !canScheduleMoreTasks()) {
        return;
    }
    
    auto availableAgents = getAvailableAgents();
    if (availableAgents.empty()) {
        return;
    }
    
    auto selectedTasks = executionStrategy_->selectTasksToExecute(taskQueue_, availableAgents);
    
    for (auto& task : selectedTasks) {
        if (!canScheduleMoreTasks()) {
            break;
        }
        
        auto agent = executionStrategy_->selectAgentForTask(task, availableAgents);
        if (!agent) {
            continue;
        }
        
        executeTask(task, agent);
    }
}

void TaskScheduler::executeTask(TaskPtr task, Agent::Ptr agent) {
    task->markStarted();
    
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        runningTasks_.insert(task->getId());
        taskAgentMap_[task->getId()] = agent->getId();
    }
    
    if (taskStartedCallback_) {
        taskStartedCallback_(task);
    }
    
    // 简化实现：这里应该是异步执行任务
    // 实际实现应该启动线程或使用线程池
    task->markCompleted(TaskResult());
    
    {
        std::lock_guard<std::mutex> lock(tasksMutex_);
        runningTasks_.erase(task->getId());
    }
    
    updateStats(task, true);
    
    if (taskCompletedCallback_) {
        taskCompletedCallback_(task);
    }
}

bool TaskScheduler::canScheduleMoreTasks() const {
    std::lock_guard<std::mutex> lock(tasksMutex_);
    return runningTasks_.size() < maxConcurrentTasks_;
}

std::vector<Agent::Ptr> TaskScheduler::getAvailableAgents() const {
    std::vector<Agent::Ptr> agents;
    // 简化实现：获取所有运行中的智能体
    return agents;
}

void TaskScheduler::updateStats(const TaskPtr& task, bool completed) {
    std::lock_guard<std::mutex> lock(statsMutex_);
    
    if (completed) {
        stats_.totalCompleted++;
        if (task->getExecutionInfo().elapsedTime.count() > 0) {
            stats_.totalExecutionTimeMs += task->getExecutionInfo().elapsedTime.count();
        }
    } else {
        stats_.totalFailed++;
    }
}

} // namespace openclaw
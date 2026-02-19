#include "task/Task.h"
#include <algorithm>

namespace openclaw {

bool TaskConfig::validate() const {
    if (id.empty()) return false;
    if (name.empty()) return false;
    if (type == TaskType::UNKNOWN) return false;
    return true;
}

Task::Task(const TaskConfig& config) : config_(config) {
    executionInfo_.taskId = config.id;
    status_ = TaskStatus::PENDING;
}

void Task::setStatus(TaskStatus status) {
    status_ = status;
    executionInfo_.status = status;
}

void Task::setAssignedAgent(const std::string& agentId) {
    config_.assignedAgentId = agentId;
    executionInfo_.agentId = agentId;
}

void Task::setProgress(double progress) {
    executionInfo_.progress = std::max(0.0, std::min(100.0, progress));
}

void Task::setPhase(const std::string& phase) {
    executionInfo_.currentPhase = phase;
}

void Task::markStarted() {
    setStatus(TaskStatus::RUNNING);
    executionInfo_.startTime = std::chrono::system_clock::now();
}

void Task::markCompleted(const TaskResult& /*result*/) {
    setStatus(TaskStatus::COMPLETED);
    executionInfo_.endTime = std::chrono::system_clock::now();
    executionInfo_.elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        executionInfo_.endTime - executionInfo_.startTime);
    executionInfo_.progress = 100.0;
}

void Task::markFailed(const std::string& /*error*/) {
    setStatus(TaskStatus::FAILED);
    executionInfo_.endTime = std::chrono::system_clock::now();
    executionInfo_.elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        executionInfo_.endTime - executionInfo_.startTime);
    executionInfo_.retryCount++;
}

void Task::markCancelled() {
    setStatus(TaskStatus::CANCELLED);
    executionInfo_.endTime = std::chrono::system_clock::now();
}

void Task::markTimeout() {
    setStatus(TaskStatus::TIMEOUT);
    executionInfo_.endTime = std::chrono::system_clock::now();
}

bool Task::areDependenciesMet(const std::vector<std::string>& completedTasks) const {
    for (const auto& dep : config_.dependencies) {
        if (std::find(completedTasks.begin(), completedTasks.end(), dep) == completedTasks.end()) {
            return false;
        }
    }
    return true;
}

bool Task::canResourceRequirementsBeMet(const ResourceRequirements& available) const {
    return config_.resourceRequirements.memoryMB <= available.memoryMB &&
           config_.resourceRequirements.cpuCores <= available.cpuCores;
}

} // namespace openclaw

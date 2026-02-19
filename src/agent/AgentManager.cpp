#include "agent/AgentManager.h"
#include "logging/Logger.h"
#include "events/EventDispatcher.h"
#include <thread>

namespace openclaw {

AgentManager::AgentManager() {}

Agent::Ptr AgentManager::createAgent(const AgentConfig& config) {
    if (!config.validate()) {
        Logger::getInstance().error("AgentManager", "Invalid agent config");
        return nullptr;
    }
    
    std::lock_guard<std::mutex> lock(agentsMutex_);
    
    // 检查ID是否已存在
    if (agents_.find(config.id) != agents_.end()) {
        Logger::getInstance().error("AgentManager", 
            "Agent with ID " + config.id + " already exists");
        return nullptr;
    }
    
    // 使用工厂创建智能体
    auto agent = AgentFactory::getInstance().createAgent(config);
    if (!agent) {
        Logger::getInstance().error("AgentManager", 
            "Failed to create agent of type " + Agent::typeToString(config.type));
        return nullptr;
    }
    
    // 添加到管理器
    agents_[config.id] = agent;
    
    // 发布创建事件
    // 简化的事件发送
    EventDispatcher::getInstance().dispatchEvent(EventType::AGENT_CREATED);
    
    Logger::getInstance().info("AgentManager", 
        "Created agent: " + config.id + " (" + config.name + ")");
    
    return agent;
}

bool AgentManager::deleteAgent(const std::string& agentId) {
    std::lock_guard<std::mutex> lock(agentsMutex_);
    
    auto it = agents_.find(agentId);
    if (it == agents_.end()) {
        Logger::getInstance().warning("AgentManager", 
            "Agent not found: " + agentId);
        return false;
    }
    
    // 停止智能体
    auto agent = it->second;
    if (agent->getStatus() == AgentStatus::RUNNING) {
        agent->stop();
    }
    
    // 发布停止事件
    EventDispatcher::getInstance().dispatchEvent(EventType::AGENT_STOPPED);
    
    // 从管理中移除
    agents_.erase(it);
    
    Logger::getInstance().info("AgentManager", 
        "Deleted agent: " + agentId);
    
    return true;
}

Agent::Ptr AgentManager::getAgent(const std::string& agentId) {
    std::lock_guard<std::mutex> lock(agentsMutex_);
    
    auto it = agents_.find(agentId);
    if (it != agents_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<Agent::Ptr> AgentManager::listAgents() const {
    std::lock_guard<std::mutex> lock(agentsMutex_);
    
    std::vector<Agent::Ptr> result;
    for (const auto& pair : agents_) {
        result.push_back(pair.second);
    }
    return result;
}

std::vector<Agent::Ptr> AgentManager::listAgentsByType(AgentType type) const {
    std::lock_guard<std::mutex> lock(agentsMutex_);
    
    std::vector<Agent::Ptr> result;
    for (const auto& pair : agents_) {
        if (pair.second->getType() == type) {
            result.push_back(pair.second);
        }
    }
    return result;
}

std::vector<Agent::Ptr> AgentManager::listAgentsByStatus(AgentStatus status) const {
    std::lock_guard<std::mutex> lock(agentsMutex_);
    
    std::vector<Agent::Ptr> result;
    for (const auto& pair : agents_) {
        if (pair.second->getStatus() == status) {
            result.push_back(pair.second);
        }
    }
    return result;
}

AgentStatus AgentManager::getAgentStatus(const std::string& agentId) {
    auto agent = getAgent(agentId);
    if (agent) {
        return agent->getStatus();
    }
    return AgentStatus::UNKNOWN;
}

bool AgentManager::updateAgentConfig(const std::string& agentId, const AgentConfig& config) {
    auto agent = getAgent(agentId);
    if (!agent) {
        Logger::getInstance().warning("AgentManager", 
            "Cannot update config, agent not found: " + agentId);
        return false;
    }
    return agent->updateConfig(config);
}

void AgentManager::startAllAgents() {
    auto agents = listAgents();
    for (auto& agent : agents) {
        if (agent->getStatus() == AgentStatus::STOPPED) {
            agent->start();
        }
    }
}

void AgentManager::stopAllAgents() {
    auto agents = listAgents();
    for (auto& agent : agents) {
        if (agent->getStatus() == AgentStatus::RUNNING ||
            agent->getStatus() == AgentStatus::PAUSED) {
            agent->stop();
        }
    }
}

void AgentManager::pauseAllAgents() {
    auto agents = listAgents();
    for (auto& agent : agents) {
        if (agent->getStatus() == AgentStatus::RUNNING) {
            agent->pause();
        }
    }
}

void AgentManager::resumeAllAgents() {
    auto agents = listAgents();
    for (auto& agent : agents) {
        if (agent->getStatus() == AgentStatus::PAUSED) {
            agent->resume();
        }
    }
}

size_t AgentManager::getAgentCount() const {
    std::lock_guard<std::mutex> lock(agentsMutex_);
    return agents_.size();
}

size_t AgentManager::getAgentCountByStatus(AgentStatus status) const {
    return listAgentsByStatus(status).size();
}

std::unordered_map<AgentStatus, size_t> AgentManager::getAgentStatusDistribution() const {
    std::unordered_map<AgentStatus, size_t> distribution;
    
    for (int i = 0; i <= static_cast<int>(AgentStatus::ERROR); ++i) {
        AgentStatus status = static_cast<AgentStatus>(i);
        distribution[status] = getAgentCountByStatus(status);
    }
    
    return distribution;
}

std::vector<std::string> AgentManager::getUnhealthyAgents() const {
    std::vector<std::string> result;
    auto agents = listAgents();
    
    for (const auto& agent : agents) {
        if (!agent->isHealthy()) {
            result.push_back(agent->getId());
        }
    }
    
    return result;
}

void AgentManager::setStatusCallback(AgentStatusCallback callback) {
    statusCallback_ = callback;
}

void AgentManager::notifyStatusChange(const std::string& agentId, 
                                      AgentStatus oldStatus, 
                                      AgentStatus newStatus) {
    if (statusCallback_) {
        statusCallback_(agentId, newStatus);
    }
    
    EventDispatcher::getInstance().dispatchEvent(EventType::AGENT_STATUS_CHANGED);
}

size_t AgentManager::cleanupStoppedAgents() {
    std::lock_guard<std::mutex> lock(agentsMutex_);
    
    size_t count = 0;
    for (auto it = agents_.begin(); it != agents_.end();) {
        if (it->second->getStatus() == AgentStatus::STOPPED) {
            it = agents_.erase(it);
            count++;
        } else {
            ++it;
        }
    }
    
    Logger::getInstance().info("AgentManager", 
        "Cleaned up " + std::to_string(count) + " stopped agents");
    
    return count;
}

// AgentMonitor 实现
AgentMonitor::AgentMonitor(AgentManager& manager) 
    : manager_(manager) {}

AgentMonitor::~AgentMonitor() {
    stopMonitoring();
}

AgentMonitor::MonitorStats AgentMonitor::getStats() const {
    MonitorStats stats;
    stats.totalAgents = manager_.getAgentCount();
    stats.runningAgents = manager_.getAgentCountByStatus(AgentStatus::RUNNING);
    stats.stoppedAgents = manager_.getAgentCountByStatus(AgentStatus::STOPPED);
    stats.errorAgents = manager_.getAgentCountByStatus(AgentStatus::ERROR);
    stats.unhealthyAgents = manager_.getUnhealthyAgents().size();
    
    return stats;
}

void AgentMonitor::startMonitoring(size_t intervalSeconds) {
    if (monitoring_.exchange(true)) {
        return; // 已经在监控
    }
    
    intervalSeconds_ = intervalSeconds;
    monitorThread_ = std::thread(&AgentMonitor::monitorLoop, this);
    
    Logger::getInstance().info("AgentMonitor", 
        "Started monitoring with interval " + std::to_string(intervalSeconds) + "s");
}

void AgentMonitor::stopMonitoring() {
    if (!monitoring_.exchange(false)) {
        return; // 已经停止
    }
    
    if (monitorThread_.joinable()) {
        monitorThread_.join();
    }
    
    Logger::getInstance().info("AgentMonitor", "Stopped monitoring");
}

bool AgentMonitor::isMonitoring() const {
    return monitoring_.load();
}

void AgentMonitor::monitorLoop() {
    while (monitoring_) {
        // 获取统计
        auto stats = getStats();
        
        // 检查不健康智能体
        auto unhealthy = manager_.getUnhealthyAgents();
        if (!unhealthy.empty()) {
            Logger::getInstance().warning("AgentMonitor", "Unhealthy agents detected");
        }
        
        // 休眠指定的时间间隔
        std::this_thread::sleep_for(std::chrono::seconds(intervalSeconds_));
    }
}

} // namespace openclaw
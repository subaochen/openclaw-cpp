#pragma once

#include "Agent.h"
#include <mutex>
#include <unordered_map>
#include <thread>
#include <vector>
#include <functional>

namespace openclaw {

// 智能体管理器
class AgentManager {
public:
    using AgentPtr = Agent::Ptr;
    using AgentStatusCallback = std::function<void(const std::string&, AgentStatus)>;
    
    AgentManager();
    ~AgentManager() = default;
    
    // 禁用拷贝
    AgentManager(const AgentManager&) = delete;
    AgentManager& operator=(const AgentManager&) = delete;
    
    // 智能体生命周期管理
    AgentPtr createAgent(const AgentConfig& config);
    bool deleteAgent(const std::string& agentId);
    
    // 智能体查询
    AgentPtr getAgent(const std::string& agentId);
    std::vector<AgentPtr> listAgents() const;
    std::vector<AgentPtr> listAgentsByType(AgentType type) const;
    std::vector<AgentPtr> listAgentsByStatus(AgentStatus status) const;
    
    // 智能体状态管理
    AgentStatus getAgentStatus(const std::string& agentId);
    bool updateAgentConfig(const std::string& agentId, const AgentConfig& config);
    
    // 批量操作
    void startAllAgents();
    void stopAllAgents();
    void pauseAllAgents();
    void resumeAllAgents();
    
    // 监控与统计
    size_t getAgentCount() const;
    size_t getAgentCountByStatus(AgentStatus status) const;
    std::unordered_map<AgentStatus, size_t> getAgentStatusDistribution() const;
    
    // 健康检查
    std::vector<std::string> getUnhealthyAgents() const;
    
    // 状态变更回调
    void setStatusCallback(AgentStatusCallback callback);
    
    // 清理已停止的智能体
    size_t cleanupStoppedAgents();

private:
    mutable std::mutex agentsMutex_;
    std::unordered_map<std::string, AgentPtr> agents_;
    AgentStatusCallback statusCallback_;
    
    // 通知状态变更
    void notifyStatusChange(const std::string& agentId, AgentStatus oldStatus, AgentStatus newStatus);
};

// 智能体监控器
class AgentMonitor {
public:
    AgentMonitor(AgentManager& manager);
    ~AgentMonitor();
    
    void startMonitoring(size_t intervalSeconds);
    void stopMonitoring();
    bool isMonitoring() const;
    
    // 获取监控统计
    struct MonitorStats {
        size_t totalAgents;
        size_t runningAgents;
        size_t stoppedAgents;
        size_t errorAgents;
        size_t unhealthyAgents;
    };
    MonitorStats getStats() const;

private:
    AgentManager& manager_;
    std::atomic<bool> monitoring_{false};
    std::thread monitorThread_; // 添加线程支持
    size_t intervalSeconds_{60};
    
    void monitorLoop();
};

} // namespace openclaw

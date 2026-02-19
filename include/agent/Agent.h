#pragma once

#include <memory>
#include <string>
#include <atomic>
#include <functional>
#include <unordered_map>
#include <vector>
#include "../events/Event.h"

namespace openclaw {

// 智能体类型枚举
enum class AgentType {
    UNKNOWN = 0,
    DEVELOPER,      // 开发智能体
    TESTER,         // 测试智能体
    ARCHITECT,      // 架构师AI
    PROJECT_MANAGER // 项目管理器代理
};

// 智能体状态枚举
enum class AgentStatus {
    UNKNOWN = 0,
    STOPPED,
    STARTING,
    RUNNING,
    PAUSED,
    STOPPING,
    ERROR
};

// 智能体配置
struct AgentConfig {
    std::string id;                                    // 智能体ID
    AgentType type{AgentType::UNKNOWN};                 // 智能体类型
    std::string name;                                  // 智能体名称
    std::string description;                            // 智能体描述
    std::unordered_map<std::string, std::string> properties; // 属性配置
    
    // 资源限制
    struct ResourceLimits {
        size_t maxMemoryMB{512};      // 最大内存使用
        size_t maxThreads{4};          // 最大线程数
        double maxCpuUsage{80.0};     // 最大CPU使用率(%)
    } resourceLimits;
    
    // 验证配置
    bool validate() const;
    std::string toJson() const;
    static AgentConfig fromJson(const std::string& json);
};

// 任务前向声明
class Task;
class TaskResult;

// 智能体接口
class Agent {
public:
    using Ptr = std::shared_ptr<Agent>;
    using TaskHandler = std::function<void(const Task&)>;
    
    Agent(const AgentConfig& config);
    virtual ~Agent() = default;
    
    // 禁用拷贝
    Agent(const Agent&) = delete;
    Agent& operator=(const Agent&) = delete;
    
    // 生命周期管理
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void resume() = 0;
    
    // 任务执行
    virtual std::shared_ptr<TaskResult> executeTask(const Task& task) = 0;
    
    // 状态查询
    AgentStatus getStatus() const { return status_.load(); }
    std::string getId() const { return config_.id; }
    AgentType getType() const { return config_.type; }
    std::string getName() const { return config_.name; }
    AgentConfig getConfig() const { return config_; }
    
    // 配置更新
    virtual bool updateConfig(const AgentConfig& config);
    
    // 健康检查
    virtual bool isHealthy() const;
    
    // 获取状态字符串
    static std::string statusToString(AgentStatus status);
    static std::string typeToString(AgentType type);

protected:
    AgentConfig config_;
    std::atomic<AgentStatus> status_{AgentStatus::UNKNOWN};
    std::atomic<bool> healthy_{true};
};

// 智能体创建工厂
class AgentFactory {
public:
    using Creator = std::function<Agent::Ptr(const AgentConfig&)>;
    
    static AgentFactory& getInstance();
    
    // 注册智能体类型
    void registerAgent(AgentType type, Creator creator);
    
    // 创建智能体
    Agent::Ptr createAgent(const AgentConfig& config);
    
    // 检查类型是否已注册
    bool isRegistered(AgentType type) const;

private:
    AgentFactory() = default;
    std::unordered_map<AgentType, Creator> creators_;
};

// 智能体事件
struct AgentCreatedEvent : public Event {
    std::string agentId;
    AgentType agentType;
    
    AgentCreatedEvent(const std::string& id, AgentType type)
        : Event(EventType::AGENT_CREATED), agentId(id), agentType(type) {}
};

struct AgentStartedEvent : public Event {
    std::string agentId;
    
    AgentStartedEvent(const std::string& id)
        : Event(EventType::AGENT_STARTED), agentId(id) {}
};

struct AgentStoppedEvent : public Event {
    std::string agentId;
    std::string reason;
    
    AgentStoppedEvent(const std::string& id, const std::string& r = "")
        : Event(EventType::AGENT_STOPPED), agentId(id), reason(r) {}
};

struct AgentStatusChangedEvent : public Event {
    std::string agentId;
    AgentStatus oldStatus;
    AgentStatus newStatus;
    
    AgentStatusChangedEvent(const std::string& id, AgentStatus oldS, AgentStatus newS)
        : Event(EventType::AGENT_STATUS_CHANGED), agentId(id), oldStatus(oldS), newStatus(newS) {}
};

} // namespace openclaw

// 事件类型扩展（在Agent头文件中定义，用于Agent相关事件）
enum class EventType {
    // ... 原有事件类型 ...
    
    // AI代理管理事件
    AGENT_CREATED,
    AGENT_STARTED,
    AGENT_STOPPED,
    AGENT_STATUS_CHANGED,
    AGENT_ERROR,
    
    TASK_ASSIGNED,
    TASK_SCHEDULED,
    TASK_COMPLETED,
    TASK_FAILED,
    
    PROJECT_CREATED,
    PROJECT_STATUS_CHANGED,
    
    RESOURCE_ALLOCATED,
    RESOURCE_RELEASED,
    
    MESSAGE_RECEIVED,
    MESSAGE_SENT
};

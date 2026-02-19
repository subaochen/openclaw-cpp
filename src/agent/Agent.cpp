#include "agent/Agent.h"
#include <sstream>

namespace openclaw {

// AgentConfig 验证
bool AgentConfig::validate() const {
    if (id.empty()) return false;
    if (type == AgentType::UNKNOWN) return false;
    if (name.empty()) return false;
    return true;
}

std::string AgentConfig::toJson() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"id\":\"" << id << "\",";
    oss << "\"type\":" << static_cast<int>(type) << ",";
    oss << "\"name\":\"" << name << "\",";
    oss << "\"description\":\"" << description << "\",";
    oss << "\"maxMemoryMB\":" << resourceLimits.maxMemoryMB << ",";
    oss << "\"maxThreads\":" << resourceLimits.maxThreads << ",";
    oss << "\"maxCpuUsage\":" << resourceLimits.maxCpuUsage;
    oss << "}";
    return oss.str();
}

AgentConfig AgentConfig::fromJson(const std::string& json) {
    AgentConfig config;
    // 简化实现：实际应使用 JSON 解析库
    return config;
}

// Agent 实现
Agent::Agent(const AgentConfig& config) : config_(config) {
    status_ = AgentStatus::STOPPED;
}

bool Agent::updateConfig(const AgentConfig& config) {
    if (!config.validate()) {
        return false;
    }
    config_ = config;
    return true;
}

bool Agent::isHealthy() const {
    return healthy_.load() && status_ == AgentStatus::RUNNING;
}

std::string Agent::statusToString(AgentStatus status) {
    switch (status) {
        case AgentStatus::UNKNOWN: return "UNKNOWN";
        case AgentStatus::STOPPED: return "STOPPED";
        case AgentStatus::STARTING: return "STARTING";
        case AgentStatus::RUNNING: return "RUNNING";
        case AgentStatus::PAUSED: return "PAUSED";
        case AgentStatus::STOPPING: return "STOPPING";
        case AgentStatus::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

std::string Agent::typeToString(AgentType type) {
    switch (type) {
        case AgentType::UNKNOWN: return "UNKNOWN";
        case AgentType::DEVELOPER: return "DEVELOPER";
        case AgentType::TESTER: return "TESTER";
        case AgentType::ARCHITECT: return "ARCHITECT";
        case AgentType::PROJECT_MANAGER: return "PROJECT_MANAGER";
        default: return "UNKNOWN";
    }
}

// AgentFactory 实现
AgentFactory& AgentFactory::getInstance() {
    static AgentFactory instance;
    return instance;
}

void AgentFactory::registerAgent(AgentType type, Creator creator) {
    creators_[type] = creator;
}

Agent::Ptr AgentFactory::createAgent(const AgentConfig& config) {
    auto it = creators_.find(config.type);
    if (it != creators_.end()) {
        return it->second(config);
    }
    return nullptr;
}

bool AgentFactory::isRegistered(AgentType type) const {
    return creators_.find(type) != creators_.end();
}

} // namespace openclaw

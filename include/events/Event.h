#pragma once

#include <memory>
#include <string>
#include <chrono>

namespace openclaw {

// 事件类型
enum class EventType {
    UNKNOWN = 0,
    
    // 系统事件
    SYSTEM_STARTUP,
    SYSTEM_SHUTDOWN,
    CONFIG_CHANGED,
    COMMUNICATION_MESSAGE,
    TEST_PASSED,
    TEST_FAILED,
    ARCHITECTURE_REVIEW,
    CODE_REVIEW,
    PERFORMANCE_MONITOR,
    LOG_MESSAGE,
    ERROR_OCCURRED,
    WARNING_OCCURRED,
    INFO_MESSAGE,
    
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

// 基础事件类
class Event {
public:
    using Ptr = std::shared_ptr<Event>;
    
    Event() : type_(EventType::UNKNOWN), timestamp_(std::chrono::system_clock::now()) {}
    explicit Event(EventType type) : type_(type), timestamp_(std::chrono::system_clock::now()) {}
    virtual ~Event() = default;
    
    EventType getType() const { return type_; }
    std::chrono::system_clock::time_point getTimestamp() const { return timestamp_; }
    
    virtual std::string toString() const {
        return "Event[type=" + std::to_string(static_cast<int>(type_)) + "]";
    }
    
    virtual std::string getSource() const { return "system"; }

private:
    EventType type_;
    std::chrono::system_clock::time_point timestamp_;
};

} // namespace openclaw

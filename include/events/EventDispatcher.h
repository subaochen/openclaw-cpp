#ifndef OPENCLAW_EVENT_DISPATCHER_H
#define OPENCLAW_EVENT_DISPATCHER_H

#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <atomic>
#include <string>

namespace openclaw {

// 事件类型定义
enum class EventType {
    SYSTEM_STARTUP,
    SYSTEM_SHUTDOWN,
    CONFIG_CHANGED,
    COMMUNICATION_MESSAGE,
    TASK_COMPLETED,
    TASK_FAILED,
    TEST_PASSED,
    TEST_FAILED,
    ARCHITECTURE_REVIEW,
    CODE_REVIEW,
    PERFORMANCE_MONITOR,
    LOG_MESSAGE,
    ERROR_OCCURRED,
    WARNING_OCCURRED,
    INFO_MESSAGE
};

// 事件数据结构
struct Event {
    EventType type;
    std::string source;
    std::string timestamp;
    std::string data;
    int priority;

    Event() 
        : type(EventType::INFO_MESSAGE), 
          source("unknown"), 
          timestamp(""), 
          data(""), 
          priority(0) {}
    
    Event(EventType t, const std::string& src, const std::string& ts, const std::string& d, int prio = 0)
        : type(t), 
          source(src), 
          timestamp(ts), 
          data(d), 
          priority(prio) {}
};

// 事件处理函数类型
using EventHandler = std::function<void(const Event&)>;

// 事件调度器
class EventDispatcher {
public:
    // 获取单例实例
    static EventDispatcher& getInstance();

    // 注册事件处理函数
    void registerEventHandler(EventType type, EventHandler handler);

    // 注销事件处理函数
    void unregisterEventHandler(EventType type, EventHandler handler);

    // 发送事件
    void dispatchEvent(const Event& event);

    // 发送简单事件
    void dispatchEvent(EventType type, const std::string& source, const std::string& data, int priority = 0);

    // 获取事件统计信息
    std::unordered_map<EventType, int> getEventStatistics() const;

    // 获取活跃的事件处理函数数量
    int getActiveEventHandlerCount() const;

    // 清除所有事件处理函数
    void clearAllEventHandlers();

    // 启用/禁用事件调度
    void setEventDispatchEnabled(bool enabled);

    // 检查事件调度是否启用
    bool isEventDispatchEnabled() const;

    // 获取事件队列大小
    size_t getEventQueueSize() const;

private:
    EventDispatcher();
    ~EventDispatcher();

    // 禁止复制和移动
    EventDispatcher(const EventDispatcher&) = delete;
    EventDispatcher& operator=(const EventDispatcher&) = delete;
    EventDispatcher(EventDispatcher&&) = delete;
    EventDispatcher& operator=(EventDispatcher&&) = delete;

    // 内部处理事件
    void processEvent(const Event& event);

    // 生成时间戳
    std::string generateTimestamp() const;

    // 事件类型到处理函数的映射
    std::unordered_map<EventType, std::vector<EventHandler>> eventHandlers_;
    
    // 互斥锁
    mutable std::mutex mutex_;
    
    // 事件统计
    std::unordered_map<EventType, int> eventStatistics_;
    
    // 事件调度启用状态
    std::atomic<bool> dispatchEnabled_;
    
    // 事件队列大小
    std::atomic<size_t> eventQueueSize_;
};

} // namespace openclaw

#endif // OPENCLAW_EVENT_DISPATCHER_H
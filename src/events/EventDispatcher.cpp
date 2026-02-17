#include "events/EventDispatcher.h"
#include <ctime>
#include <sstream>
#include <iomanip>

namespace openclaw {

EventDispatcher::EventDispatcher()
    : dispatchEnabled_(true),
      eventQueueSize_(0) {
    // 初始化事件统计
    eventStatistics_ = {
        {EventType::SYSTEM_STARTUP, 0},
        {EventType::SYSTEM_SHUTDOWN, 0},
        {EventType::CONFIG_CHANGED, 0},
        {EventType::COMMUNICATION_MESSAGE, 0},
        {EventType::TASK_COMPLETED, 0},
        {EventType::TASK_FAILED, 0},
        {EventType::TEST_PASSED, 0},
        {EventType::TEST_FAILED, 0},
        {EventType::ARCHITECTURE_REVIEW, 0},
        {EventType::CODE_REVIEW, 0},
        {EventType::PERFORMANCE_MONITOR, 0},
        {EventType::LOG_MESSAGE, 0},
        {EventType::ERROR_OCCURRED, 0},
        {EventType::WARNING_OCCURRED, 0},
        {EventType::INFO_MESSAGE, 0}
    };
}

EventDispatcher::~EventDispatcher() {
    clearAllEventHandlers();
}

EventDispatcher& EventDispatcher::getInstance() {
    static EventDispatcher instance;
    return instance;
}

void EventDispatcher::registerEventHandler(EventType type, EventHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    eventHandlers_[type].push_back(handler);
}

void EventDispatcher::unregisterEventHandler(EventType type, EventHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = eventHandlers_.find(type);
    if (it != eventHandlers_.end()) {
        auto& handlers = it->second;
        handlers.erase(std::remove(handlers.begin(), handlers.end(), handler), handlers.end());
    }
}

void EventDispatcher::dispatchEvent(const Event& event) {
    if (!dispatchEnabled_) {
        return;
    }

    eventQueueSize_++;
    
    // 更新事件统计
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = eventStatistics_.find(event.type);
        if (it != eventStatistics_.end()) {
            it->second++;
        } else {
            eventStatistics_[event.type] = 1;
        }
    }

    // 处理事件
    processEvent(event);
    
    eventQueueSize_--;
}

void EventDispatcher::dispatchEvent(EventType type, const std::string& source, const std::string& data, int priority) {
    Event event(type, source, generateTimestamp(), data, priority);
    dispatchEvent(event);
}

void EventDispatcher::processEvent(const Event& event) {
    std::vector<EventHandler> handlersToCall;
    
    // 获取需要调用的事件处理函数
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = eventHandlers_.find(event.type);
        if (it != eventHandlers_.end()) {
            handlersToCall = it->second;
        }
    }

    // 调用事件处理函数
    for (auto& handler : handlersToCall) {
        try {
            handler(event);
        } catch (const std::exception& e) {
            // 处理事件处理函数的异常
            Event errorEvent(
                EventType::ERROR_OCCURRED,
                "EventDispatcher",
                generateTimestamp(),
                "Event handler error: " + std::string(e.what())
            );
            dispatchEvent(errorEvent);
        } catch (...) {
            // 处理未知异常
            Event errorEvent(
                EventType::ERROR_OCCURRED,
                "EventDispatcher",
                generateTimestamp(),
                "Unknown event handler error"
            );
            dispatchEvent(errorEvent);
        }
    }
}

std::unordered_map<EventType, int> EventDispatcher::getEventStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return eventStatistics_;
}

int EventDispatcher::getActiveEventHandlerCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    int count = 0;
    for (const auto& pair : eventHandlers_) {
        count += pair.second.size();
    }
    return count;
}

void EventDispatcher::clearAllEventHandlers() {
    std::lock_guard<std::mutex> lock(mutex_);
    eventHandlers_.clear();
}

void EventDispatcher::setEventDispatchEnabled(bool enabled) {
    dispatchEnabled_ = enabled;
}

bool EventDispatcher::isEventDispatchEnabled() const {
    return dispatchEnabled_;
}

size_t EventDispatcher::getEventQueueSize() const {
    return eventQueueSize_;
}

std::string EventDispatcher::generateTimestamp() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

} // namespace openclaw
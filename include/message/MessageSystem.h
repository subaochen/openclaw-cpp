#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <atomic>
#include <chrono>
#include <queue>
#include <thread>
#include <condition_variable>
#include "../events/EventDispatcher.h"

namespace openclaw {

// 消息类型枚举
enum class MessageType {
    UNKNOWN = 0,
    COMMAND,        // 命令消息
    EVENT,          // 事件消息
    STATUS,         // 状态消息
    DATA,           // 数据消息
    HEARTBEAT,      // 心跳消息
    ERROR           // 错误消息
};

// 消息优先级
enum class MessagePriority {
    LOW = 0,
    NORMAL,
    HIGH,
    URGENT
};

// 消息结构
class Message {
public:
    using Ptr = std::shared_ptr<Message>;
    
    Message();
    explicit Message(MessageType type);
    
    std::string id;
    std::string from;
    std::string to;
    MessageType type{MessageType::UNKNOWN};
    MessagePriority priority{MessagePriority::NORMAL};
    std::string content;
    std::unordered_map<std::string, std::string> headers;
    std::chrono::system_clock::time_point timestamp;
    
    bool isBroadcast() const { return to.empty(); }
    std::string toJson() const;
    static Message fromJson(const std::string& json);
};

// 消息系统 - 负责消息的发送、接收、路由和订阅
class MessageSystem {
public:
    using Ptr = std::shared_ptr<MessageSystem>;
    using MessageHandler = std::function<void(const Message&)>;
    using SubscriptionId = std::string;
    
    // 单例模式
    static MessageSystem& getInstance();
    
    // 初始化
    void initialize(std::shared_ptr<EventDispatcher> eventDispatcher);
    
    // 启动/停止
    void start();
    void stop();
    
    // 消息发送
    bool sendMessage(const Message& message);
    bool sendMessage(const std::string& sender, const std::string& receiver, 
                     const std::string& content);
    
    // 广播发送
    bool broadcastMessage(const Message& message);
    bool broadcastMessage(const std::string& sender, const std::string& content);
    
    // 消息订阅
    SubscriptionId subscribe(MessageType type, MessageHandler handler);
    SubscriptionId subscribe(const std::string& subscriber, MessageType type, MessageHandler handler);
    bool unsubscribe(const SubscriptionId& subscriptionId);
    
    // 消息处理
    void processMessage(const Message& message);
    
    // 状态和统计
    size_t getQueueSize() const;
    size_t getSentMessageCount() const { return sentCount_.load(); }
    size_t getReceivedMessageCount() const { return receivedCount_.load(); }
    bool isRunning() const { return running_.load(); }

private:
    MessageSystem() = default;
    ~MessageSystem() = default;
    
    MessageSystem(const MessageSystem&) = delete;
    MessageSystem& operator=(const MessageSystem&) = delete;
    
    void messageLoop();
    void deliverMessage(const Message& message);
    void notifySubscribers(const Message& message);
    
    mutable std::mutex mutex_;
    std::queue<Message> messageQueue_;
    std::unordered_map<SubscriptionId, std::pair<MessageType, MessageHandler>> subscriptions_;
    std::unordered_map<std::string, std::vector<std::string>> subscriberSubscriptions_;
    
    std::condition_variable condition_;
    std::atomic<bool> running_{false};
    std::atomic<size_t> sentCount_{0};
    std::atomic<size_t> receivedCount_{0};
    std::atomic<uint64_t> subscriptionCounter_{0};
    std::weak_ptr<EventDispatcher> eventDispatcher_;
    std::thread messageThread_;
};

} // namespace openclaw

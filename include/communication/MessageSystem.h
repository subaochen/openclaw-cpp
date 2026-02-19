#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <future>
#include "../events/Event.h"

namespace openclaw {

// 消息类型
enum class MessageType {
    UNKNOWN = 0,
    TASK_ASSIGN,      // 任务分配
    TASK_COMPLETE,    // 任务完成
    TASK_FAILED,      // 任务失败
    STATUS_UPDATE,    // 状态更新
    HEARTBEAT,        // 心跳
    COMMAND,          // 命令
    RESPONSE,         // 响应
    BROADCAST,        // 广播
    DIRECT            // 直接消息
};

// 消息优先级
enum class MessagePriority {
    LOW = 0,
    NORMAL,
    HIGH,
    CRITICAL
};

// 消息头部
struct MessageHeader {
    std::string messageId;           // 消息ID
    std::string from;               // 发送者
    std::string to;                 // 接收者
    MessageType type{MessageType::UNKNOWN};
    MessagePriority priority{MessagePriority::NORMAL};
    std::string timestamp;          // 时间戳
    std::string correlationId;        // 关联ID（用于请求-响应）
    size_t payloadSize{0};           // 负载大小
};

// 消息
class Message {
public:
    Message() = default;
    ~Message() = default;
    
    // 构建消息
    static Message create(const std::string& from, const std::string& to, 
                         MessageType type, const std::string& content);
    
    // 获取信息
    const MessageHeader& getHeader() const { return header_; }
    const std::string& getContent() const { return content_; }
    std::string getId() const { return header_.messageId; }
    std::string getFrom() const { return header_.from; }
    std::string getTo() const { return header_.to; }
    MessageType getType() const { return header_.type; }
    MessagePriority getPriority() const { return header_.priority; }
    
    // 序列化
    std::string serialize() const;
    static Message deserialize(const std::string& data);
    
    // 设置内容
    void setContent(const std::string& content);
    void setPriority(MessagePriority priority);
    void setCorrelationId(const std::string& id);

private:
    MessageHeader header_;
    std::string content_;
    
public:
    // 生成唯一ID
    static std::string generateId();
};

// 消息处理器
using MessageHandler = std::function<void(const Message&)>;

// 消息订阅者
struct MessageSubscriber {
    std::string id;
    std::string name;
    MessageType messageType;
    MessageHandler handler;
};

// 消息系统
class MessageSystem {
public:
    MessageSystem();
    ~MessageSystem();
    
    // 禁用拷贝
    MessageSystem(const MessageSystem&) = delete;
    MessageSystem& operator=(const MessageSystem&) = delete;
    
    // 初始化与生命周期
    bool initialize();
    void shutdown();
    bool isRunning() const;
    
    // 发送消息
    bool sendMessage(const Message& message);
    bool sendMessage(const std::string& from, const std::string& to, 
                    MessageType type, const std::string& content);
    
    // 广播消息
    bool broadcastMessage(const std::string& from, MessageType type, 
                         const std::string& content);
    
    // 订阅消息
    std::string subscribe(MessageType type, const std::string& name, 
                         MessageHandler handler);
    bool unsubscribe(const std::string& subscriberId);
    
    // 路由控制
    void enableRouting(bool enable);
    bool isRoutingEnabled() const;
    
    // 消息确认机制
    void enableAcknowledgment(bool enable);
    bool waitForAcknowledgment(const std::string& messageId, 
                               std::chrono::milliseconds timeout);
    
    // 统计信息
    struct MessageStats {
        size_t totalSent;
        size_t totalReceived;
        size_t totalFailed;
        size_t queueSize;
        double averageLatencyMs;
    };
    MessageStats getStats() const;
    
    // 清理
    size_t cleanupOldMessages(std::chrono::seconds maxAge);

private:
    std::atomic<bool> running_{false};
    std::atomic<bool> routingEnabled_{true};
    std::atomic<bool> acknowledgmentEnabled_{false};
    
    // 订阅者管理
    mutable std::mutex subscribersMutex_;
    std::unordered_map<std::string, MessageSubscriber> subscribers_;
    std::unordered_map<MessageType, std::vector<std::string>> typeSubscribers_;
    
    // 消息队列
    mutable std::mutex queueMutex_;
    std::queue<Message> messageQueue_;
    std::condition_variable queueCV_;
    
    // 处理线程
    std::thread processorThread_;
    void processLoop();
    void routeMessage(const Message& message);
    
    // 统计
    mutable std::mutex statsMutex_;
    MessageStats stats_{};
    
    // 确认追踪
    mutable std::mutex ackMutex_;
    std::unordered_map<std::string, std::promise<bool>> pendingAcks_;
};

// 消息系统事件
struct MessageReceivedEvent : public Event {
    std::string messageId;
    std::string from;
    std::string to;
    MessageType type;
    
    MessageReceivedEvent(const std::string& mid, const std::string& f, 
                        const std::string& t, MessageType tp)
        : Event(EventType::MESSAGE_RECEIVED), messageId(mid), from(f), to(t), type(tp) {}
};

struct MessageSentEvent : public Event {
    std::string messageId;
    std::string from;
    std::string to;
    MessageType type;
    bool success;
    
    MessageSentEvent(const std::string& mid, const std::string& f, 
                    const std::string& t, MessageType tp, bool ok)
        : Event(EventType::MESSAGE_SENT), messageId(mid), from(f), to(t), type(tp), success(ok) {}
};

} // namespace openclaw

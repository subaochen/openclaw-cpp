#include "communication/MessageSystem.h"
#include "logging/Logger.h"
#include "events/EventDispatcher.h"
#include <chrono>
#include <sstream>
#include <iomanip>
#include <random>
#include <regex>
#include <future>

namespace openclaw {

// Message 实现
Message Message::create(const std::string& from, const std::string& to, 
                       MessageType type, const std::string& content) {
    Message msg;
    msg.header_.messageId = generateId();
    msg.header_.from = from;
    msg.header_.to = to;
    msg.header_.type = type;
    
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    msg.header_.timestamp = oss.str();
    
    msg.content_ = content;
    msg.header_.payloadSize = content.size();
    
    return msg;
}

std::string Message::serialize() const {
    std::ostringstream oss;
    oss << "{";
    oss << "\"messageId\":\"" << header_.messageId << "\",";
    oss << "\"from\":\"" << header_.from << "\",";
    oss << "\"to\":\"" << header_.to << "\",";
    oss << "\"type\":" << static_cast<int>(header_.type) << ",";
    oss << "\"priority\":" << static_cast<int>(header_.priority) << ",";
    oss << "\"timestamp\":\"" << header_.timestamp << "\",";
    oss << "\"correlationId\":\"" << header_.correlationId << "\",";
    oss << "\"payloadSize\":" << header_.payloadSize << ",";
    oss << "\"content\":\"" << content_ << "\"";
    oss << "}";
    return oss.str();
}

Message Message::deserialize(const std::string& data) {
    Message msg;
    // 简化实现：实际应使用 JSON 解析库
    // 这里仅解析必要字段
    std::regex idRegex("\"messageId\":\"([^\"]+)\"");
    std::regex fromRegex("\"from\":\"([^\"]+)\"");
    std::regex toRegex("\"to\":\"([^\"]+)\"");
    std::regex contentRegex("\"content\":\"([^\"]+)\"");
    
    std::smatch match;
    if (std::regex_search(data, match, idRegex)) {
        msg.header_.messageId = match[1];
    }
    if (std::regex_search(data, match, fromRegex)) {
        msg.header_.from = match[1];
    }
    if (std::regex_search(data, match, toRegex)) {
        msg.header_.to = match[1];
    }
    if (std::regex_search(data, match, contentRegex)) {
        msg.content_ = match[1];
    }
    
    return msg;
}

void Message::setContent(const std::string& content) {
    content_ = content;
    header_.payloadSize = content.size();
}

void Message::setPriority(MessagePriority priority) {
    header_.priority = priority;
}

void Message::setCorrelationId(const std::string& id) {
    header_.correlationId = id;
}

std::string Message::generateId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex;
    for (int i = 0; i < 32; ++i) {
        ss << dis(gen);
    }
    return ss.str();
}

// MessageSystem 实现
MessageSystem::MessageSystem() {}

MessageSystem::~MessageSystem() {
    shutdown();
}

bool MessageSystem::initialize() {
    if (running_.exchange(true)) {
        return false; // 已在运行
    }
    
    processorThread_ = std::thread(&MessageSystem::processLoop, this);
    
    Logger::getInstance().info("MessageSystem", "Initialized successfully");
    return true;
}

void MessageSystem::shutdown() {
    if (!running_.exchange(false)) {
        return;
    }
    
    queueCV_.notify_all();
    
    if (processorThread_.joinable()) {
        processorThread_.join();
    }
    
    Logger::getInstance().info("MessageSystem", "Shutdown completed");
}

bool MessageSystem::isRunning() const {
    return running_.load();
}

bool MessageSystem::sendMessage(const Message& message) {
    if (!running_) {
        Logger::getInstance().error("MessageSystem", "Cannot send message, system not running");
        return false;
    }
    
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        messageQueue_.push(message);
    }
    
    queueCV_.notify_one();
    
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        stats_.totalSent++;
    }
    
    Logger::getInstance().debug("MessageSystem", 
        "Queued message: " + message.getId() + " from " + message.getFrom() + " to " + message.getTo());
    
    return true;
}

bool MessageSystem::sendMessage(const std::string& from, const std::string& to, 
                               MessageType type, const std::string& content) {
    auto message = Message::create(from, to, type, content);
    return sendMessage(message);
}

bool MessageSystem::broadcastMessage(const std::string& from, MessageType type, 
                                    const std::string& content) {
    std::lock_guard<std::mutex> lock(subscribersMutex_);
    
    auto it = typeSubscribers_.find(type);
    if (it == typeSubscribers_.end()) {
        return true; // 没有订阅者，不算失败
    }
    
    bool allSuccess = true;
    for (const auto& subId : it->second) {
        auto subIt = subscribers_.find(subId);
        if (subIt != subscribers_.end()) {
            auto message = Message::create(from, subIt->second.name, type, content);
            sendMessage(message);
        }
    }
    
    return allSuccess;
}

std::string MessageSystem::subscribe(MessageType type, const std::string& name, 
                                    MessageHandler handler) {
    std::lock_guard<std::mutex> lock(subscribersMutex_);
    
    MessageSubscriber sub;
    sub.id = Message::generateId();
    sub.name = name;
    sub.messageType = type;
    sub.handler = handler;
    
    subscribers_[sub.id] = sub;
    typeSubscribers_[type].push_back(sub.id);
    
    Logger::getInstance().info("MessageSystem", 
        "Subscriber added: " + name + " (ID: " + sub.id + ")");
    
    return sub.id;
}

bool MessageSystem::unsubscribe(const std::string& subscriberId) {
    std::lock_guard<std::mutex> lock(subscribersMutex_);
    
    auto it = subscribers_.find(subscriberId);
    if (it == subscribers_.end()) {
        return false;
    }
    
    auto type = it->second.messageType;
    subscribers_.erase(it);
    
    auto& subs = typeSubscribers_[type];
    subs.erase(std::remove(subs.begin(), subs.end(), subscriberId), subs.end());
    
    Logger::getInstance().info("MessageSystem", "Subscriber removed: " + subscriberId);
    
    return true;
}

void MessageSystem::enableRouting(bool enable) {
    routingEnabled_ = enable;
}

bool MessageSystem::isRoutingEnabled() const {
    return routingEnabled_.load();
}

void MessageSystem::enableAcknowledgment(bool enable) {
    acknowledgmentEnabled_ = enable;
}

bool MessageSystem::waitForAcknowledgment(const std::string& messageId, 
                                         std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(ackMutex_);
    
    auto it = pendingAcks_.find(messageId);
    if (it == pendingAcks_.end()) {
        std::promise<bool> promise;
        pendingAcks_[messageId] = std::move(promise);
        it = pendingAcks_.find(messageId);
    }
    
    auto future = it->second.get_future();
    lock.unlock();
    
    return future.wait_for(timeout) == std::future_status::ready && future.get();
}

MessageSystem::MessageStats MessageSystem::getStats() const {
    std::lock_guard<std::mutex> lock(statsMutex_);
    return stats_;
}

size_t MessageSystem::cleanupOldMessages(std::chrono::seconds maxAge) {
    // 简化实现
    return 0;
}

void MessageSystem::processLoop() {
    while (running_) {
        std::unique_lock<std::mutex> lock(queueMutex_);
        queueCV_.wait(lock, [this] { return !messageQueue_.empty() || !running_; });
        
        while (!messageQueue_.empty()) {
            auto message = messageQueue_.front();
            messageQueue_.pop();
            lock.unlock();
            
            routeMessage(message);
            
            lock.lock();
        }
    }
}

void MessageSystem::routeMessage(const Message& message) {
    if (!routingEnabled_) {
        Logger::getInstance().debug("MessageSystem", 
            "Routing disabled, ignoring message: " + message.getId());
        return;
    }
    
    {
        std::lock_guard<std::mutex> lock(statsMutex_);
        stats_.totalReceived++;
    }
    
    // 找到匹配的订阅者
    std::vector<std::string> subscribersToNotify;
    {
        std::lock_guard<std::mutex> lock(subscribersMutex_);
        auto it = typeSubscribers_.find(message.getType());
        if (it != typeSubscribers_.end()) {
            subscribersToNotify = it->second;
        }
    }
    
    // 通知每个订阅者
    for (const auto& subscriberId : subscribersToNotify) {
        std::lock_guard<std::mutex> lock(subscribersMutex_);
        auto subIt = subscribers_.find(subscriberId);
        if (subIt != subscribers_.end()) {
            try {
                subIt->second.handler(message);
            } catch (const std::exception& e) {
                Logger::getInstance().error("MessageSystem", 
                    "Error calling handler for subscriber " + subscriberId + ": " + e.what());
                {
                    std::lock_guard<std::mutex> lock(statsMutex_);
                    stats_.totalFailed++;
                }
            }
        }
    }
}

} // namespace openclaw

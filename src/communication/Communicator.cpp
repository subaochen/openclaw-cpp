#include "communication/Communicator.h"
#include <iostream>
#include <chrono>
#include <random>

namespace openclaw {

Communicator::Communicator()
    : isRunning_(false),
      isConnected_(false),
      heartbeatActive_(false),
      host_("localhost"),
      port_(8080),
      timeout_(5000),
      reconnectionAttempts_(3),
      reconnectionDelay_(1000),
      heartbeatInterval_(30000),
      connectionCount_(0) {
}

Communicator::~Communicator() {
    shutdown();
}

Communicator& Communicator::getInstance() {
    static Communicator instance;
    return instance;
}

bool Communicator::initialize() {
    if (isRunning_) {
        return true;
    }

    isRunning_ = true;
    
    // 启动消息处理线程
    handlerThread_ = std::thread([this]() {
        messageHandlerThread();
    });

    // 初始化消息队列
    messageQueue_.clear();

    return true;
}

void Communicator::shutdown() {
    if (!isRunning_) {
        return;
    }

    isRunning_ = false;
    
    // 停止心跳
    stopHeartbeat();
    
    // 等待线程结束
    if (handlerThread_.joinable()) {
        handlerThread_.join();
    }
    
    // 断开连接
    disconnect();
}

bool Communicator::connect(const std::string& host, int port) {
    if (isConnected_) {
        return true;
    }

    std::lock_guard<std::mutex> lock(mutex_);
    
    host_ = host;
    port_ = port;
    
    // 模拟连接过程
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // 连接成功
    isConnected_ = true;
    connectionCount_++;
    
    // 启动心跳
    startHeartbeat(heartbeatInterval_);
    
    return true;
}

void Communicator::disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!isConnected_) {
        return;
    }
    
    isConnected_ = false;
    
    // 停止心跳
    stopHeartbeat();
}

bool Communicator::isConnected() const {
    return isConnected_;
}

bool Communicator::sendMessage(const std::string& recipient, const std::string& message) {
    if (!isConnected_) {
        // 尝试重连
        if (!attemptReconnection()) {
            return false;
        }
    }
    
    std::lock_guard<std::mutex> lock(queueMutex_);
    
    // 存储消息到队列
    messageQueue_[recipient].push_back(serializeMessage("local", recipient, message));
    
    // 通知等待消息的线程
    messageCondition_.notify_all();
    
    // 调用对应的消息处理函数（如果有）
    if (hasMessageHandler(recipient)) {
        auto it = messageHandlers_.find(recipient);
        if (it != messageHandlers_.end()) {
            it->second("local", message);
        }
    }
    
    return true;
}

std::string Communicator::receiveMessage(const std::string& sender) {
    std::unique_lock<std::mutex> lock(queueMutex_);
    
    // 等待消息到达
    if (waitForMessage(getTimeout())) {
        auto it = messageQueue_.find(sender);
        if (it != messageQueue_.end() && !it->second.empty()) {
            std::string message = it->second.front();
            it->second.erase(it->second.begin());
            
            std::string actualSender, recipient, content;
            deserializeMessage(message, actualSender, recipient, content);
            
            return content;
        }
    }
    
    return "";
}

void Communicator::registerMessageHandler(const std::string& recipient, MessageHandler handler) {
    std::lock_guard<std::mutex> lock(mutex_);
    messageHandlers_[recipient] = handler;
}

void Communicator::unregisterMessageHandler(const std::string& recipient) {
    std::lock_guard<std::mutex> lock(mutex_);
    messageHandlers_.erase(recipient);
}

bool Communicator::hasMessageHandler(const std::string& recipient) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return messageHandlers_.find(recipient) != messageHandlers_.end();
}

void Communicator::sendBroadcast(const std::string& message) {
    if (!isConnected_) {
        return;
    }
    
    // 发送广播消息
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (const auto& handler : messageHandlers_) {
        handler.second("broadcast", message);
    }
}

std::vector<std::string> Communicator::getConnectedPeers() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<std::string> peers;
    for (const auto& handler : messageHandlers_) {
        peers.push_back(handler.first);
    }
    
    return peers;
}

int Communicator::getConnectionCount() const {
    return connectionCount_;
}

void Communicator::setTimeout(int timeout) {
    std::lock_guard<std::mutex> lock(mutex_);
    timeout_ = timeout;
}

int Communicator::getTimeout() const {
    return timeout_;
}

void Communicator::setReconnectionAttempts(int attempts) {
    std::lock_guard<std::mutex> lock(mutex_);
    reconnectionAttempts_ = attempts;
}

int Communicator::getReconnectionAttempts() const {
    return reconnectionAttempts_;
}

void Communicator::setReconnectionDelay(int delay) {
    std::lock_guard<std::mutex> lock(mutex_);
    reconnectionDelay_ = delay;
}

int Communicator::getReconnectionDelay() const {
    return reconnectionDelay_;
}

void Communicator::startHeartbeat(int interval) {
    if (heartbeatActive_) {
        return;
    }
    
    heartbeatInterval_ = interval;
    heartbeatActive_ = true;
    
    heartbeatThread_ = std::thread([this]() {
        heartbeatThread();
    });
}

void Communicator::stopHeartbeat() {
    heartbeatActive_ = false;
    
    if (heartbeatThread_.joinable()) {
        heartbeatThread_.join();
    }
}

bool Communicator::isHeartbeatActive() const {
    return heartbeatActive_;
}

bool Communicator::isRunning() const {
    return isRunning_;
}

bool Communicator::waitForMessage(int timeout) {
    std::unique_lock<std::mutex> lock(queueMutex_);
    
    if (timeout > 0) {
        // 立即检查是否有消息，不等待
        bool hasMessages = false;
        for (const auto& queue : messageQueue_) {
            if (!queue.second.empty()) {
                hasMessages = true;
                break;
            }
        }
        
        return hasMessages;
    } else {
        return false;
    }
}

void Communicator::messageHandlerThread() {
    while (isRunning_) {
        // 处理消息（这里简化处理）
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool Communicator::attemptReconnection() {
    for (int i = 0; i < reconnectionAttempts_; ++i) {
        if (connect(host_, port_)) {
            return true;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(reconnectionDelay_));
    }
    
    return false;
}

void Communicator::heartbeatThread() {
    while (heartbeatActive_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(heartbeatInterval_));
        
        if (isConnected_) {
            sendHeartbeat();
        }
    }
}

bool Communicator::sendHeartbeat() {
    // 发送心跳消息
    std::string heartbeatMessage = "__HEARTBEAT__";
    return sendMessage("system", heartbeatMessage);
}

std::string Communicator::generateMessageId() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 9);
    
    std::string messageId;
    for (int i = 0; i < 8; ++i) {
        messageId += std::to_string(dis(gen));
    }
    
    return messageId;
}

std::string Communicator::serializeMessage(const std::string& sender, const std::string& recipient, const std::string& message) {
    // 简单的消息序列化格式：sender|recipient|message
    return sender + "|" + recipient + "|" + message;
}

bool Communicator::deserializeMessage(const std::string& data, std::string& sender, std::string& recipient, std::string& message) {
    size_t firstDelim = data.find('|');
    if (firstDelim == std::string::npos) {
        return false;
    }
    
    size_t secondDelim = data.find('|', firstDelim + 1);
    if (secondDelim == std::string::npos) {
        return false;
    }
    
    sender = data.substr(0, firstDelim);
    recipient = data.substr(firstDelim + 1, secondDelim - firstDelim - 1);
    message = data.substr(secondDelim + 1);
    
    return true;
}

} // namespace openclaw
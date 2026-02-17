#ifndef OPENCLAW_COMMUNICATOR_H
#define OPENCLAW_COMMUNICATOR_H

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <mutex>
#include <thread>
#include <atomic>
#include <condition_variable>

namespace openclaw {

class Communicator {
public:
    using MessageHandler = std::function<void(const std::string& sender, const std::string& message)>;
    
    static Communicator& getInstance();

    bool initialize();
    void shutdown();

    bool connect(const std::string& host, int port);
    void disconnect();
    bool isConnected() const;

    bool sendMessage(const std::string& recipient, const std::string& message);
    std::string receiveMessage(const std::string& sender);

    void registerMessageHandler(const std::string& recipient, MessageHandler handler);
    void unregisterMessageHandler(const std::string& recipient);
    bool hasMessageHandler(const std::string& recipient) const;

    void sendBroadcast(const std::string& message);
    
    std::vector<std::string> getConnectedPeers() const;
    int getConnectionCount() const;

    void setTimeout(int timeout);
    int getTimeout() const;

    void setReconnectionAttempts(int attempts);
    int getReconnectionAttempts() const;

    void setReconnectionDelay(int delay);
    int getReconnectionDelay() const;

    void startHeartbeat(int interval);
    void stopHeartbeat();
    bool isHeartbeatActive() const;

    bool isRunning() const;
    bool waitForMessage(int timeout = -1);

private:
    Communicator();
    ~Communicator();

    Communicator(const Communicator&) = delete;
    Communicator& operator=(const Communicator&) = delete;
    Communicator(Communicator&&) = delete;
    Communicator& operator=(Communicator&&) = delete;

    void messageHandlerThread();
    bool attemptReconnection();
    void heartbeatThread();
    bool sendHeartbeat();

    std::string generateMessageId();
    std::string serializeMessage(const std::string& sender, const std::string& recipient, const std::string& message);
    bool deserializeMessage(const std::string& data, std::string& sender, std::string& recipient, std::string& message);

    std::map<std::string, MessageHandler> messageHandlers_;
    std::map<std::string, std::vector<std::string>> messageQueue_;
    
    mutable std::mutex mutex_;
    mutable std::mutex queueMutex_;
    
    std::thread handlerThread_;
    std::thread heartbeatThread_;
    
    std::condition_variable messageCondition_;
    
    std::atomic<bool> isRunning_;
    std::atomic<bool> isConnected_;
    std::atomic<bool> heartbeatActive_;
    
    std::string host_;
    int port_;
    int timeout_;
    int reconnectionAttempts_;
    int reconnectionDelay_;
    int heartbeatInterval_;
    
    int connectionCount_;
};

} // namespace openclaw

#endif // OPENCLAW_COMMUNICATOR_H
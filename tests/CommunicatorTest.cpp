#include <gtest/gtest.h>
#include "communication/Communicator.h"

using namespace openclaw;

// 测试通信系统的基本功能
TEST(CommunicatorTest, BasicFunctionality) {
    // 创建通信系统实例
    Communicator& communicator = Communicator::getInstance();
    
    // 测试初始化
    EXPECT_TRUE(communicator.initialize());
    
    // 测试连接状态
    EXPECT_FALSE(communicator.isConnected());
    
    // 测试连接和断开连接
    EXPECT_TRUE(communicator.connect("localhost", 8080));
    EXPECT_TRUE(communicator.isConnected());
    
    communicator.disconnect();
    EXPECT_FALSE(communicator.isConnected());
}

// 测试消息发送和接收
TEST(CommunicatorTest, MessageSending) {
    Communicator& communicator = Communicator::getInstance();
    
    // 重置通信系统
    communicator.shutdown();
    EXPECT_TRUE(communicator.initialize());
    
    // 连接到本地服务器
    EXPECT_TRUE(communicator.connect("localhost", 8080));
    
    // 发送消息
    EXPECT_TRUE(communicator.sendMessage("test_user", "Test message"));
    
    // 测试接收消息
    std::string receivedMessage = communicator.receiveMessage("test_user");
    EXPECT_FALSE(receivedMessage.empty());
}

// 测试消息处理函数的注册和调用
TEST(CommunicatorTest, MessageHandlerRegistration) {
    Communicator& communicator = Communicator::getInstance();
    
    // 重置通信系统
    communicator.shutdown();
    EXPECT_TRUE(communicator.initialize());
    
    // 连接到本地服务器
    EXPECT_TRUE(communicator.connect("localhost", 8080));
    
    // 测试消息处理函数的注册
    bool handlerCalled = false;
    communicator.registerMessageHandler("test_user", 
        [&handlerCalled](const std::string& sender, const std::string& message) {
            handlerCalled = true;
        });
    
    // 发送消息
    EXPECT_TRUE(communicator.sendMessage("test_user", "Test message"));
    
    // 验证消息处理函数是否被调用
    EXPECT_TRUE(handlerCalled);
}

// 测试消息处理函数的注销
TEST(CommunicatorTest, MessageHandlerUnregistration) {
    Communicator& communicator = Communicator::getInstance();
    
    // 重置通信系统
    communicator.shutdown();
    EXPECT_TRUE(communicator.initialize());
    
    // 连接到本地服务器
    EXPECT_TRUE(communicator.connect("localhost", 8080));
    
    // 测试消息处理函数的注销
    bool handlerCalled = false;
    auto handler = [&handlerCalled](const std::string& sender, const std::string& message) {
        handlerCalled = true;
    };
    
    communicator.registerMessageHandler("test_user", handler);
    communicator.unregisterMessageHandler("test_user");
    
    // 发送消息
    EXPECT_TRUE(communicator.sendMessage("test_user", "Test message"));
    
    // 验证消息处理函数是否未被调用
    EXPECT_FALSE(handlerCalled);
}

// 测试通信系统的单例模式
TEST(CommunicatorTest, SingletonPattern) {
    Communicator& communicator1 = Communicator::getInstance();
    Communicator& communicator2 = Communicator::getInstance();
    
    // 验证两个实例是否是同一个
    EXPECT_EQ(&communicator1, &communicator2);
}

// 测试心跳检测功能
TEST(CommunicatorTest, HeartbeatDetection) {
    Communicator& communicator = Communicator::getInstance();
    
    // 重置通信系统
    communicator.shutdown();
    EXPECT_TRUE(communicator.initialize());
    
    // 连接到本地服务器
    EXPECT_TRUE(communicator.connect("localhost", 8080));
    
    // 测试心跳检测
    communicator.startHeartbeat(1000);  // 1秒间隔
    EXPECT_TRUE(communicator.isHeartbeatActive());
    
    communicator.stopHeartbeat();
    EXPECT_FALSE(communicator.isHeartbeatActive());
}

// 测试通信系统的超时设置
TEST(CommunicatorTest, TimeoutSetting) {
    Communicator& communicator = Communicator::getInstance();
    
    // 重置通信系统
    communicator.shutdown();
    EXPECT_TRUE(communicator.initialize());
    
    // 测试超时设置
    communicator.setTimeout(5000);
    EXPECT_EQ(communicator.getTimeout(), 5000);
    
    communicator.setTimeout(10000);
    EXPECT_EQ(communicator.getTimeout(), 10000);
}

// 测试通信系统的重新连接功能
TEST(CommunicatorTest, Reconnection) {
    Communicator& communicator = Communicator::getInstance();
    
    // 重置通信系统
    communicator.shutdown();
    EXPECT_TRUE(communicator.initialize());
    
    // 测试重新连接设置
    communicator.setReconnectionAttempts(3);
    EXPECT_EQ(communicator.getReconnectionAttempts(), 3);
    
    communicator.setReconnectionDelay(1000);
    EXPECT_EQ(communicator.getReconnectionDelay(), 1000);
}
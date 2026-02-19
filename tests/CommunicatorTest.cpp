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
    
    communicator.disconnect();
    EXPECT_FALSE(communicator.isConnected());
}

// 测试通信系统的单例模式
TEST(CommunicatorTest, SingletonPattern) {
    Communicator& communicator1 = Communicator::getInstance();
    Communicator& communicator2 = Communicator::getInstance();
    
    // 验证两个实例是否是同一个
    EXPECT_EQ(&communicator1, &communicator2);
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
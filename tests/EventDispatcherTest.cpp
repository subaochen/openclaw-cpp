#include <gtest/gtest.h>
#include "events/EventDispatcher.h"

using namespace openclaw;

// 测试事件调度器的基本功能
TEST(EventDispatcherTest, BasicFunctionality) {
    // 创建事件调度器实例
    EventDispatcher& dispatcher = EventDispatcher::getInstance();
    
    // 测试事件调度是否启用
    EXPECT_TRUE(dispatcher.isEventDispatchEnabled());
    
    // 测试获取事件统计 - 更新为Phase 2的事件类型数量（28种）
    auto statistics = dispatcher.getEventStatistics();
    EXPECT_EQ(statistics.size(), 28);
    
    // 测试获取活跃事件处理函数数量
    EXPECT_EQ(dispatcher.getActiveEventHandlerCount(), 0);
}

// 测试事件处理函数的注册和调用
TEST(EventDispatcherTest, EventHandlerRegistration) {
    EventDispatcher& dispatcher = EventDispatcher::getInstance();
    
    // 重置事件处理函数
    dispatcher.clearAllEventHandlers();
    
    // 测试事件处理函数的注册
    bool handlerCalled = false;
    dispatcher.registerEventHandler(EventType::INFO_MESSAGE, 
        [&handlerCalled](const Event& event) {
            handlerCalled = true;
        });
    
    // 测试获取活跃事件处理函数数量
    EXPECT_EQ(dispatcher.getActiveEventHandlerCount(), 1);
    
    // 发送事件 - 使用简化版本
    dispatcher.dispatchEvent(EventType::INFO_MESSAGE);
    
    // 验证事件处理函数是否被调用
    EXPECT_TRUE(handlerCalled);
}

// 测试事件处理函数的注销
TEST(EventDispatcherTest, EventHandlerUnregistration) {
    EventDispatcher& dispatcher = EventDispatcher::getInstance();
    
    // 重置事件处理函数
    dispatcher.clearAllEventHandlers();
    
    // 测试事件处理函数的注册和注销
    bool handlerCalled = false;
    EventHandler handler = [&handlerCalled](const Event& event) {
        handlerCalled = true;
    };
    
    dispatcher.registerEventHandler(EventType::INFO_MESSAGE, handler);
    dispatcher.unregisterEventHandler(EventType::INFO_MESSAGE, handler);
    
    // 发送事件
    dispatcher.dispatchEvent(EventType::INFO_MESSAGE);
    
    // 验证事件处理函数是否未被调用
    EXPECT_FALSE(handlerCalled);
}

// 测试事件调度的启用和禁用
TEST(EventDispatcherTest, EventDispatchEnablement) {
    EventDispatcher& dispatcher = EventDispatcher::getInstance();
    
    // 重置事件处理函数
    dispatcher.clearAllEventHandlers();
    
    // 测试事件调度的启用和禁用
    bool handlerCalled = false;
    dispatcher.registerEventHandler(EventType::INFO_MESSAGE, 
        [&handlerCalled](const Event& event) {
            handlerCalled = true;
        });
    
    dispatcher.setEventDispatchEnabled(false);
    dispatcher.dispatchEvent(EventType::INFO_MESSAGE);
    EXPECT_FALSE(handlerCalled);
    
    dispatcher.setEventDispatchEnabled(true);
    dispatcher.dispatchEvent(EventType::INFO_MESSAGE);
    EXPECT_TRUE(handlerCalled);
}

// 测试事件调度器的单例模式
TEST(EventDispatcherTest, SingletonPattern) {
    EventDispatcher& dispatcher1 = EventDispatcher::getInstance();
    EventDispatcher& dispatcher2 = EventDispatcher::getInstance();
    
    // 验证两个实例是否是同一个
    EXPECT_EQ(&dispatcher1, &dispatcher2);
}

// 测试事件队列大小的监控
TEST(EventDispatcherTest, EventQueueSize) {
    EventDispatcher& dispatcher = EventDispatcher::getInstance();
    
    // 重置事件处理函数
    dispatcher.clearAllEventHandlers();
    
    // 测试事件队列大小
    EXPECT_EQ(dispatcher.getEventQueueSize(), 0);
    
    // 发送事件
    dispatcher.dispatchEvent(EventType::INFO_MESSAGE);
    
    // 验证事件队列大小
    EXPECT_EQ(dispatcher.getEventQueueSize(), 0);  // 事件已处理
}

// 测试事件统计功能
TEST(EventDispatcherTest, EventStatistics) {
    EventDispatcher& dispatcher = EventDispatcher::getInstance();
    
    // 重置事件统计
    dispatcher.clearAllEventHandlers();
    dispatcher.resetEventStatistics();
    
    // 发送事件
    dispatcher.dispatchEvent(EventType::INFO_MESSAGE);
    dispatcher.dispatchEvent(EventType::INFO_MESSAGE);
    
    // 验证事件统计
    auto statistics = dispatcher.getEventStatistics();
    EXPECT_EQ(statistics[EventType::INFO_MESSAGE], 2);
}
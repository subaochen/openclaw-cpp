#include <iostream>
#include <thread>
#include <chrono>

#include "events/EventDispatcher.h"
#include "config/ConfigManager.h"
#include "logging/Logger.h"
#include "communication/Communicator.h"

using namespace openclaw;

void initializeComponents() {
    // 初始化配置管理
    ConfigManager::getInstance().loadConfig("config.ini");
    
    // 初始化日志系统
    Logger::getInstance().initialize("openclaw.log");
    Logger::getInstance().info("OpenClaw-CPP system starting up");
    
    // 初始化通信系统
    Communicator::getInstance().initialize();
    
    // 初始化事件驱动系统
    EventDispatcher::getInstance().setEventDispatchEnabled(true);
}

void registerEventHandlers() {
    // 系统启动事件处理
    EventDispatcher::getInstance().registerEventHandler(
        EventType::SYSTEM_STARTUP,
        [](const Event& event) {
            Logger::getInstance().info("System startup event received");
            Logger::getInstance().info("Source: " + event.source);
            Logger::getInstance().info("Data: " + event.data);
        }
    );
    
    // 任务完成事件处理
    EventDispatcher::getInstance().registerEventHandler(
        EventType::TASK_COMPLETED,
        [](const Event& event) {
            Logger::getInstance().info("Task completed: " + event.data);
        }
    );
    
    // 任务失败事件处理
    EventDispatcher::getInstance().registerEventHandler(
        EventType::TASK_FAILED,
        [](const Event& event) {
            Logger::getInstance().error("Task failed: " + event.data);
        }
    );
    
    // 测试通过事件处理
    EventDispatcher::getInstance().registerEventHandler(
        EventType::TEST_PASSED,
        [](const Event& event) {
            Logger::getInstance().info("Test passed: " + event.data);
        }
    );
    
    // 测试失败事件处理
    EventDispatcher::getInstance().registerEventHandler(
        EventType::TEST_FAILED,
        [](const Event& event) {
            Logger::getInstance().warning("Test failed: " + event.data);
        }
    );
}

void registerMessageHandlers() {
    // 消息处理
    Communicator::getInstance().registerMessageHandler(
        "project manager",
        [](const std::string& sender, const std::string& message) {
            Logger::getInstance().info("Received message from project manager: " + message);
        }
    );
    
    Communicator::getInstance().registerMessageHandler(
        "coder",
        [](const std::string& sender, const std::string& message) {
            Logger::getInstance().info("Received message from coder: " + message);
        }
    );
    
    Communicator::getInstance().registerMessageHandler(
        "tester",
        [](const std::string& sender, const std::string& message) {
            Logger::getInstance().info("Received message from tester: " + message);
        }
    );
}

void testEventDispatcher() {
    Logger::getInstance().info("Testing event dispatcher");
    
    // 发送系统启动事件
    EventDispatcher::getInstance().dispatchEvent(
        EventType::SYSTEM_STARTUP,
        "main",
        "System initialization completed"
    );
    
    // 发送任务完成事件
    EventDispatcher::getInstance().dispatchEvent(
        EventType::TASK_COMPLETED,
        "coder",
        "EventDispatcher implementation completed"
    );
    
    // 发送测试通过事件
    EventDispatcher::getInstance().dispatchEvent(
        EventType::TEST_PASSED,
        "tester",
        "EventDispatcher functional test"
    );
    
    // 发送警告事件
    EventDispatcher::getInstance().dispatchEvent(
        EventType::WARNING_OCCURRED,
        "system",
        "Low memory warning"
    );
}

void testCommunication() {
    Logger::getInstance().info("Testing communication system");
    
    if (Communicator::getInstance().connect("localhost", 8080)) {
        Logger::getInstance().info("Connected to communication system");
        
        // 发送消息
        Communicator::getInstance().sendMessage(
            "project manager",
            "Event dispatcher implementation completed"
        );
        
        Communicator::getInstance().sendMessage(
            "coder",
            "Testing communication module"
        );
        
        Communicator::getInstance().sendMessage(
            "tester",
            "Ready for integration testing"
        );
        
        // 发送广播消息
        Communicator::getInstance().sendBroadcast(
            "All components initialized and ready"
        );
    } else {
        Logger::getInstance().error("Failed to connect to communication system");
    }
}

void testConfiguration() {
    Logger::getInstance().info("Testing configuration system");
    
    // 获取配置值
    std::string logLevel = ConfigManager::getInstance().getString("log_level", "INFO");
    int maxFileSize = ConfigManager::getInstance().getInt("max_log_size", 10);
    bool debugMode = ConfigManager::getInstance().getBool("debug_mode", false);
    
    Logger::getInstance().info("Log level: " + logLevel);
    Logger::getInstance().info("Max log file size (MB): " + std::to_string(maxFileSize));
    Logger::getInstance().info("Debug mode: " + std::string(debugMode ? "true" : "false"));
}

void runTests() {
    Logger::getInstance().info("Running system tests");
    
    // 测试配置系统
    testConfiguration();
    
    // 测试事件调度器
    testEventDispatcher();
    
    // 测试通信系统
    testCommunication();
    
    Logger::getInstance().info("All tests completed");
}

int main() {
    std::cout << "=== OpenClaw-CPP System ===" << std::endl;
    
    try {
        // 初始化组件
        initializeComponents();
        
        // 注册事件和消息处理程序
        registerEventHandlers();
        registerMessageHandlers();
        
        // 运行测试
        runTests();
        
        // 系统运行状态
        Logger::getInstance().info("OpenClaw-CPP system is running");
        
        // 保持系统运行一段时间
        std::this_thread::sleep_for(std::chrono::seconds(5));
        
        // 系统关闭
        EventDispatcher::getInstance().dispatchEvent(
            EventType::SYSTEM_SHUTDOWN,
            "main",
            "System shutting down"
        );
        
        Logger::getInstance().info("OpenClaw-CPP system shut down");
        
    } catch (const std::exception& e) {
        Logger::getInstance().critical("Unhandled exception: " + std::string(e.what()));
        std::cerr << "Unhandled exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        Logger::getInstance().critical("Unknown exception occurred");
        std::cerr << "Unknown exception occurred" << std::endl;
        return 1;
    }
    
    return 0;
}
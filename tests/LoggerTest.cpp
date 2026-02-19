#include <gtest/gtest.h>
#include "logging/Logger.h"

using namespace openclaw;

// 测试日志系统的基本功能
TEST(LoggerTest, BasicFunctionality) {
    // 创建日志系统实例
    Logger& logger = Logger::getInstance();
    
    // 测试初始化
    EXPECT_TRUE(logger.initialize("test.log"));
    
    // 测试日志级别设置
    logger.setLogLevel(LogLevel::DEBUG);
    EXPECT_EQ(logger.getLogLevel(), LogLevel::DEBUG);
    
    // 测试日志输出设置
    logger.setConsoleOutputEnabled(true);
    EXPECT_TRUE(logger.isConsoleOutputEnabled());
    
    logger.setFileOutputEnabled(true);
    EXPECT_TRUE(logger.isFileOutputEnabled());
}

// 测试不同级别的日志输出
TEST(LoggerTest, LogLevelOutput) {
    Logger& logger = Logger::getInstance();
    
    // 重置日志系统
    logger.shutdown();
    EXPECT_TRUE(logger.initialize("test.log"));
    
    // 测试不同级别的日志输出
    logger.debug("Debug message");
    logger.info("Info message");
    logger.warning("Warning message");
    logger.error("Error message");
    logger.critical("Critical message");
    
    // 验证日志文件已创建
    EXPECT_FALSE(logger.getLogFile().empty());
}

// 测试日志格式化功能
TEST(LoggerTest, LogFormatting) {
    Logger& logger = Logger::getInstance();
    
    // 测试自定义格式
    std::string customFormat = "[%level%] %timestamp% %message%";
    logger.setFormat(customFormat);
    EXPECT_EQ(logger.getFormat(), customFormat);
    
    // 发送日志消息
    logger.info("Formatted message");
}

// 测试日志文件管理
TEST(LoggerTest, LogFileManagement) {
    Logger& logger = Logger::getInstance();
    
    // 测试最大文件大小设置
    logger.setMaxFileSize(1024 * 1024);  // 1MB
    EXPECT_EQ(logger.getMaxFileSize(), 1024 * 1024);
    
    // 测试备份文件数量设置
    logger.setMaxBackupFiles(5);
    EXPECT_EQ(logger.getMaxBackupFiles(), 5);
}

// 测试日志系统的单例模式
TEST(LoggerTest, SingletonPattern) {
    Logger& logger1 = Logger::getInstance();
    Logger& logger2 = Logger::getInstance();
    
    // 验证两个实例是否是同一个
    EXPECT_EQ(&logger1, &logger2);
}

// 测试日志系统的关闭
TEST(LoggerTest, Shutdown) {
    Logger& logger = Logger::getInstance();
    
    // 测试关闭
    logger.shutdown();
    
    // 测试重新初始化
    EXPECT_TRUE(logger.initialize("test2.log"));
}

// 测试日志系统的线程安全模式
TEST(LoggerTest, ThreadSafety) {
    Logger& logger = Logger::getInstance();
    
    // 测试线程安全模式
    logger.setThreadSafe(true);
    EXPECT_TRUE(logger.isThreadSafe());
    
    logger.setThreadSafe(false);
    EXPECT_FALSE(logger.isThreadSafe());
}

// 测试日志系统的刷新功能
TEST(LoggerTest, Flush) {
    Logger& logger = Logger::getInstance();
    
    // 测试刷新功能
    logger.flush();
}
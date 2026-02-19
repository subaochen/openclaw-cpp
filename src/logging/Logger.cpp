#include "logging/Logger.h"
#include <thread>
#include <sys/stat.h>
#include <filesystem>

namespace openclaw {

Logger::Logger()
    : logLevel_(LogLevel::INFO),
      consoleOutputEnabled_(true),
      fileOutputEnabled_(true),
      maxFileSize_(10 * 1024 * 1024),  // 10MB
      maxBackupFiles_(5),
      format_("[%timestamp%] [%level%] [%thread%] %message%"),
      threadSafe_(true),
      initialized_(false) {
}

Logger::~Logger() {
    shutdown();
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

bool Logger::initialize(const std::string& logFile) {
    if (initialized_) {
        return true;
    }

    if (logFile.empty()) {
        logFileName_ = "openclaw.log";
    } else {
        logFileName_ = logFile;
    }

    // 打开日志文件
    logFile_.open(logFileName_, std::ios::out | std::ios::app);
    if (!logFile_.is_open()) {
        std::cerr << "Failed to open log file: " << logFileName_ << std::endl;
        return false;
    }

    initialized_ = true;
    return true;
}

void Logger::shutdown() {
    if (!initialized_) {
        return;
    }

    initialized_ = false;
    if (logFile_.is_open()) {
        logFile_.flush();
        logFile_.close();
    }
}

void Logger::log(LogLevel level, const std::string& message) {
    if (!initialized_) {
        initialize();
    }

    if (level < logLevel_) {
        return;
    }

    std::string formattedMessage = formatMessage(level, message);

    if (threadSafe_) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (consoleOutputEnabled_) {
            writeToConsole(formattedMessage);
        }
        if (fileOutputEnabled_) {
            writeToFile(formattedMessage);
        }
    } else {
        if (consoleOutputEnabled_) {
            writeToConsole(formattedMessage);
        }
        if (fileOutputEnabled_) {
            writeToFile(formattedMessage);
        }
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warning(const std::string& message) {
    log(LogLevel::WARNING, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::log(LogLevel level, const std::string& tag, const std::string& message) {
    log(level, "[" + tag + "] " + message);
}

void Logger::debug(const std::string& tag, const std::string& message) {
    log(LogLevel::DEBUG, tag, message);
}

void Logger::info(const std::string& tag, const std::string& message) {
    log(LogLevel::INFO, tag, message);
}

void Logger::warning(const std::string& tag, const std::string& message) {
    log(LogLevel::WARNING, tag, message);
}

void Logger::error(const std::string& tag, const std::string& message) {
    log(LogLevel::ERROR, tag, message);
}

void Logger::critical(const std::string& tag, const std::string& message) {
    log(LogLevel::CRITICAL, tag, message);
}

void Logger::critical(const std::string& message) {
    log(LogLevel::CRITICAL, message);
}

void Logger::setLogLevel(LogLevel level) {
    if (threadSafe_) {
        std::lock_guard<std::mutex> lock(mutex_);
        logLevel_ = level;
    } else {
        logLevel_ = level;
    }
}

LogLevel Logger::getLogLevel() const {
    return logLevel_;
}

void Logger::setConsoleOutputEnabled(bool enabled) {
    if (threadSafe_) {
        std::lock_guard<std::mutex> lock(mutex_);
        consoleOutputEnabled_ = enabled;
    } else {
        consoleOutputEnabled_ = enabled;
    }
}

bool Logger::isConsoleOutputEnabled() const {
    return consoleOutputEnabled_;
}

void Logger::setFileOutputEnabled(bool enabled) {
    if (threadSafe_) {
        std::lock_guard<std::mutex> lock(mutex_);
        fileOutputEnabled_ = enabled;
    } else {
        fileOutputEnabled_ = enabled;
    }
}

bool Logger::isFileOutputEnabled() const {
    return fileOutputEnabled_;
}

void Logger::setLogFile(const std::string& logFile) {
    if (threadSafe_) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (logFile_.is_open()) {
            logFile_.flush();
            logFile_.close();
        }
        
        logFileName_ = logFile;
        logFile_.open(logFileName_, std::ios::out | std::ios::app);
        
        if (!logFile_.is_open()) {
            std::cerr << "Failed to open log file: " << logFileName_ << std::endl;
        }
    } else {
        if (logFile_.is_open()) {
            logFile_.flush();
            logFile_.close();
        }
        
        logFileName_ = logFile;
        logFile_.open(logFileName_, std::ios::out | std::ios::app);
        
        if (!logFile_.is_open()) {
            std::cerr << "Failed to open log file: " << logFileName_ << std::endl;
        }
    }
}

std::string Logger::getLogFile() const {
    return logFileName_;
}

void Logger::setMaxFileSize(size_t maxSize) {
    if (threadSafe_) {
        std::lock_guard<std::mutex> lock(mutex_);
        maxFileSize_ = maxSize;
    } else {
        maxFileSize_ = maxSize;
    }
}

size_t Logger::getMaxFileSize() const {
    return maxFileSize_;
}

void Logger::setMaxBackupFiles(int maxFiles) {
    if (threadSafe_) {
        std::lock_guard<std::mutex> lock(mutex_);
        maxBackupFiles_ = maxFiles;
    } else {
        maxBackupFiles_ = maxFiles;
    }
}

int Logger::getMaxBackupFiles() const {
    return maxBackupFiles_;
}

void Logger::setFormat(const std::string& format) {
    if (threadSafe_) {
        std::lock_guard<std::mutex> lock(mutex_);
        format_ = format;
    } else {
        format_ = format;
    }
}

std::string Logger::getFormat() const {
    return format_;
}

void Logger::setThreadSafe(bool threadSafe) {
    if (threadSafe_) {
        std::lock_guard<std::mutex> lock(mutex_);
        threadSafe_ = threadSafe;
    } else {
        threadSafe_ = threadSafe;
    }
}

bool Logger::isThreadSafe() const {
    return threadSafe_;
}

void Logger::flush() {
    if (threadSafe_) {
        std::lock_guard<std::mutex> lock(mutex_);
        logFile_.flush();
    } else {
        logFile_.flush();
    }
}

std::string Logger::formatMessage(LogLevel level, const std::string& message) const {
    std::string formattedMessage = format_;
    
    // 替换格式占位符
    size_t pos;
    
    pos = formattedMessage.find("%timestamp%");
    if (pos != std::string::npos) {
        formattedMessage.replace(pos, 11, getTimestamp());
    }
    
    pos = formattedMessage.find("%level%");
    if (pos != std::string::npos) {
        formattedMessage.replace(pos, 7, logLevelToString(level));
    }
    
    pos = formattedMessage.find("%thread%");
    if (pos != std::string::npos) {
        formattedMessage.replace(pos, 8, getCurrentThreadId());
    }
    
    pos = formattedMessage.find("%message%");
    if (pos != std::string::npos) {
        formattedMessage.replace(pos, 9, message);
    }
    
    return formattedMessage;
}

std::string Logger::logLevelToString(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG:
            return "DEBUG";
        case LogLevel::INFO:
            return "INFO";
        case LogLevel::WARNING:
            return "WARNING";
        case LogLevel::ERROR:
            return "ERROR";
        case LogLevel::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

void Logger::rotateLogFile() {
    logFile_.flush();
    logFile_.close();
    
    // 旋转备份文件
    for (int i = maxBackupFiles_ - 1; i > 0; --i) {
        std::string oldFileName = logFileName_ + "." + std::to_string(i);
        std::string newFileName = logFileName_ + "." + std::to_string(i + 1);
        
        try {
            if (std::filesystem::exists(oldFileName)) {
                if (std::filesystem::exists(newFileName)) {
                    std::filesystem::remove(newFileName);
                }
                std::filesystem::rename(oldFileName, newFileName);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to rotate log file: " << e.what() << std::endl;
        }
    }
    
    // 重命名当前日志文件
    std::string backupFileName = logFileName_ + ".1";
    try {
        if (std::filesystem::exists(backupFileName)) {
            std::filesystem::remove(backupFileName);
        }
        std::filesystem::rename(logFileName_, backupFileName);
    } catch (const std::exception& e) {
        std::cerr << "Failed to rename log file: " << e.what() << std::endl;
    }
    
    // 重新打开日志文件
    logFile_.open(logFileName_, std::ios::out | std::ios::app);
}

void Logger::writeToConsole(const std::string& formattedMessage) {
    std::cout << formattedMessage << std::endl;
}

void Logger::writeToFile(const std::string& formattedMessage) {
    logFile_ << formattedMessage << std::endl;
    
    // 检查是否需要旋转日志文件
    struct stat fileStat;
    if (stat(logFileName_.c_str(), &fileStat) == 0 && fileStat.st_size > maxFileSize_) {
        rotateLogFile();
    }
}

std::string Logger::getTimestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time);
    
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") 
        << "." << std::setw(3) << std::setfill('0') << ms.count();
    
    return oss.str();
}

std::string Logger::getCurrentThreadId() const {
    std::ostringstream oss;
    oss << std::this_thread::get_id();
    return oss.str();
}

} // namespace openclaw
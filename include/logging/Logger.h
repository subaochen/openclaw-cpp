#ifndef OPENCLAW_LOGGER_H
#define OPENCLAW_LOGGER_H

#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <atomic>
#include <vector>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace openclaw {

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    static Logger& getInstance();

    bool initialize(const std::string& logFile = "");
    void shutdown();

    void log(LogLevel level, const std::string& message);
    void log(LogLevel level, const std::string& tag, const std::string& message);
    void debug(const std::string& message);
    void debug(const std::string& tag, const std::string& message);
    void info(const std::string& message);
    void info(const std::string& tag, const std::string& message);
    void warning(const std::string& message);
    void warning(const std::string& tag, const std::string& message);
    void error(const std::string& message);
    void error(const std::string& tag, const std::string& message);
    void critical(const std::string& message);
    void critical(const std::string& tag, const std::string& message);

    void setLogLevel(LogLevel level);
    LogLevel getLogLevel() const;

    void setConsoleOutputEnabled(bool enabled);
    bool isConsoleOutputEnabled() const;

    void setFileOutputEnabled(bool enabled);
    bool isFileOutputEnabled() const;

    void setLogFile(const std::string& logFile);
    std::string getLogFile() const;

    void setMaxFileSize(size_t maxSize);
    size_t getMaxFileSize() const;

    void setMaxBackupFiles(int maxFiles);
    int getMaxBackupFiles() const;

    void setFormat(const std::string& format);
    std::string getFormat() const;

    void setThreadSafe(bool threadSafe);
    bool isThreadSafe() const;

    void flush();

private:
    Logger();
    ~Logger();

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    Logger(Logger&&) = delete;
    Logger& operator=(Logger&&) = delete;

    std::string formatMessage(LogLevel level, const std::string& message) const;
    std::string logLevelToString(LogLevel level) const;
    void rotateLogFile();
    void writeToConsole(const std::string& formattedMessage);
    void writeToFile(const std::string& formattedMessage);
    std::string getTimestamp() const;
    std::string getCurrentThreadId() const;

    std::ofstream logFile_;
    std::string logFileName_;
    LogLevel logLevel_;
    bool consoleOutputEnabled_;
    bool fileOutputEnabled_;
    size_t maxFileSize_;
    int maxBackupFiles_;
    std::string format_;
    bool threadSafe_;
    mutable std::mutex mutex_;
    std::atomic<bool> initialized_;
};

} // namespace openclaw

#endif // OPENCLAW_LOGGER_H
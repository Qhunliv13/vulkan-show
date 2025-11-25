#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>

// 日志级别
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

// 统一日志系统 - 单例模式，线程安全
class Logger {
public:
    // 获取单例实例
    static Logger& GetInstance();
    
    // 初始化日志系统
    bool Initialize(const std::string& logFile = "");
    
    // 清理日志系统
    void Shutdown();
    
    // 日志输出函数
    void Log(LogLevel level, const std::string& message, const char* file = nullptr, int line = 0);
    
    // 便捷函数
    void Debug(const std::string& message, const char* file = nullptr, int line = 0);
    void Info(const std::string& message, const char* file = nullptr, int line = 0);
    void Warning(const std::string& message, const char* file = nullptr, int line = 0);
    void Error(const std::string& message, const char* file = nullptr, int line = 0);
    void Fatal(const std::string& message, const char* file = nullptr, int line = 0);
    
    // 设置最小日志级别
    void SetMinLevel(LogLevel level) { m_minLevel = level; }
    
    // 是否输出到控制台
    void SetConsoleOutput(bool enable) { m_consoleOutput = enable; }

private:
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
    
    std::string GetLevelString(LogLevel level) const;
    std::string GetTimestamp() const;
    
    std::unique_ptr<std::ofstream> m_logFile;
    LogLevel m_minLevel = LogLevel::Debug;
    bool m_consoleOutput = true;
    bool m_initialized = false;
    std::mutex m_mutex;  // 线程安全
};

// 便捷宏
#define LOG_DEBUG(msg) Logger::GetInstance().Debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::GetInstance().Info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::GetInstance().Warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::GetInstance().Error(msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) Logger::GetInstance().Fatal(msg, __FILE__, __LINE__)


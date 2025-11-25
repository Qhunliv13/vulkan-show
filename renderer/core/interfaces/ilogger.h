#pragma once

#include <string>

// 日志级别
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

// 日志接口 - 用于依赖注入，替代单例
class ILogger {
public:
    virtual ~ILogger() = default;
    
    // 初始化日志系统
    virtual bool Initialize(const std::string& logFile = "") = 0;
    
    // 清理日志系统
    virtual void Shutdown() = 0;
    
    // 日志输出函数
    virtual void Log(LogLevel level, const std::string& message, const char* file = nullptr, int line = 0) = 0;
    
    // 便捷函数
    virtual void Debug(const std::string& message, const char* file = nullptr, int line = 0) = 0;
    virtual void Info(const std::string& message, const char* file = nullptr, int line = 0) = 0;
    virtual void Warning(const std::string& message, const char* file = nullptr, int line = 0) = 0;
    virtual void Error(const std::string& message, const char* file = nullptr, int line = 0) = 0;
    virtual void Fatal(const std::string& message, const char* file = nullptr, int line = 0) = 0;
    
    // 设置最小日志级别
    virtual void SetMinLevel(LogLevel level) = 0;
    
    // 是否输出到控制台
    virtual void SetConsoleOutput(bool enable) = 0;
};


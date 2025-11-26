#pragma once

#include <string>  // 2. 系统头文件

/**
 * 日志级别枚举
 * 
 * 定义日志输出的不同级别，用于过滤和控制日志输出
 */
enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
};

/**
 * 日志接口 - 用于依赖注入，替代单例
 * 
 * 职责：提供统一的日志输出接口，支持多种日志级别和输出目标
 * 设计：通过接口抽象，支持多种日志实现（文件日志、控制台日志等）
 * 
 * 使用方式：
 * 1. 通过依赖注入获取接口指针
 * 2. 使用 Log() 或便捷方法（Debug、Info、Warning等）输出日志
 * 3. 通过 SetMinLevel() 控制日志级别
 */
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


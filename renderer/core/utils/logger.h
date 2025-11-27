#pragma once

#include <fstream>
#include <memory>
#include <mutex>
#include <string>

#include "core/interfaces/ilogger.h"  // 4. 项目头文件

/**
 * 统一日志系统 - 实现 ILogger 接口，支持依赖注入
 * 
 * 职责：提供统一的日志输出接口，支持多种日志级别和输出目标（文件、控制台）
 * 设计：通过接口抽象，支持依赖注入，禁止使用单例
 * 
 * 使用方式：
 * 1. 通过依赖注入获取接口指针
 * 2. 使用 Log() 或便捷方法（Debug、Info、Warning等）输出日志
 * 3. 通过 SetMinLevel() 控制日志级别
 */
class Logger : public ILogger {
public:
    /**
     * 构造函数（支持依赖注入）
     */
    Logger() = default;
    
    /**
     * 析构函数
     */
    ~Logger() = default;
    
    // ILogger 接口实现
    bool Initialize(const std::string& logFile = "") override;
    void Shutdown() override;
    void Log(LogLevel level, const std::string& message, const char* file = nullptr, int line = 0) override;
    void Debug(const std::string& message, const char* file = nullptr, int line = 0) override;
    void Info(const std::string& message, const char* file = nullptr, int line = 0) override;
    void Warning(const std::string& message, const char* file = nullptr, int line = 0) override;
    void Error(const std::string& message, const char* file = nullptr, int line = 0) override;
    void Fatal(const std::string& message, const char* file = nullptr, int line = 0) override;
    void SetMinLevel(LogLevel level) override { m_minLevel = level; }
    void SetConsoleOutput(bool enable) override { m_consoleOutput = enable; }

private:
    // 禁止拷贝和赋值
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


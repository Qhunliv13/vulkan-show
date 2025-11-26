#pragma once

#include <string>
#include <fstream>
#include <memory>
#include <mutex>
#include "core/interfaces/ilogger.h"

// 统一日志系统 - 实现 ILogger 接口，支持依赖注入
// 保留单例方法以支持向后兼容，但推荐使用依赖注入
class Logger : public ILogger {
public:
    // 获取单例实例（向后兼容）
    static Logger& GetInstance();
    
    // 公共构造函数（支持依赖注入）
    Logger() = default;
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

// 便捷宏
#define LOG_DEBUG(msg) Logger::GetInstance().Debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::GetInstance().Info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::GetInstance().Warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::GetInstance().Error(msg, __FILE__, __LINE__)
#define LOG_FATAL(msg) Logger::GetInstance().Fatal(msg, __FILE__, __LINE__)


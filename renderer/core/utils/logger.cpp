#include "core/utils/logger.h"  // 1. 对应头文件

#include <windows.h>            // 2. 系统头文件
#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>

bool Logger::Initialize(const std::string& logFile) {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_initialized) {
        return true;
    }
    
    if (!logFile.empty()) {
        // 使用 app 模式追加，如果文件不存在会创建
        m_logFile = std::make_unique<std::ofstream>(logFile, std::ios::app | std::ios::out);
        if (!m_logFile->is_open()) {
            // 尝试创建新文件
            m_logFile = std::make_unique<std::ofstream>(logFile, std::ios::out | std::ios::trunc);
            if (!m_logFile->is_open()) {
                return false;
            }
        }
        // 确保文件流处于良好状态
        m_logFile->clear();
    }
    
    m_initialized = true;
    
    // 直接写入初始消息，避免递归
    std::string initMsg = "[" + GetTimestamp() + "] [INFO ] Logger initialized\n";
    if (m_consoleOutput) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE && hConsole != nullptr) {
            std::cout << initMsg;
        }
    }
    if (m_logFile && m_logFile->is_open()) {
        *m_logFile << initMsg;
        m_logFile->flush();
        // 验证写入是否成功
        if (m_logFile->fail()) {
            m_logFile->clear();
        }
    }
    
    return true;
}

void Logger::Shutdown() {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (m_logFile && m_logFile->is_open()) {
        // 直接写入，避免递归调用Info()
        *m_logFile << "[" << GetTimestamp() << "] [INFO ] Logger shutting down" << std::endl;
        m_logFile->flush();
        m_logFile->close();
    }
    
    m_initialized = false;
}

void Logger::Log(LogLevel level, const std::string& message, const char* file, int line) {
    if (level < m_minLevel) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_mutex);
    
    // 即使未初始化也输出到控制台（用于调试）
    if (!m_initialized) {
        // 如果日志系统未初始化，至少输出到控制台
        if (m_consoleOutput) {
            std::cout << "[UNINIT] " << message << std::endl;
        }
        return;
    }
    
    std::stringstream ss;
    ss << "[" << GetTimestamp() << "] "
       << "[" << GetLevelString(level) << "] ";
    
    if (file) {
        // 只显示文件名，不显示完整路径
        std::string fileName = file;
        size_t pos = fileName.find_last_of("\\/");
        if (pos != std::string::npos) {
            fileName = fileName.substr(pos + 1);
        }
        ss << "[" << fileName << ":" << line << "] ";
    }
    
    ss << message;
    
    std::string logMessage = ss.str();
    
    // 输出到控制台（安全检查）
    if (m_consoleOutput) {
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hConsole != INVALID_HANDLE_VALUE && hConsole != nullptr) {
            WORD color = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;  // 默认白色
            
            switch (level) {
                case LogLevel::Debug:
                    color = FOREGROUND_INTENSITY | FOREGROUND_BLUE;
                    break;
                case LogLevel::Info:
                    color = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
                    break;
                case LogLevel::Warning:
                    color = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN;
                    break;
                case LogLevel::Error:
                case LogLevel::Fatal:
                    color = FOREGROUND_INTENSITY | FOREGROUND_RED;
                    break;
            }
            
            SetConsoleTextAttribute(hConsole, color);
            std::cout << logMessage << std::endl;
            SetConsoleTextAttribute(hConsole, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        } else {
            // 控制台不可用，只输出到文件
            std::cout << logMessage << std::endl;
        }
    }
    
    // 输出到文件
    if (m_logFile && m_logFile->is_open()) {
        *m_logFile << logMessage << std::endl;
        m_logFile->flush();
        // 检查文件流状态
        if (m_logFile->fail() || m_logFile->bad()) {
            // 文件写入失败，但继续执行
            m_logFile->clear();
        }
    }
    
    // Fatal错误显示消息框
    if (level == LogLevel::Fatal) {
        MessageBoxA(nullptr, message.c_str(), "Fatal Error", MB_OK | MB_ICONERROR);
    }
}

void Logger::Debug(const std::string& message, const char* file, int line) {
    Log(LogLevel::Debug, message, file, line);
}

void Logger::Info(const std::string& message, const char* file, int line) {
    Log(LogLevel::Info, message, file, line);
}

void Logger::Warning(const std::string& message, const char* file, int line) {
    Log(LogLevel::Warning, message, file, line);
}

void Logger::Error(const std::string& message, const char* file, int line) {
    Log(LogLevel::Error, message, file, line);
}

void Logger::Fatal(const std::string& message, const char* file, int line) {
    Log(LogLevel::Fatal, message, file, line);
}

std::string Logger::GetLevelString(LogLevel level) const {
    switch (level) {
        case LogLevel::Debug:   return "DEBUG";
        case LogLevel::Info:    return "INFO ";
        case LogLevel::Warning: return "WARN ";
        case LogLevel::Error:   return "ERROR";
        case LogLevel::Fatal:   return "FATAL";
        default:                return "UNKNOWN";
    }
}

std::string Logger::GetTimestamp() const {
    auto now = std::time(nullptr);
    auto tm = *std::localtime(&now);
    
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}


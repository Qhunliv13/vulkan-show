#pragma once

#include <functional>  // 2. 系统头文件
#include <memory>  // 2. 系统头文件
#include <string>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件

// 初始化阶段结果
struct PhaseResult {
    bool success;
    std::string errorMessage;
    
    static PhaseResult Success() { return {true, ""}; }
    static PhaseResult Failure(const std::string& msg) { return {false, msg}; }
};

// 初始化阶段接口
class IInitializationPhase {
public:
    virtual ~IInitializationPhase() = default;
    
    // 执行初始化
    virtual PhaseResult Initialize() = 0;
    
    // 清理资源（用于回滚）
    virtual void Cleanup() = 0;
    
    // 获取阶段名称（用于日志）
    virtual std::string GetName() const = 0;
    
    // 检查是否已初始化
    virtual bool IsInitialized() const = 0;
};

// 初始化阶段管理器 - 简化依赖管理和回滚逻辑
class InitializationPhaseManager {
public:
    InitializationPhaseManager();
    ~InitializationPhaseManager();
    
    // 添加初始化阶段（按依赖顺序添加）
    void AddPhase(std::unique_ptr<IInitializationPhase> phase);
    
    // 执行所有阶段的初始化
    bool InitializeAll();
    
    // 清理所有已初始化的阶段（按逆序）
    void CleanupAll();
    
    // 获取已初始化的阶段数量
    size_t GetInitializedCount() const { return m_initializedCount; }
    
private:
    std::vector<std::unique_ptr<IInitializationPhase>> m_phases;
    size_t m_initializedCount = 0;
};


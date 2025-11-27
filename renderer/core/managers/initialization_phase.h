#pragma once

#include <functional>  // 2. 系统头文件
#include <memory>  // 2. 系统头文件
#include <string>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件

/**
 * 初始化阶段结果 - 封装阶段初始化的结果信息
 * 
 * 职责：统一返回阶段初始化的成功状态和错误信息
 */
struct PhaseResult {
    bool success;
    std::string errorMessage;
    
    static PhaseResult Success() { return {true, ""}; }
    static PhaseResult Failure(const std::string& msg) { return {false, msg}; }
};

/**
 * 初始化阶段接口 - 定义初始化阶段的通用接口
 * 
 * 职责：定义初始化阶段的统一接口，支持阶段化的初始化流程
 * 设计：所有初始化阶段必须实现此接口，支持清理和状态查询
 */
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

/**
 * 初始化阶段管理器 - 简化依赖管理和回滚逻辑
 * 
 * 职责：管理多个初始化阶段的执行顺序和回滚逻辑
 * 设计：按添加顺序执行阶段，失败时自动回滚已初始化的阶段
 * 
 * 使用方式：
 * 1. 创建 InitializationPhaseManager 实例
 * 2. 调用 AddPhase() 按依赖顺序添加初始化阶段
 * 3. 调用 InitializeAll() 执行所有阶段的初始化
 * 4. 如果失败，自动调用 CleanupAll() 清理已初始化的阶段
 * 5. 手动调用 CleanupAll() 清理所有阶段
 */
class InitializationPhaseManager {
public:
    InitializationPhaseManager();
    ~InitializationPhaseManager();
    
    /**
     * 添加初始化阶段（按依赖顺序添加）
     * 
     * 将初始化阶段添加到管理器中，必须按依赖顺序添加
     * 
     * @param phase 初始化阶段（转移所有权）
     */
    void AddPhase(std::unique_ptr<IInitializationPhase> phase);
    
    /**
     * 执行所有阶段的初始化
     * 
     * 按添加顺序执行所有阶段的初始化，如果任何阶段失败，自动清理已初始化的阶段
     * 
     * @return bool 如果所有阶段初始化成功返回 true，否则返回 false
     */
    bool InitializeAll();
    
    /**
     * 清理所有已初始化的阶段（按逆序）
     * 
     * 按初始化顺序的逆序清理阶段，确保依赖关系正确
     */
    void CleanupAll();
    
    /**
     * 获取已初始化的阶段数量
     * 
     * @return size_t 已成功初始化的阶段数量
     */
    size_t GetInitializedCount() const { return m_initializedCount; }
    
private:
    std::vector<std::unique_ptr<IInitializationPhase>> m_phases;  // 初始化阶段列表（拥有所有权）
    size_t m_initializedCount = 0;  // 已成功初始化的阶段数量
};


#pragma once

#include <string>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件

/**
 * 初始化结果 - 统一错误处理
 * 
 * 职责：封装初始化操作的结果信息，包括成功状态、错误消息和失败步骤索引
 * 设计：提供静态工厂方法创建成功/失败结果，支持转换为 bool 用于条件判断
 */
struct InitializationResult {
    bool success = false;
    std::string errorMessage;
    int stepIndex = -1;  // 失败的步骤索引（用于定位失败位置）
    
    InitializationResult() = default;
    InitializationResult(bool s, const std::string& msg = "", int step = -1) 
        : success(s), errorMessage(msg), stepIndex(step) {}
    
    static InitializationResult Success() { return InitializationResult(true); }
    static InitializationResult Failure(const std::string& msg, int step = -1) {
        return InitializationResult(false, msg, step);
    }
    
    // 转换为 bool，用于条件判断
    explicit operator bool() const { return success; }
};

/**
 * 初始化步骤接口 - 定义初始化步骤的通用接口
 * 
 * 职责：定义初始化步骤的统一接口，支持依赖声明和清理
 * 设计：所有初始化步骤必须实现此接口，声明其依赖关系以支持拓扑排序
 */
class IInitializationStep {
public:
    virtual ~IInitializationStep() = default;
    
    // 执行初始化步骤
    virtual InitializationResult Initialize() = 0;
    
    // 清理步骤（用于回滚）
    virtual void Cleanup() = 0;
    
    // 获取步骤名称（用于日志）
    virtual const char* GetName() const = 0;
    
    // 获取依赖的步骤名称列表
    virtual std::vector<const char*> GetDependencies() const = 0;
};


#pragma once

#include <string>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件

// 初始化结果 - 统一错误处理
struct InitializationResult {
    bool success = false;
    std::string errorMessage;
    int stepIndex = -1;  // 失败的步骤索引
    
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

// 初始化步骤接口
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


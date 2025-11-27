#pragma once

#include <memory>  // 2. 系统头文件
#include <string>  // 2. 系统头文件
#include <unordered_map>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件
#include "initialization_result.h"  // 4. 项目头文件（初始化结果）

// 初始化链 - 管理初始化顺序和依赖关系
class InitializationChain {
public:
    InitializationChain();
    ~InitializationChain();
    
    // 添加初始化步骤
    void AddStep(std::unique_ptr<IInitializationStep> step);
    
    // 执行所有初始化步骤（按依赖顺序）
    InitializationResult Execute();
    
    // 清理所有已初始化的步骤（按逆序）
    void Cleanup(int initializedCount = -1);
    
    // 获取已初始化的步骤数量
    int GetInitializedCount() const { return m_initializedCount; }

private:
    // 拓扑排序，确定初始化顺序
    std::vector<int> TopologicalSort();
    
    // 查找步骤索引
    int FindStepIndex(const char* name) const;
    
    std::vector<std::unique_ptr<IInitializationStep>> m_steps;
    std::unordered_map<std::string, int> m_stepNameToIndex;
    int m_initializedCount = 0;
};


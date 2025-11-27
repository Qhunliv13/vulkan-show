#pragma once

#include <memory>  // 2. 系统头文件
#include <string>  // 2. 系统头文件
#include <unordered_map>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件
#include "initialization_result.h"  // 4. 项目头文件（初始化结果）

/**
 * 初始化链 - 管理初始化顺序和依赖关系
 * 
 * 职责：管理多个初始化步骤的执行顺序，通过拓扑排序自动确定依赖顺序
 * 设计：使用依赖图进行拓扑排序，确保依赖步骤在依赖项之后执行
 * 
 * 使用方式：
 * 1. 创建 InitializationChain 实例
 * 2. 调用 AddStep() 添加初始化步骤（每个步骤声明其依赖）
 * 3. 调用 Execute() 执行所有步骤（自动按依赖顺序执行）
 * 4. 如果失败，自动按逆序清理已初始化的步骤
 * 5. 调用 Cleanup() 手动清理所有步骤
 */
class InitializationChain {
public:
    InitializationChain();
    ~InitializationChain();
    
    /**
     * 添加初始化步骤
     * 
     * 将初始化步骤添加到链中，步骤必须声明其依赖关系
     * 
     * @param step 初始化步骤（转移所有权）
     */
    void AddStep(std::unique_ptr<IInitializationStep> step);
    
    /**
     * 执行所有初始化步骤（按依赖顺序）
     * 
     * 使用拓扑排序确定执行顺序，确保依赖步骤在依赖项之后执行
     * 如果任何步骤失败，自动清理已初始化的步骤
     * 
     * @return InitializationResult 初始化结果，包含成功状态和错误信息
     */
    InitializationResult Execute();
    
    /**
     * 清理所有已初始化的步骤（按逆序）
     * 
     * 按初始化顺序的逆序清理步骤，确保依赖关系正确
     * 
     * @param initializedCount 要清理的步骤数量（-1 表示清理所有已初始化的步骤）
     */
    void Cleanup(int initializedCount = -1);
    
    /**
     * 获取已初始化的步骤数量
     * 
     * @return int 已成功初始化的步骤数量
     */
    int GetInitializedCount() const { return m_initializedCount; }

private:
    /**
     * 拓扑排序，确定初始化顺序
     * 
     * 根据步骤的依赖关系进行拓扑排序，返回按依赖顺序排列的步骤索引
     * 
     * @return std::vector<int> 按依赖顺序排列的步骤索引，如果存在循环依赖则返回空向量
     */
    std::vector<int> TopologicalSort();
    
    /**
     * 查找步骤索引
     * 
     * 根据步骤名称查找其在步骤列表中的索引
     * 
     * @param name 步骤名称
     * @return int 步骤索引，如果未找到则返回 -1
     */
    int FindStepIndex(const char* name) const;
    
    std::vector<std::unique_ptr<IInitializationStep>> m_steps;  // 初始化步骤列表（拥有所有权）
    std::unordered_map<std::string, int> m_stepNameToIndex;  // 步骤名称到索引的映射
    int m_initializedCount = 0;  // 已成功初始化的步骤数量
};


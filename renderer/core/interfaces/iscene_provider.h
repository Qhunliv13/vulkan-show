#pragma once

#include "core/config/constants.h"  // 4. 项目头文件（配置）

/**
 * 场景提供者接口 - 用于解耦场景管理与事件处理
 * 
 * 职责：提供场景状态访问接口，支持事件处理基于场景状态决策
 * 设计：通过接口抽象，解耦事件处理与场景管理实现
 * 
 * 使用方式：
 * 1. 通过依赖注入获取接口指针
 * 2. 使用 GetState() 获取当前场景状态
 * 3. 使用 ShouldHandleInput() 检查是否应该处理输入
 */
class ISceneProvider {
public:
    virtual ~ISceneProvider() = default;
    
    // 获取当前应用状态
    virtual AppState GetState() const = 0;
    
    // 检查是否应该处理输入事件（基于当前状态）
    virtual bool ShouldHandleInput() const = 0;
    
    // 切换到Loading状态（用于ESC键返回）
    virtual void SwitchToLoading() = 0;
    
    // 检查pipeline是否已创建（用于渲染判断）
    virtual bool IsLoadingCubesPipelineCreated() const = 0;
};


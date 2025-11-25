#pragma once

#include "core/constants.h"

// 场景提供者接口 - 用于解耦场景管理与事件处理
class ISceneProvider {
public:
    virtual ~ISceneProvider() = default;
    
    // 获取当前应用状态
    virtual AppState GetState() const = 0;
    
    // 检查是否应该处理输入事件（基于当前状态）
    virtual bool ShouldHandleInput() const = 0;
};


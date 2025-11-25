#pragma once

#include "core/config/constants.h"

// 前向声明
class IRenderer;

// UI管理器接口 - 用于解耦EventManager对UIManager的直接依赖
class IUIManager {
public:
    virtual ~IUIManager() = default;
    
    // 处理鼠标点击
    virtual bool HandleClick(float x, float y) = 0;
    
    // 处理鼠标移动
    virtual void HandleMouseMove(float x, float y) = 0;
    
    // 处理鼠标释放
    virtual void HandleMouseUp() = 0;
    
    // 处理窗口大小变化
    virtual void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) = 0;
};


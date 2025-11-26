#pragma once

#include "core/config/constants.h"

// 前向声明
class IRenderer;

// 窗口大小变化处理接口 - 用于解耦窗口大小变化事件处理
class IWindowResizeHandler {
public:
    virtual ~IWindowResizeHandler() = default;
    
    // 处理窗口大小变化
    virtual void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) = 0;
};


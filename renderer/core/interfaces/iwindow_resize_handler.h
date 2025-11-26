#pragma once

#include "core/config/constants.h"  // 4. 项目头文件（配置）

// 前向声明
class IRenderer;

/**
 * 窗口大小变化处理接口 - 用于解耦窗口大小变化事件处理
 * 
 * 职责：提供窗口大小变化处理接口，支持多种处理实现
 * 设计：通过接口抽象，解耦事件处理与窗口大小变化处理实现
 * 
 * 使用方式：
 * 1. 通过依赖注入获取接口指针
 * 2. 使用 HandleWindowResize() 处理窗口大小变化事件
 */
class IWindowResizeHandler {
public:
    virtual ~IWindowResizeHandler() = default;
    
    // 处理窗口大小变化
    virtual void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) = 0;
};


#pragma once

#include "core/config/constants.h"  // 4. 项目头文件（配置）

// 前向声明
class IRenderer;

/**
 * UI管理器接口 - 用于解耦EventManager对UIManager的直接依赖
 * 
 * 职责：提供UI交互处理接口，处理鼠标事件和窗口大小变化
 * 设计：通过接口抽象，解耦事件处理与UI管理实现
 * 
 * 使用方式：
 * 1. 通过依赖注入获取接口指针
 * 2. 使用 HandleClick()、HandleMouseMove() 等方法处理UI事件
 */
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


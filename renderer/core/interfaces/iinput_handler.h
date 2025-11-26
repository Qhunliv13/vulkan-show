#pragma once

#include "core/config/constants.h"  // 4. 项目头文件（配置）

/**
 * 输入处理器接口 - 用于解耦EventManager对InputHandler的直接依赖
 * 
 * 职责：提供输入坐标转换接口，将窗口坐标转换为UI坐标
 * 设计：通过接口抽象，解耦事件处理与输入处理实现
 * 
 * 使用方式：
 * 1. 通过依赖注入获取接口指针
 * 2. 使用 ConvertWindowToUICoords() 转换坐标
 * 3. 使用 SetStretchMode() 更新拉伸模式
 */
class IInputHandler {
public:
    virtual ~IInputHandler() = default;
    
    // 将窗口坐标转换为UI坐标系坐标
    virtual void ConvertWindowToUICoords(int windowX, int windowY, float& uiX, float& uiY) = 0;
    
    // 设置拉伸模式（窗口大小变化时可能需要更新）
    virtual void SetStretchMode(StretchMode mode) = 0;
};


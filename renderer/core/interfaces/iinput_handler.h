#pragma once

#include "core/config/constants.h"

// 输入处理器接口 - 用于解耦EventManager对InputHandler的直接依赖
class IInputHandler {
public:
    virtual ~IInputHandler() = default;
    
    // 将窗口坐标转换为UI坐标系坐标
    virtual void ConvertWindowToUICoords(int windowX, int windowY, float& uiX, float& uiY) = 0;
    
    // 设置拉伸模式（窗口大小变化时可能需要更新）
    virtual void SetStretchMode(StretchMode mode) = 0;
};


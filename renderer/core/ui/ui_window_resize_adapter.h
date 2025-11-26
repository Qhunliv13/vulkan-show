#pragma once

#include "core/interfaces/iwindow_resize_handler.h"

// 前向声明
class UIManager;

// UI窗口大小变化处理适配器 - 实现 IWindowResizeHandler 接口，委托给 UIManager
// 职责：将窗口大小变化处理接口与 UI 管理器解耦，实现接口职责单一原则
class UIWindowResizeAdapter : public IWindowResizeHandler {
public:
    explicit UIWindowResizeAdapter(UIManager* uiManager);
    
    // IWindowResizeHandler 接口实现
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;

private:
    UIManager* m_uiManager = nullptr;
};


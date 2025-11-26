#include "core/ui/ui_window_resize_adapter.h"  // 1. 对应头文件

#include "core/ui/ui_manager.h"  // 4. 项目头文件（UI管理器）

UIWindowResizeAdapter::UIWindowResizeAdapter(UIManager* uiManager)
    : m_uiManager(uiManager) {
}

void UIWindowResizeAdapter::HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) {
    if (m_uiManager) {
        m_uiManager->HandleWindowResize(stretchMode, renderer);
    }
}


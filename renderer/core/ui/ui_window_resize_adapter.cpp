#include "core/ui/ui_window_resize_adapter.h"  // 1. 对应头文件

// 2. 系统头文件（本文件不需要）
// 3. 第三方库头文件（本文件不需要）

#include "core/ui/ui_manager.h"  // 4. 项目头文件（UI管理器）

UIWindowResizeAdapter::UIWindowResizeAdapter(UIManager* uiManager)
    : m_uiManager(uiManager) {
}

void UIWindowResizeAdapter::HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) {
    if (m_uiManager) {
        m_uiManager->HandleWindowResize(stretchMode, renderer);
    }
}


#include "core/ui/ui_window_resize_adapter.h"
#include "core/ui/ui_manager.h"

UIWindowResizeAdapter::UIWindowResizeAdapter(UIManager* uiManager)
    : m_uiManager(uiManager) {
}

void UIWindowResizeAdapter::HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) {
    if (m_uiManager) {
        m_uiManager->HandleWindowResize(stretchMode, renderer);
    }
}


#include "core/window_message_handler.h"
#include "core/event_manager.h"

WindowMessageHandler::WindowMessageHandler() {
}

WindowMessageHandler::~WindowMessageHandler() {
}

void WindowMessageHandler::Initialize(EventManager* eventManager, class Window* window, 
                                     StretchMode stretchMode, class IRenderer* renderer) {
    m_eventManager = eventManager;
    m_stretchMode = stretchMode;
    // 注意：此方法已废弃，实际功能已迁移到 EventManager
}

bool WindowMessageHandler::ProcessMessages() {
    // 注意：此方法已废弃，使用 EventManager::ProcessMessages() 代替
    if (m_eventManager) {
        return m_eventManager->ProcessMessages(m_stretchMode);
    }
    return false;
}


#include "core/managers/event_manager.h"
#include "core/interfaces/iscene_provider.h"
#include "core/utils/logger.h"
#include "core/utils/event_bus.h"
#include "core/interfaces/ievent_bus.h"
#include "core/managers/scene_manager.h"
#include "core/interfaces/iconfig_provider.h"
#include "window/window.h"
#include <windows.h>

EventManager::EventManager() {
}

EventManager::~EventManager() {
}

void EventManager::Initialize(IInputHandler* inputHandler, 
                              IUIManager* uiManager, 
                              IRenderer* renderer,
                              Window* window,
                              ISceneProvider* sceneProvider,
                              IEventBus* eventBus) {
    m_inputHandler = inputHandler;
    m_uiManager = uiManager;
    m_renderer = renderer;
    m_window = window;
    m_sceneProvider = sceneProvider;
    m_eventBus = eventBus;
}

void EventManager::HandleMouseClick(int x, int y, StretchMode stretchMode) {
    if (!m_inputHandler || !m_uiManager) {
        return;
    }
    
    // 转换坐标
    float uiX, uiY;
    m_inputHandler->ConvertWindowToUICoords(x, y, uiX, uiY);
    
    if (uiX < 0.0f || uiY < 0.0f) {
        return;
    }
    
    // 更新UI组件位置（确保位置正确）
    if (stretchMode != StretchMode::Fit) {
        m_uiManager->HandleWindowResize(stretchMode, m_renderer);
    }
    
    // 使用 UIManager 的统一接口处理点击
    m_uiManager->HandleClick(uiX, uiY);
}

void EventManager::HandleMouseMove(int x, int y) {
    if (!m_inputHandler || !m_uiManager) {
        return;
    }
    
    // 转换坐标
    float uiX, uiY;
    m_inputHandler->ConvertWindowToUICoords(x, y, uiX, uiY);
    
    // 使用 UIManager 的统一接口处理鼠标移动
    m_uiManager->HandleMouseMove(uiX, uiY);
}

void EventManager::HandleMouseUp() {
    if (!m_uiManager) {
        return;
    }
    
    // 使用 UIManager 的统一接口处理鼠标释放
    m_uiManager->HandleMouseUp();
}

void EventManager::HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) {
    if (m_uiManager) {
        m_uiManager->HandleWindowResize(stretchMode, renderer);
    }
}

bool EventManager::ProcessMessage(const MSG& msg, StretchMode stretchMode) {
    if (msg.message == WM_QUIT) {
        if (m_window) {
            m_window->SetRunning(false);
        }
        return false;
    }
    
    // 处理窗口消息
    if (!HandleWindowMessage(msg, stretchMode)) {
        return false;
    }
    
    // 处理鼠标消息
    HandleMouseMessage(msg, stretchMode);
    
    // 处理键盘消息
    HandleKeyboardMessage(msg);
    
    TranslateMessage(&msg);
    DispatchMessage(&msg);
    
    return true;
}

bool EventManager::ProcessMessages(StretchMode stretchMode) {
    if (!m_window) {
        return false;
    }
    
    MSG msg = {};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (!ProcessMessage(msg, stretchMode)) {
            return false;
        }
    }
    
    return true;
}

bool EventManager::HandleWindowMessage(const MSG& msg, StretchMode stretchMode) {
    switch (msg.message) {
        case WM_SIZE: {
            if (m_renderer) {
                HandleWindowResize(stretchMode, m_renderer);
            }
            if (m_window) {
                InvalidateRect(m_window->GetHandle(), nullptr, FALSE);
            }
            return true;
        }
        default:
            return true;
    }
}

bool EventManager::HandleMouseMessage(const MSG& msg, StretchMode stretchMode) {
    // 使用场景提供者接口检查是否应该处理输入
    if (!m_sceneProvider || !m_sceneProvider->ShouldHandleInput()) {
        return true;
    }
    
    switch (msg.message) {
        case WM_LBUTTONDOWN: {
            POINT pt;
            pt.x = LOWORD(msg.lParam);
            pt.y = HIWORD(msg.lParam);
            HandleMouseClick(pt.x, pt.y, stretchMode);
            return true;
        }
        case WM_MOUSEMOVE: {
            POINT pt;
            pt.x = LOWORD(msg.lParam);
            pt.y = HIWORD(msg.lParam);
            HandleMouseMove(pt.x, pt.y);
            return true;
        }
        case WM_LBUTTONUP: {
            HandleMouseUp();
            return true;
        }
        default:
            return true;
    }
}

bool EventManager::HandleKeyboardMessage(const MSG& msg) {
    // 可以在这里处理键盘消息
    // 目前主要处理在RenderScheduler中
    return true;
}


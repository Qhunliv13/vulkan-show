#include "core/managers/event_manager.h"  // 1. 对应头文件

#include <windows.h>                     // 2. 系统头文件

#include "core/interfaces/ievent_bus.h"  // 3. 项目头文件（接口）
#include "core/interfaces/iscene_provider.h"
#include "core/interfaces/iconfig_provider.h"
#include "core/managers/scene_manager.h"
#include "core/utils/logger.h"
#include "core/utils/event_bus.h"
#include "window/window.h"

EventManager::EventManager() {
}

EventManager::~EventManager() {
}

void EventManager::Initialize(IInputHandler* inputHandler, 
                              IRenderer* renderer,
                              IWindow* window,
                              ISceneProvider* sceneProvider,
                              IEventBus* eventBus) {
    m_inputHandler = inputHandler;
    m_renderer = renderer;
    m_window = window;
    m_sceneProvider = sceneProvider;
    m_eventBus = eventBus;
}

void EventManager::PublishUIClickEvent(int windowX, int windowY, StretchMode stretchMode) {
    if (!m_inputHandler || !m_eventBus) {
        return;
    }
    
    // 转换坐标
    float uiX, uiY;
    m_inputHandler->ConvertWindowToUICoords(windowX, windowY, uiX, uiY);
    
    if (uiX < 0.0f || uiY < 0.0f) {
        return;
    }
    
    // 发布UI点击事件（由订阅者处理，如UIManager）
    UIClickEvent event(uiX, uiY, stretchMode);
    m_eventBus->Publish(event);
}

void EventManager::PublishMouseMoveUIEvent(int windowX, int windowY) {
    if (!m_inputHandler || !m_eventBus) {
        return;
    }
    
    // 转换坐标
    float uiX, uiY;
    m_inputHandler->ConvertWindowToUICoords(windowX, windowY, uiX, uiY);
    
    // 发布UI鼠标移动事件（由订阅者处理，如UIManager）
    MouseMovedUIEvent event(uiX, uiY);
    m_eventBus->Publish(event);
}

void EventManager::PublishMouseUpEvent() {
    if (!m_eventBus) {
        return;
    }
    
    // 发布鼠标释放事件（由订阅者处理，如UIManager）
    MouseUpEvent event;
    m_eventBus->Publish(event);
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
            // 发布窗口大小变化事件（由订阅者处理，如UIManager）
            if (m_eventBus) {
                WindowResizeRequestEvent event(stretchMode, m_renderer);
                m_eventBus->Publish(event);
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
            PublishUIClickEvent(pt.x, pt.y, stretchMode);
            return true;
        }
        case WM_MOUSEMOVE: {
            POINT pt;
            pt.x = LOWORD(msg.lParam);
            pt.y = HIWORD(msg.lParam);
            PublishMouseMoveUIEvent(pt.x, pt.y);
            return true;
        }
        case WM_LBUTTONUP: {
            PublishMouseUpEvent();
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


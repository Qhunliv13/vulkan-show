#pragma once

#include <functional>
#include <memory>
#include <windows.h>
#include "core/constants.h"
#include "core/irenderer.h"
#include "core/iwindow_resize_handler.h"

// 前向声明
class InputHandler;
class UIManager;
class SceneManager;
class Window;

// 事件管理器 - 统一处理所有窗口消息和输入事件（完全接管事件处理）
class EventManager {
public:
    EventManager();
    ~EventManager();
    
    // 初始化（使用接口而不是具体类）
    void Initialize(InputHandler* inputHandler, 
                   UIManager* uiManager, 
                   IRenderer* renderer,
                   Window* window,
                   ISceneProvider* sceneProvider);
    
    // 统一的消息处理接口 - 处理所有Windows消息
    bool ProcessMessage(const MSG& msg, StretchMode stretchMode);
    
    // 处理消息队列中的所有消息
    bool ProcessMessages(StretchMode stretchMode);
    
    // 处理鼠标点击（保留向后兼容）
    void HandleMouseClick(int x, int y, StretchMode stretchMode);
    
    // 处理鼠标移动（保留向后兼容）
    void HandleMouseMove(int x, int y);
    
    // 处理鼠标释放（保留向后兼容）
    void HandleMouseUp();
    
    // 处理窗口大小变化（使用接口）
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer);
    
    // 设置场景状态变化回调（已废弃，使用事件总线）
    void SetOnStateChangeCallback(std::function<void()> callback) { m_onStateChangeCallback = callback; }

private:
    // 处理单个消息类型
    bool HandleWindowMessage(const MSG& msg, StretchMode stretchMode);
    bool HandleMouseMessage(const MSG& msg, StretchMode stretchMode);
    bool HandleKeyboardMessage(const MSG& msg);
    
    InputHandler* m_inputHandler = nullptr;
    UIManager* m_uiManager = nullptr;  // UIManager 实现了 IWindowResizeHandler
    IRenderer* m_renderer = nullptr;
    Window* m_window = nullptr;
    ISceneProvider* m_sceneProvider = nullptr;  // 使用接口而不是具体类
    std::function<void()> m_onStateChangeCallback;
};


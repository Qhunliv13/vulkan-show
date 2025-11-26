#pragma once

#include <functional>
#include <memory>
#include <windows.h>
#include "core/config/constants.h"
#include "core/interfaces/irenderer.h"
#include "core/interfaces/iuimanager.h"
#include "core/interfaces/iinput_handler.h"

// 前向声明
class SceneManager;
class Window;
class ISceneProvider;
class IEventBus;
class IRenderer;
class IConfigProvider;

// 事件管理器 - 统一处理所有窗口消息和输入事件（完全接管事件处理）
// 通过接口和事件总线解耦，避免直接依赖具体类
class EventManager {
public:
    EventManager();
    ~EventManager();
    
    // 初始化（使用接口而不是具体类）
    void Initialize(IInputHandler* inputHandler, 
                   IUIManager* uiManager, 
                   IRenderer* renderer,
                   Window* window,
                   ISceneProvider* sceneProvider,
                   IEventBus* eventBus = nullptr);
    
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

private:
    // 处理单个消息类型
    bool HandleWindowMessage(const MSG& msg, StretchMode stretchMode);
    bool HandleMouseMessage(const MSG& msg, StretchMode stretchMode);
    bool HandleKeyboardMessage(const MSG& msg);
    
    IInputHandler* m_inputHandler = nullptr;  // 使用接口而不是具体类
    IUIManager* m_uiManager = nullptr;  // 使用接口而不是具体类
    IRenderer* m_renderer = nullptr;
    Window* m_window = nullptr;
    ISceneProvider* m_sceneProvider = nullptr;  // 使用接口而不是具体类
    IEventBus* m_eventBus = nullptr;  // 事件总线（用于解耦）
};


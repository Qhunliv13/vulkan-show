#pragma once

#include <functional>
#include <memory>
#include <windows.h>
#include "core/config/constants.h"
#include "core/interfaces/irenderer.h"
#include "core/interfaces/iinput_handler.h"
#include "core/interfaces/iwindow.h"

// 前向声明
class SceneManager;
class ISceneProvider;
class IEventBus;
class IRenderer;
class IConfigProvider;

// 事件管理器 - 统一处理所有窗口消息和输入事件
// 职责：只负责将Windows消息转换为事件并发布到事件总线，不直接调用其他组件
// 通过事件总线实现完全解耦
class EventManager {
public:
    EventManager();
    ~EventManager();
    
    // 初始化（只依赖事件总线和必要的接口）
    void Initialize(IInputHandler* inputHandler,  // 仅用于坐标转换
                     IRenderer* renderer,
                     IWindow* window,
                     ISceneProvider* sceneProvider,
                     IEventBus* eventBus);
    
    // 统一的消息处理接口 - 处理所有Windows消息
    bool ProcessMessage(const MSG& msg, StretchMode stretchMode);
    
    // 处理消息队列中的所有消息
    bool ProcessMessages(StretchMode stretchMode);

private:
    // 处理单个消息类型
    bool HandleWindowMessage(const MSG& msg, StretchMode stretchMode);
    bool HandleMouseMessage(const MSG& msg, StretchMode stretchMode);
    bool HandleKeyboardMessage(const MSG& msg);
    
    // 发布UI相关事件（转换坐标后发布）
    void PublishUIClickEvent(int windowX, int windowY, StretchMode stretchMode);
    void PublishMouseMoveUIEvent(int windowX, int windowY);
    void PublishMouseUpEvent();
    
    IInputHandler* m_inputHandler = nullptr;  // 仅用于坐标转换
    IRenderer* m_renderer = nullptr;
    IWindow* m_window = nullptr;
    ISceneProvider* m_sceneProvider = nullptr;  // 使用接口检查是否应该处理输入
    IEventBus* m_eventBus = nullptr;  // 事件总线（唯一通信方式）
};


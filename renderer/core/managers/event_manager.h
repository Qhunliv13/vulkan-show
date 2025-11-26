#pragma once

#include <windows.h>  // 2. 系统头文件
#include <functional>  // 2. 系统头文件
#include <memory>  // 2. 系统头文件
#include "core/config/constants.h"  // 4. 项目头文件（配置）
#include "core/interfaces/irenderer.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iinput_handler.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iwindow.h"  // 4. 项目头文件（接口）

// 前向声明
class SceneManager;
class ISceneProvider;
class IEventBus;
class IRenderer;
class IConfigProvider;

/**
 * 事件管理器 - 统一处理所有窗口消息和输入事件
 * 
 * 职责：将 Windows 消息转换为事件并发布到事件总线，不直接调用其他组件
 * 设计：通过事件总线实现完全解耦，所有组件间通信通过事件总线
 * 
 * 使用方式：
 * 1. 通过依赖注入传入所有依赖（IInputHandler、IRenderer、IWindow、ISceneProvider、IEventBus）
 * 2. 调用 ProcessMessages() 处理消息队列
 * 3. 所有事件通过 IEventBus 发布，由订阅者处理
 */
class EventManager {
public:
    EventManager();
    ~EventManager();
    
    /**
     * 初始化事件管理器
     * 
     * 通过依赖注入传入所有依赖，确保组件间解耦
     * 
     * @param inputHandler 输入处理器（不拥有所有权，仅用于坐标转换）
     * @param renderer 渲染器（不拥有所有权，用于窗口大小变化事件）
     * @param window 窗口（不拥有所有权，用于消息处理）
     * @param sceneProvider 场景提供者（不拥有所有权，用于检查是否应该处理输入）
     * @param eventBus 事件总线（不拥有所有权，用于发布事件）
     */
    void Initialize(IInputHandler* inputHandler,
                     IRenderer* renderer,
                     IWindow* window,
                     ISceneProvider* sceneProvider,
                     IEventBus* eventBus);
    
    /**
     * 处理单个 Windows 消息
     * 
     * 将 Windows 消息转换为事件并发布到事件总线
     * 
     * @param msg Windows 消息
     * @param stretchMode 拉伸模式，用于坐标转换
     * @return true 如果继续处理，false 如果收到退出消息
     */
    bool ProcessMessage(const MSG& msg, StretchMode stretchMode);
    
    /**
     * 处理消息队列中的所有消息
     * 
     * 循环处理所有待处理的消息，直到消息队列为空或收到退出消息
     * 
     * @param stretchMode 拉伸模式，用于坐标转换
     * @return true 如果继续运行，false 如果收到退出消息
     */
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
    
    IInputHandler* m_inputHandler = nullptr;  // 输入处理器（不拥有所有权，仅用于坐标转换）
    IRenderer* m_renderer = nullptr;  // 渲染器（不拥有所有权，用于窗口大小变化事件）
    IWindow* m_window = nullptr;  // 窗口（不拥有所有权，用于消息处理）
    ISceneProvider* m_sceneProvider = nullptr;  // 场景提供者（不拥有所有权，用于检查是否应该处理输入）
    IEventBus* m_eventBus = nullptr;  // 事件总线（不拥有所有权，用于发布事件，唯一通信方式）
};


#pragma once

#include <windows.h>
#include <memory>
#include "initialization_result.h"

// 前向声明
class IRendererFactory;
class WindowManager;
class IRenderer;
class ITextRenderer;
class IInputHandler;
class IUIManager;
class EventManager;
class ISceneProvider;
class RenderScheduler;
class WindowMessageHandler;
class Window;
class IConfigProvider;
class ILogger;
class IEventBus;
class IWindowFactory;
class ITextRendererFactory;

// 应用初始化器 - 管理初始化顺序和依赖关系
class AppInitializer {
public:
    AppInitializer();
    ~AppInitializer();
    
    // 初始化所有组件（按正确顺序）
    // 使用依赖注入：所有组件必须通过参数传入，不再使用单例
    bool Initialize(IRendererFactory* rendererFactory, HINSTANCE hInstance, const char* lpCmdLine,
                   IConfigProvider* configProvider, ILogger* logger, IEventBus* eventBus,
                   IWindowFactory* windowFactory, ITextRendererFactory* textRendererFactory);
    
    // 获取组件指针（优先返回接口，减少对具体类的依赖）
    WindowManager* GetWindowManager() const { return m_windowManager.get(); }
    IRenderer* GetRenderer() const { return m_renderer; }
    ITextRenderer* GetTextRenderer() const { return m_textRenderer; }
    IInputHandler* GetInputHandler() const { return m_inputHandler; }
    IUIManager* GetUIManager() const;
    EventManager* GetEventManager() const { return m_eventManager.get(); }
    ISceneProvider* GetSceneProvider() const;
    RenderScheduler* GetRenderScheduler() const { return m_renderScheduler.get(); }
    WindowMessageHandler* GetMessageHandler() const { return m_messageHandler.get(); }
    IConfigProvider* GetConfigProvider() const { return m_configProvider; }
    ILogger* GetLogger() const { return m_logger; }
    IEventBus* GetEventBus() const { return m_eventBus; }
    
    // 清理所有资源
    void Cleanup();
    
    // 部分清理（用于初始化失败时的回滚）
    void CleanupPartial(int initializedSteps);

private:
    // 初始化步骤（按依赖顺序）
    bool InitializeConsole();
    InitializationResult InitializeLogger();
    InitializationResult InitializeWindow(HINSTANCE hInstance);
    InitializationResult InitializeRenderer(IRendererFactory* rendererFactory, HINSTANCE hInstance);
    InitializationResult InitializeInputHandler();
    bool InitializeManagers();
    InitializationResult InitializeUI();
    InitializationResult InitializeEventSystem();
    InitializationResult InitializeRenderScheduler();
    
    // 组件（使用接口类型减少依赖）
    std::unique_ptr<WindowManager> m_windowManager;
    IRenderer* m_renderer = nullptr;
    IRendererFactory* m_rendererFactory = nullptr;
    ITextRenderer* m_textRenderer = nullptr;
    class InputHandler* m_inputHandlerImpl = nullptr;  // 保存具体实现指针，用于类型转换
    IInputHandler* m_inputHandler = nullptr;  // 使用接口类型（用于 EventManager）
    std::unique_ptr<class UIManager> m_uiManager;  // 前向声明，减少头文件依赖
    std::unique_ptr<EventManager> m_eventManager;
    std::unique_ptr<class SceneManager> m_sceneManager;  // 前向声明，减少头文件依赖
    std::unique_ptr<RenderScheduler> m_renderScheduler;
    std::unique_ptr<WindowMessageHandler> m_messageHandler;
    IConfigProvider* m_configProvider = nullptr;  // 配置提供者（依赖注入）
    ILogger* m_logger = nullptr;  // 日志提供者（依赖注入）
    IEventBus* m_eventBus = nullptr;  // 事件总线（依赖注入）
    IWindowFactory* m_windowFactory = nullptr;  // 窗口工厂（依赖注入）
    ITextRendererFactory* m_textRendererFactory = nullptr;  // 文字渲染器工厂（依赖注入）
    
    // 控制台文件指针
    FILE* m_pCout = nullptr;
    FILE* m_pCin = nullptr;
    FILE* m_pCerr = nullptr;
    
    bool m_initialized = false;
};


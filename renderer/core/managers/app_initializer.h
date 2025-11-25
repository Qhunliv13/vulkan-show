#pragma once

#include <windows.h>
#include <memory>

// 前向声明
class IRendererFactory;
class WindowManager;
class IRenderer;
class TextRenderer;
class InputHandler;
class UIManager;
class EventManager;
class SceneManager;
class RenderScheduler;
class WindowMessageHandler;
class Window;
class IConfigProvider;
class ISceneProvider;

// 应用初始化器 - 管理初始化顺序和依赖关系
class AppInitializer {
public:
    AppInitializer();
    ~AppInitializer();
    
    // 初始化所有组件（按正确顺序）
    bool Initialize(IRendererFactory* rendererFactory, HINSTANCE hInstance, const char* lpCmdLine);
    
    // 获取组件指针
    WindowManager* GetWindowManager() const { return m_windowManager.get(); }
    IRenderer* GetRenderer() const { return m_renderer; }
    TextRenderer* GetTextRenderer() const { return m_textRenderer; }
    InputHandler* GetInputHandler() const { return m_inputHandler; }
    UIManager* GetUIManager() const { return m_uiManager.get(); }
    EventManager* GetEventManager() const { return m_eventManager.get(); }
    SceneManager* GetSceneManager() const { return m_sceneManager.get(); }
    RenderScheduler* GetRenderScheduler() const { return m_renderScheduler.get(); }
    WindowMessageHandler* GetMessageHandler() const { return m_messageHandler.get(); }
    IConfigProvider* GetConfigProvider() const { return m_configProvider; }
    ISceneProvider* GetSceneProvider() const;
    
    // 清理所有资源
    void Cleanup();
    
    // 部分清理（用于初始化失败时的回滚）
    void CleanupPartial(int initializedSteps);

private:
    // 初始化步骤（按依赖顺序）
    bool InitializeConfig(const char* lpCmdLine);
    bool InitializeConsole();
    bool InitializeLogger();
    bool InitializeWindow(HINSTANCE hInstance);
    bool InitializeRenderer(IRendererFactory* rendererFactory, HINSTANCE hInstance);
    bool InitializeInputHandler();
    bool InitializeManagers();
    bool InitializeUI();
    bool InitializeEventSystem();
    bool InitializeRenderScheduler();
    
    // 组件
    std::unique_ptr<WindowManager> m_windowManager;
    IRenderer* m_renderer = nullptr;
    IRendererFactory* m_rendererFactory = nullptr;
    TextRenderer* m_textRenderer = nullptr;
    InputHandler* m_inputHandler = nullptr;
    std::unique_ptr<UIManager> m_uiManager;
    std::unique_ptr<EventManager> m_eventManager;
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<RenderScheduler> m_renderScheduler;
    std::unique_ptr<WindowMessageHandler> m_messageHandler;
    IConfigProvider* m_configProvider = nullptr;  // 配置提供者（依赖注入）
    
    // 控制台文件指针
    FILE* m_pCout = nullptr;
    FILE* m_pCin = nullptr;
    FILE* m_pCerr = nullptr;
    
    bool m_initialized = false;
};


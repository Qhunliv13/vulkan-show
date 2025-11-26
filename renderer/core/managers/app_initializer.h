#pragma once

#include <windows.h>  // 2. 系统头文件
#include <memory>  // 2. 系统头文件
#include "initialization_result.h"  // 4. 项目头文件（初始化结果）
#include "app_initialization_config.h"  // 4. 项目头文件（初始化配置）
#include "initialization_phase.h"  // 4. 项目头文件（初始化阶段）

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

/**
 * 应用初始化器 - 管理初始化顺序和依赖关系
 * 
 * 职责：管理所有组件的初始化顺序和依赖关系，确保初始化正确完成
 * 设计：通过 AppInitializationConfig 配置对象传递所有依赖，实现依赖注入
 * 
 * 初始化顺序：Config → Console → Logger → Window → Renderer → Input → Managers → UI → Event → Scheduler
 * 
 * 初始化流程图：
 * ```
 * 1. InitializeConfig()        [无依赖]
 *    ↓
 * 2. InitializeConsole()       [依赖: Config]
 *    ↓
 * 3. InitializeLogger()        [依赖: Config]
 *    ↓
 * 4. InitializeWindow()        [依赖: Logger]
 *    ↓
 * 5. InitializeRenderer()      [依赖: Window, Logger]
 *    ↓
 * 6. InitializeInputHandler()  [依赖: Logger, EventBus]
 *    ↓
 * 7. InitializeManagers()      [依赖: Renderer, Window]
 *    ↓
 * 8. InitializeUI()             [依赖: Renderer, Window, Logger]
 *    ↓
 * 9. InitializeEventSystem()   [依赖: InputHandler, UIManager, Renderer]
 *    ↓
 * 10. InitializeRenderScheduler() [依赖: Renderer, EventManager]
 * ```
 * 
 * 错误处理：失败时按逆序回滚已初始化的步骤，确保资源正确释放
 * 
 * 使用方式：
 * 1. 创建 AppInitializationConfig 配置对象，设置所有依赖
 * 2. 调用 Initialize() 初始化所有组件
 * 3. 使用 Get 方法获取组件指针
 * 4. 调用 Cleanup() 清理所有资源
 */
class AppInitializer {
public:
    AppInitializer();
    ~AppInitializer();
    
    // 初始化所有组件（按正确顺序）
    // 使用配置对象封装所有初始化参数，简化接口
    bool Initialize(const AppInitializationConfig& config);
    
    // 获取组件指针（优先返回接口，减少对具体类的依赖）
    WindowManager* GetWindowManager() const { return m_windowManager.get(); }
    IRenderer* GetRenderer() const { return m_renderer.get(); }
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
    // 注意：已废弃，使用阶段管理器自动处理回滚
    void CleanupPartial(int initializedSteps);
    
    // 使用阶段管理器简化初始化（新方法）
    bool InitializeWithPhases(const AppInitializationConfig& config);

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
    std::unique_ptr<IRenderer> m_renderer;  // 使用 unique_ptr 管理渲染器生命周期
    IRendererFactory* m_rendererFactory = nullptr;
    ITextRenderer* m_textRenderer = nullptr;
    class InputHandler* m_inputHandlerImpl = nullptr;  // 保存具体实现指针，用于类型转换
    IInputHandler* m_inputHandler = nullptr;  // 使用接口类型（用于 EventManager）
    std::unique_ptr<class UIManager> m_uiManager;  // 前向声明，减少头文件依赖
    std::unique_ptr<class UIRenderProviderAdapter> m_uiRenderProviderAdapter;  // UI渲染提供者适配器
    std::unique_ptr<class UIWindowResizeAdapter> m_uiWindowResizeAdapter;  // UI窗口大小变化适配器
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
    
    // 事件订阅ID（用于在 Cleanup() 时取消订阅）
    size_t m_mouseMovedSubscriptionId = 0;
    size_t m_keyPressedSubscriptionId = 0;
    size_t m_buttonClickedSubscriptionId = 0;
    
    bool m_initialized = false;
};


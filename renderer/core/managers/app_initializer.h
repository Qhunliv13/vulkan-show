#pragma once

#include <memory>  // 2. 系统头文件
#include <windows.h>  // 2. 系统头文件
#include "app_initialization_config.h"  // 4. 项目头文件（初始化配置）
#include "initialization_phase.h"  // 4. 项目头文件（初始化阶段）
#include "initialization_result.h"  // 4. 项目头文件（初始化结果）

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
 * 依赖注入说明：
 * Initialize() 方法依赖 8 个参数（通过 AppInitializationConfig 封装）：
 * - IRendererFactory* rendererFactory: 渲染器工厂（用于创建渲染器）
 * - IConfigProvider* configProvider: 配置提供者（用于获取配置参数）
 * - ILogger* logger: 日志器（用于日志输出）
 * - IEventBus* eventBus: 事件总线（用于事件通信）
 * - IWindowFactory* windowFactory: 窗口工厂（用于创建窗口）
 * - ITextRendererFactory* textRendererFactory: 文字渲染器工厂（用于创建文字渲染器）
 * - HINSTANCE hInstance: Windows 实例句柄
 * - const char* lpCmdLine: 命令行参数
 * 
 * 依赖关系图（Mermaid）：
 * ```mermaid
 * graph TD
 *     subgraph "依赖注入层"
 *         RendererFactory[IRendererFactory]
 *         ConfigProvider[IConfigProvider]
 *         Logger[ILogger]
 *         EventBus[IEventBus]
 *         WindowFactory[IWindowFactory]
 *         TextRendererFactory[ITextRendererFactory]
 *     end
 *     
 *     subgraph "核心组件"
 *         AppInit[AppInitializer]
 *         WindowMgr[WindowManager]
 *         Renderer[IRenderer]
 *         EventMgr[EventManager]
 *         UIMgr[UIManager]
 *         SceneMgr[SceneManager]
 *         RenderScheduler[RenderScheduler]
 *     end
 *     
 *     AppInit -->|依赖注入| RendererFactory
 *     AppInit -->|依赖注入| ConfigProvider
 *     AppInit -->|依赖注入| Logger
 *     AppInit -->|依赖注入| EventBus
 *     AppInit -->|依赖注入| WindowFactory
 *     AppInit -->|依赖注入| TextRendererFactory
 *     
 *     AppInit -->|创建| WindowMgr
 *     AppInit -->|创建| Renderer
 *     AppInit -->|创建| EventMgr
 *     AppInit -->|创建| UIMgr
 *     AppInit -->|创建| SceneMgr
 *     AppInit -->|创建| RenderScheduler
 *     
 *     Renderer -->|依赖| WindowMgr
 *     EventMgr -->|依赖| Renderer
 *     EventMgr -->|依赖| EventBus
 *     UIMgr -->|依赖| Renderer
 *     UIMgr -->|依赖| Logger
 *     RenderScheduler -->|依赖| Renderer
 *     RenderScheduler -->|依赖| EventMgr
 * ```
 * 
 * 初始化顺序：Config → Console → Logger → Window → Renderer → Input → Managers → UI → Event → Scheduler
 * 
 * 初始化流程图（Mermaid）：
 * ```mermaid
 * flowchart TD
 *     Start([开始初始化]) --> Step1[步骤1: 初始化配置]
 *     Step1 -->|成功| Step2[步骤2: 初始化控制台]
 *     Step1 -->|失败| Error1([初始化失败])
 *     
 *     Step2 -->|成功| Step3[步骤3: 初始化日志]
 *     Step2 -->|失败| Rollback1[回滚步骤1]
 *     Rollback1 --> Error1
 *     
 *     Step3 -->|成功| Step4[步骤4: 初始化窗口]
 *     Step3 -->|失败| Rollback2[回滚步骤2-1]
 *     Rollback2 --> Error1
 *     
 *     Step4 -->|成功| Step5[步骤5: 初始化渲染器]
 *     Step4 -->|失败| Rollback3[回滚步骤3-1]
 *     Rollback3 --> Error1
 *     
 *     Step5 -->|成功| Step6[步骤6: 初始化输入处理器]
 *     Step5 -->|失败| Rollback4[回滚步骤4-1]
 *     Rollback4 --> Error1
 *     
 *     Step6 -->|成功| Step7[步骤7: 初始化管理器]
 *     Step6 -->|失败| Rollback5[回滚步骤5-1]
 *     Rollback5 --> Error1
 *     
 *     Step7 -->|成功| Step8[步骤8: 初始化UI]
 *     Step7 -->|失败| Rollback6[回滚步骤6-1]
 *     Rollback6 --> Error1
 *     
 *     Step8 -->|成功| Step9[步骤9: 初始化事件系统]
 *     Step8 -->|失败| Rollback7[回滚步骤7-1]
 *     Rollback7 --> Error1
 *     
 *     Step9 -->|成功| Step10[步骤10: 初始化渲染调度器]
 *     Step9 -->|失败| Rollback8[回滚步骤8-1]
 *     Rollback8 --> Error1
 *     
 *     Step10 -->|成功| End([初始化完成])
 *     Step10 -->|失败| Rollback9[回滚步骤9-1]
 *     Rollback9 --> Error1
 *     
 *     style Start fill:#90EE90
 *     style End fill:#90EE90
 *     style Error1 fill:#FFB6C1
 *     style Rollback1 fill:#FFA500
 *     style Rollback2 fill:#FFA500
 *     style Rollback3 fill:#FFA500
 *     style Rollback4 fill:#FFA500
 *     style Rollback5 fill:#FFA500
 *     style Rollback6 fill:#FFA500
 *     style Rollback7 fill:#FFA500
 *     style Rollback8 fill:#FFA500
 *     style Rollback9 fill:#FFA500
 * ```
 * 
 * 错误处理说明：
 * - 每个初始化步骤失败时，会按逆序回滚已初始化的步骤
 * - 回滚顺序：Scheduler → Event → UI → Managers → Input → Renderer → Window → Logger → Console → Config
 * - 所有事件订阅在回滚时会被取消，避免悬空指针
 * - 使用 unique_ptr 管理的资源会自动清理
 * - 日志系统失败时允许继续（使用控制台输出），其他步骤失败则回滚
 * 
 * 调用示例：
 * ```cpp
 * // 1. 创建所有依赖对象
 * auto configManager = std::make_unique<ConfigManager>();
 * auto logger = std::make_unique<Logger>();
 * auto eventBus = std::make_unique<EventBus>();
 * auto windowFactory = std::make_unique<WindowFactory>();
 * auto textRendererFactory = std::make_unique<TextRendererFactory>();
 * 
 * // 2. 创建初始化配置
 * AppInitializationConfig config;
 * config.rendererFactory = &rendererFactory;  // 外部传入
 * config.hInstance = hInstance;
 * config.lpCmdLine = lpCmdLine;
 * config.configProvider = configManager.get();
 * config.logger = logger.get();
 * config.eventBus = eventBus.get();
 * config.windowFactory = windowFactory.get();
 * config.textRendererFactory = textRendererFactory.get();
 * 
 * // 3. 创建并初始化
 * auto initializer = std::make_unique<AppInitializer>();
 * if (!initializer->Initialize(config)) {
 *     // 初始化失败，所有资源已自动回滚
 *     return false;
 * }
 * 
 * // 4. 使用组件
 * auto* renderer = initializer->GetRenderer();
 * auto* windowManager = initializer->GetWindowManager();
 * 
 * // 5. 清理（析构函数自动调用）
 * initializer->Cleanup();
 * ```
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
    ITextRenderer* GetTextRenderer() const { return m_textRenderer.get(); }
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
    std::unique_ptr<ITextRenderer> m_textRenderer;  // 使用 unique_ptr 管理文字渲染器生命周期
    std::unique_ptr<class InputHandler> m_inputHandlerImpl;  // 使用 unique_ptr 管理 InputHandler 生命周期
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


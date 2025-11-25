#include "core/managers/app_initializer.h"
#include "core/managers/config_manager.h"
#include "core/managers/initialization_result.h"
#include "core/interfaces/iconfig_provider.h"
#include "core/interfaces/iscene_provider.h"
#include "core/managers/event_manager.h"
#include "core/managers/window_manager.h"
#include "core/interfaces/irenderer_factory.h"
#include "core/interfaces/irenderer.h"
#include "core/utils/logger.h"
#include "core/utils/event_bus.h"
#include "core/interfaces/ilogger.h"
#include "core/interfaces/ievent_bus.h"
#include "core/ui/ui_manager.h"
#include "core/managers/event_manager.h"
#include "core/managers/scene_manager.h"
#include "core/managers/render_scheduler.h"
#include "core/handlers/window_message_handler.h"
#include "core/utils/input_handler.h"
#include "text/text_renderer.h"
#include "core/interfaces/itext_renderer.h"
#include "window/window.h"
#include <windows.h>
#include <io.h>
#include <fcntl.h>

AppInitializer::AppInitializer() {
}

AppInitializer::~AppInitializer() {
    Cleanup();
}

bool AppInitializer::Initialize(IRendererFactory* rendererFactory, HINSTANCE hInstance, const char* lpCmdLine,
                               ILogger* logger, IEventBus* eventBus) {
    if (m_initialized) {
        return true;
    }
    
    // 设置依赖注入的组件（如果提供）
    m_logger = logger;
    m_eventBus = eventBus;
    
    // 初始化步骤计数器（用于回滚）
    int initializedSteps = 0;
    
    // 1. 初始化配置（最先，其他组件依赖配置）
    InitializationResult configResult = InitializeConfig(lpCmdLine);
    if (!configResult.success) {
        if (m_logger) {
            m_logger->Error(configResult.errorMessage.empty() ? 
                           "Failed to initialize config" : configResult.errorMessage);
        }
        return false;
    }
    initializedSteps = 1;
    
    // 2. 初始化控制台（日志系统需要）
    InitializeConsole();
    initializedSteps = 2;
    
    // 3. 初始化日志系统
    InitializationResult loggerResult = InitializeLogger();
    if (!loggerResult.success) {
        // 日志系统失败时继续，但记录警告
        printf("[WARNING] Failed to initialize logger: %s, continuing without file logging\n", 
               loggerResult.errorMessage.c_str());
    }
    initializedSteps = 3;
    
    if (m_logger) {
        m_logger->Info("Application initializing...");
    }
    
    // 4. 初始化窗口（渲染器依赖窗口）
    InitializationResult windowResult = InitializeWindow(hInstance);
    if (!windowResult.success) {
        if (m_logger) {
            m_logger->Error(windowResult.errorMessage.empty() ? 
                           "Failed to initialize window" : windowResult.errorMessage);
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 4;
    
    // 5. 初始化渲染器（UI和场景依赖渲染器）
    InitializationResult rendererResult = InitializeRenderer(rendererFactory, hInstance);
    if (!rendererResult.success) {
        if (m_logger) {
            m_logger->Error(rendererResult.errorMessage.empty() ? 
                           "Failed to initialize renderer" : rendererResult.errorMessage);
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 5;
    
    // 6. 初始化输入处理器（事件管理器依赖）
    InitializationResult inputResult = InitializeInputHandler();
    if (!inputResult.success) {
        if (m_logger) {
            m_logger->Error(inputResult.errorMessage.empty() ? 
                           "Failed to initialize input handler" : inputResult.errorMessage);
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 6;
    
    // 7. 初始化管理器（基础组件）
    if (!InitializeManagers()) {
        if (m_logger) {
            m_logger->Error("Failed to initialize managers");
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 7;
    
    // 8. 初始化UI（依赖渲染器和窗口）
    InitializationResult uiResult = InitializeUI();
    if (!uiResult.success) {
        if (m_logger) {
            m_logger->Error(uiResult.errorMessage.empty() ? 
                           "Failed to initialize UI" : uiResult.errorMessage);
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 8;
    
    // 9. 初始化事件系统（依赖UI、场景、输入处理器）
    InitializationResult eventResult = InitializeEventSystem();
    if (!eventResult.success) {
        if (m_logger) {
            m_logger->Error(eventResult.errorMessage.empty() ? 
                           "Failed to initialize event system" : eventResult.errorMessage);
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 9;
    
    // 10. 初始化渲染调度器（依赖所有其他组件）
    InitializationResult schedulerResult = InitializeRenderScheduler();
    if (!schedulerResult.success) {
        if (m_logger) {
            m_logger->Error(schedulerResult.errorMessage.empty() ? 
                           "Failed to initialize render scheduler" : schedulerResult.errorMessage);
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    
    m_initialized = true;
    if (m_logger) {
        m_logger->Info("Application initialized successfully");
    }
    return true;
}

InitializationResult AppInitializer::InitializeConfig(const char* lpCmdLine) {
    // 获取配置管理器实例（通过单例获取，但通过接口传递以保持依赖注入）
    // 注意：ConfigManager 仍然使用单例模式，但通过 IConfigProvider 接口传递，
    // 这样其他组件只依赖接口，不直接依赖 ConfigManager
    m_configProvider = &ConfigManager::GetInstance();
    if (!m_configProvider) {
        return InitializationResult::Failure("Failed to get ConfigManager instance");
    }
    m_configProvider->Initialize(lpCmdLine);
    return InitializationResult::Success();
}

bool AppInitializer::InitializeConsole() {
    AllocConsole();
    freopen_s(&m_pCout, "CONOUT$", "w", stdout);
    freopen_s(&m_pCin, "CONIN$", "r", stdin);
    freopen_s(&m_pCerr, "CONOUT$", "w", stderr);
    SetConsoleTitleA("Shader App Debug Console");
    return true;
}

InitializationResult AppInitializer::InitializeLogger() {
    if (!m_configProvider) {
        return InitializationResult::Failure("ConfigProvider not initialized");
    }
    
    // 使用依赖注入：如果未注入，则使用单例（向后兼容）
    if (!m_logger) {
        m_logger = &Logger::GetInstance();
    }
    
    std::string logPath = m_configProvider->GetLogPath();
    
    if (!m_logger->Initialize(logPath)) {
        // 尝试不指定文件，只使用控制台输出
        if (!m_logger->Initialize("")) {
            return InitializationResult::Failure("Failed to initialize logger even with console output");
        }
        // 警告：日志文件初始化失败，但控制台输出可用
        return InitializationResult::Success();
    }
    return InitializationResult::Success();
}

InitializationResult AppInitializer::InitializeWindow(HINSTANCE hInstance) {
    if (!m_configProvider) {
        return InitializationResult::Failure("ConfigProvider not initialized");
    }
    
    m_windowManager = std::make_unique<WindowManager>();
    // 使用依赖注入传递配置提供者
    if (!m_windowManager->Initialize(hInstance, m_configProvider)) {
        return InitializationResult::Failure("Failed to initialize WindowManager");
    }
    return InitializationResult::Success();
}

InitializationResult AppInitializer::InitializeRenderer(IRendererFactory* rendererFactory, HINSTANCE hInstance) {
    if (!rendererFactory || !m_windowManager || !m_windowManager->GetWindow()) {
        return InitializationResult::Failure("Invalid parameters for renderer initialization");
    }
    
    m_rendererFactory = rendererFactory;
    
    // 使用工厂创建渲染器
    m_renderer = rendererFactory->CreateRenderer();
    if (!m_renderer) {
        return InitializationResult::Failure("Failed to create renderer from factory");
    }
    
    if (!m_renderer->Initialize(m_windowManager->GetWindow()->GetHandle(), hInstance)) {
        if (rendererFactory) {
            rendererFactory->DestroyRenderer(m_renderer);
            m_renderer = nullptr;
        }
        return InitializationResult::Failure("Failed to initialize renderer");
    }
    
    // 从配置提供者获取配置（依赖注入）
    if (!m_configProvider) {
        if (rendererFactory) {
            rendererFactory->DestroyRenderer(m_renderer);
            m_renderer = nullptr;
        }
        return InitializationResult::Failure("ConfigProvider not initialized");
    }
    m_renderer->SetAspectRatioMode(m_configProvider->GetAspectRatioMode());
    m_renderer->SetStretchMode(m_configProvider->GetStretchMode());
    m_renderer->SetBackgroundStretchMode(m_configProvider->GetBackgroundStretchMode());
    
    // 设置鼠标移动回调
    auto* window = m_windowManager->GetWindow();
    if (window) {
        window->SetMouseMoveCallback([this](float deltaX, float deltaY, bool leftButtonDown) {
            if (m_renderer) {
                m_renderer->SetMouseInput(deltaX, deltaY, leftButtonDown);
            }
        });
        
        // 设置键盘回调
        window->SetKeyCallback([](int keyCode, bool isPressed) {
            // 可以在这里处理按键事件，但相机位置更新在游戏循环中进行
        });
    }
    
    // 加载背景纹理（非关键，失败时继续）
    if (!m_renderer->LoadBackgroundTexture(m_configProvider->GetBackgroundTexturePath().c_str())) {
        if (m_logger) {
            m_logger->Warning("Failed to load background texture, continuing without background");
        }
    }
    
    // 尝试创建ray tracing pipeline（非关键，失败时继续）
    if (m_renderer->IsRayTracingSupported()) {
        if (m_logger) {
            m_logger->Info("Hardware ray tracing is supported, attempting to create pipeline...");
        }
        if (m_renderer->CreateRayTracingPipeline()) {
            if (m_logger) {
                m_logger->Info("Hardware ray tracing pipeline created successfully!");
            }
        } else {
            if (m_logger) {
                m_logger->Info("Hardware ray tracing pipeline creation failed, will use software ray casting");
            }
        }
    } else {
        if (m_logger) {
            m_logger->Info("Hardware ray tracing not supported, using software ray casting");
        }
    }
    
    return InitializationResult::Success();
}

InitializationResult AppInitializer::InitializeInputHandler() {
    if (!m_renderer || !m_windowManager || !m_windowManager->GetWindow() || !m_configProvider) {
        return InitializationResult::Failure("Invalid parameters for input handler initialization");
    }
    
    m_inputHandler = new InputHandler();
    if (!m_inputHandler) {
        return InitializationResult::Failure("Failed to create InputHandler");
    }
    
    m_inputHandler->Initialize(m_renderer, m_windowManager->GetWindow(), m_configProvider->GetStretchMode());
    return InitializationResult::Success();
}

bool AppInitializer::InitializeManagers() {
    m_sceneManager = std::make_unique<SceneManager>();
    m_uiManager = std::make_unique<UIManager>();
    m_eventManager = std::make_unique<EventManager>();
    m_renderScheduler = std::make_unique<RenderScheduler>();
    m_messageHandler = std::make_unique<WindowMessageHandler>();
    return true;
}

InitializationResult AppInitializer::InitializeUI() {
    if (!m_renderer || !m_windowManager || !m_windowManager->GetWindow() || !m_configProvider) {
        return InitializationResult::Failure("Invalid parameters for UI initialization");
    }
    
    // 初始化文字渲染器
    m_textRenderer = new TextRenderer();
    if (!m_textRenderer) {
        return InitializationResult::Failure("Failed to create TextRenderer");
    }
    
    if (!m_textRenderer->Initialize(
            m_renderer->GetDevice(),
            m_renderer->GetPhysicalDevice(),
            m_renderer->GetCommandPool(),
            m_renderer->GetGraphicsQueue(),
            m_renderer->GetRenderPass())) {
        delete m_textRenderer;
        m_textRenderer = nullptr;
        return InitializationResult::Failure("Failed to initialize TextRenderer");
    }
    
    m_textRenderer->LoadFont("Microsoft YaHei", 24);
    
    // 初始化UI管理器
    if (!m_uiManager->Initialize(m_renderer, m_textRenderer, m_windowManager->GetWindow(), m_configProvider->GetStretchMode())) {
        if (m_textRenderer) {
            m_textRenderer->Cleanup();
            delete m_textRenderer;
            m_textRenderer = nullptr;
        }
        return InitializationResult::Failure("Failed to initialize UIManager");
    }
    
    // 设置UI组件的回调函数（使用事件总线解耦）
    IEventBus* eventBus = m_eventBus ? m_eventBus : &EventBus::GetInstance();
    m_uiManager->SetupCallbacks(eventBus);
    
    return InitializationResult::Success();
}

InitializationResult AppInitializer::InitializeEventSystem() {
    if (!m_inputHandler || !m_uiManager || !m_renderer || !m_windowManager || !m_sceneManager || !m_configProvider) {
        return InitializationResult::Failure("Invalid parameters for event system initialization");
    }
    
    // 获取事件总线（使用依赖注入或单例）
    IEventBus* eventBus = m_eventBus ? m_eventBus : &EventBus::GetInstance();
    
    // 初始化事件管理器（使用ISceneProvider接口）
    m_eventManager->Initialize(m_inputHandler, m_uiManager.get(), m_renderer, 
                               m_windowManager->GetWindow(), m_sceneManager.get(), eventBus);
    
    // 设置事件处理器（处理按钮点击等事件）
    m_eventManager->SetupEventHandlers(m_sceneManager.get(), m_renderer, m_configProvider);
    
    // 初始化窗口消息处理器（返回 void，无需检查）
    m_messageHandler->Initialize(m_eventManager.get(), m_windowManager->GetWindow(), 
                                m_configProvider->GetStretchMode(), m_renderer);
    
    return InitializationResult::Success();
}

InitializationResult AppInitializer::InitializeRenderScheduler() {
    if (!m_renderer || !m_sceneManager || !m_uiManager || !m_textRenderer || !m_windowManager || !m_configProvider || !m_inputHandler) {
        return InitializationResult::Failure("Invalid parameters for render scheduler initialization");
    }
    
    // 使用接口而不是具体类（依赖注入）
    m_renderScheduler->Initialize(m_renderer, 
                                  m_sceneManager.get(),  // ISceneProvider*
                                  m_uiManager.get(),    // IUIRenderProvider*
                                  m_inputHandler,       // IInputProvider*
                                  m_textRenderer, 
                                  m_windowManager->GetWindow(), 
                                  m_configProvider->GetStretchMode());
    
    return InitializationResult::Success();
}

ISceneProvider* AppInitializer::GetSceneProvider() const {
    return m_sceneManager.get();  // SceneManager 继承自 ISceneProvider，可以隐式转换
}

void AppInitializer::CleanupPartial(int initializedSteps) {
    // 按初始化顺序的逆序进行部分清理
    // 注意：这里只清理已初始化的步骤，避免访问未初始化的资源
    
    // 步骤9: 清理渲染调度器
    if (initializedSteps >= 10) {
        m_renderScheduler.reset();
    }
    
    // 步骤8: 清理事件系统
    if (initializedSteps >= 9) {
        m_messageHandler.reset();
        m_eventManager.reset();
    }
    
    // 步骤7: 清理UI
    if (initializedSteps >= 8) {
        m_uiManager.reset();
    }
    
    // 步骤6: 清理管理器
    if (initializedSteps >= 7) {
        m_sceneManager.reset();
    }
    
    // 步骤5: 清理输入处理器
    if (initializedSteps >= 6) {
        if (m_inputHandler) {
            delete m_inputHandler;
            m_inputHandler = nullptr;
        }
    }
    
    // 步骤4: 清理渲染器
    if (initializedSteps >= 5) {
        if (m_textRenderer) {
            m_textRenderer->Cleanup();
            delete m_textRenderer;
            m_textRenderer = nullptr;
        }
        if (m_renderer && m_rendererFactory) {
            m_rendererFactory->DestroyRenderer(m_renderer);
            m_renderer = nullptr;
        }
    }
    
    // 步骤3: 清理窗口
    if (initializedSteps >= 4) {
        if (m_windowManager) {
            m_windowManager->Cleanup();
            m_windowManager.reset();
        }
    }
    
    // 步骤2-3: 清理日志和控制台（最后清理，因为其他步骤可能需要日志）
    if (initializedSteps >= 3) {
        if (m_logger) {
            m_logger->Shutdown();
        } else {
            // 向后兼容：使用单例
            Logger::GetInstance().Shutdown();
        }
    }
    
    if (initializedSteps >= 2) {
        if (m_pCout) {
            fclose(m_pCout);
            m_pCout = nullptr;
        }
        if (m_pCin) {
            fclose(m_pCin);
            m_pCin = nullptr;
        }
        if (m_pCerr) {
            fclose(m_pCerr);
            m_pCerr = nullptr;
        }
        FreeConsole();
    }
    
    // 步骤1: 配置管理器不需要清理（静态变量）
}

void AppInitializer::Cleanup() {
    if (!m_initialized) {
        return;
    }
    
    if (m_logger) {
        m_logger->Info("Application cleaning up...");
    }
    
    // 清理顺序与初始化顺序相反（RAII原则）
    // 1. 清理事件总线和订阅
    if (m_eventBus) {
        m_eventBus->Clear();
    } else {
        // 向后兼容：使用单例
        EventBus::GetInstance().Clear();
    }
    
    // 2. 清理管理器（按依赖顺序：依赖其他管理器的先清理）
    m_renderScheduler.reset();
    m_messageHandler.reset();
    m_eventManager.reset();
    m_uiManager.reset();
    m_sceneManager.reset();
    
    // 3. 清理渲染相关资源
    if (m_textRenderer) {
        m_textRenderer->Cleanup();
        delete m_textRenderer;
        m_textRenderer = nullptr;
    }
    
    // 4. 清理渲染器
    if (m_renderer && m_rendererFactory) {
        m_rendererFactory->DestroyRenderer(m_renderer);
        m_renderer = nullptr;
    }
    
    // 5. 清理输入处理器
    if (m_inputHandler) {
        delete m_inputHandler;
        m_inputHandler = nullptr;
    }
    
    // 6. 清理窗口管理器
    if (m_windowManager) {
        m_windowManager->Cleanup();
        m_windowManager.reset();
    }
    
    // 7. 清理控制台资源
    if (m_pCout) {
        fclose(m_pCout);
        m_pCout = nullptr;
    }
    if (m_pCin) {
        fclose(m_pCin);
        m_pCin = nullptr;
    }
    if (m_pCerr) {
        fclose(m_pCerr);
        m_pCerr = nullptr;
    }
    
    FreeConsole();
    
    // 8. 清理日志系统
    if (m_logger) {
        m_logger->Shutdown();
    } else {
        // 向后兼容：使用单例
        Logger::GetInstance().Shutdown();
    }
    
    m_initialized = false;
    if (m_logger) {
        m_logger->Info("Application cleanup completed");
    }
}


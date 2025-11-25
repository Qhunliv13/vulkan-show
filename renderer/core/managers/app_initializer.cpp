#include "core/managers/app_initializer.h"
#include "core/managers/config_manager.h"
#include "core/interfaces/iconfig_provider.h"
#include "core/interfaces/iscene_provider.h"
#include "core/managers/event_manager.h"
#include "core/managers/window_manager.h"
#include "core/interfaces/irenderer_factory.h"
#include "core/interfaces/irenderer.h"
#include "core/utils/logger.h"
#include "core/utils/event_bus.h"
#include "core/ui/ui_manager.h"
#include "core/managers/event_manager.h"
#include "core/managers/scene_manager.h"
#include "core/managers/render_scheduler.h"
#include "core/handlers/window_message_handler.h"
#include "core/utils/input_handler.h"
#include "text/text_renderer.h"
#include "window/window.h"
#include <windows.h>
#include <io.h>
#include <fcntl.h>

AppInitializer::AppInitializer() {
}

AppInitializer::~AppInitializer() {
    Cleanup();
}

bool AppInitializer::Initialize(IRendererFactory* rendererFactory, HINSTANCE hInstance, const char* lpCmdLine) {
    if (m_initialized) {
        return true;
    }
    
    // 1. 初始化配置（最先，其他组件依赖配置）
    if (!InitializeConfig(lpCmdLine)) {
        return false;
    }
    
    // 2. 初始化控制台（日志系统需要）
    InitializeConsole();
    
    // 3. 初始化日志系统
    if (!InitializeLogger()) {
        printf("[WARNING] Failed to initialize logger, continuing without file logging\n");
    }
    
    LOG_INFO("Application initializing...");
    
    // 4. 初始化窗口（渲染器依赖窗口）
    if (!InitializeWindow(hInstance)) {
        LOG_ERROR("Failed to initialize window");
        return false;
    }
    
    // 5. 初始化渲染器（UI和场景依赖渲染器）
    if (!InitializeRenderer(rendererFactory, hInstance)) {
        LOG_ERROR("Failed to initialize renderer");
        return false;
    }
    
    // 6. 初始化输入处理器（事件管理器依赖）
    if (!InitializeInputHandler()) {
        LOG_ERROR("Failed to initialize input handler");
        return false;
    }
    
    // 7. 初始化管理器（基础组件）
    if (!InitializeManagers()) {
        LOG_ERROR("Failed to initialize managers");
        return false;
    }
    
    // 8. 初始化UI（依赖渲染器和窗口）
    if (!InitializeUI()) {
        LOG_ERROR("Failed to initialize UI");
        return false;
    }
    
    // 9. 初始化事件系统（依赖UI、场景、输入处理器）
    if (!InitializeEventSystem()) {
        LOG_ERROR("Failed to initialize event system");
        return false;
    }
    
    // 10. 初始化渲染调度器（依赖所有其他组件）
    if (!InitializeRenderScheduler()) {
        LOG_ERROR("Failed to initialize render scheduler");
        return false;
    }
    
    m_initialized = true;
    LOG_INFO("Application initialized successfully");
    return true;
}

bool AppInitializer::InitializeConfig(const char* lpCmdLine) {
    // 初始化配置管理器并保存引用（用于依赖注入）
    m_configProvider = &ConfigManager::GetInstance();
    m_configProvider->Initialize(lpCmdLine);
    return true;
}

bool AppInitializer::InitializeConsole() {
    AllocConsole();
    freopen_s(&m_pCout, "CONOUT$", "w", stdout);
    freopen_s(&m_pCin, "CONIN$", "r", stdin);
    freopen_s(&m_pCerr, "CONOUT$", "w", stderr);
    SetConsoleTitleA("Shader App Debug Console");
    return true;
}

bool AppInitializer::InitializeLogger() {
    if (!m_configProvider) {
        return false;
    }
    
    std::string logPath = m_configProvider->GetLogPath();
    
    if (!Logger::GetInstance().Initialize(logPath)) {
        // 尝试不指定文件，只使用控制台输出
        Logger::GetInstance().Initialize("");
        return false;
    }
    return true;
}

bool AppInitializer::InitializeWindow(HINSTANCE hInstance) {
    if (!m_configProvider) {
        return false;
    }
    
    m_windowManager = std::make_unique<WindowManager>();
    // 使用依赖注入传递配置提供者
    return m_windowManager->Initialize(hInstance, m_configProvider);
}

bool AppInitializer::InitializeRenderer(IRendererFactory* rendererFactory, HINSTANCE hInstance) {
    if (!rendererFactory || !m_windowManager || !m_windowManager->GetWindow()) {
        return false;
    }
    
    m_rendererFactory = rendererFactory;
    
    // 使用工厂创建渲染器
    m_renderer = rendererFactory->CreateRenderer();
    if (!m_renderer || !m_renderer->Initialize(m_windowManager->GetWindow()->GetHandle(), hInstance)) {
        if (m_renderer && rendererFactory) {
            rendererFactory->DestroyRenderer(m_renderer);
            m_renderer = nullptr;
        }
        return false;
    }
    
    // 从配置提供者获取配置（依赖注入）
    if (!m_configProvider) {
        return false;
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
    
    // 加载背景纹理
    if (!m_renderer->LoadBackgroundTexture(m_configProvider->GetBackgroundTexturePath().c_str())) {
        printf("[WARNING] Failed to load background texture, continuing without background\n");
    }
    
    // 尝试创建ray tracing pipeline
    if (m_renderer->IsRayTracingSupported()) {
        printf("[INFO] Hardware ray tracing is supported, attempting to create pipeline...\n");
        if (m_renderer->CreateRayTracingPipeline()) {
            printf("[INFO] Hardware ray tracing pipeline created successfully!\n");
        } else {
            printf("[INFO] Hardware ray tracing pipeline creation failed, will use software ray casting\n");
        }
    } else {
        printf("[INFO] Hardware ray tracing not supported, using software ray casting\n");
    }
    
    return true;
}

bool AppInitializer::InitializeInputHandler() {
    if (!m_renderer || !m_windowManager || !m_windowManager->GetWindow() || !m_configProvider) {
        return false;
    }
    
    m_inputHandler = new InputHandler();
    m_inputHandler->Initialize(m_renderer, m_windowManager->GetWindow(), m_configProvider->GetStretchMode());
    return true;
}

bool AppInitializer::InitializeManagers() {
    m_sceneManager = std::make_unique<SceneManager>();
    m_uiManager = std::make_unique<UIManager>();
    m_eventManager = std::make_unique<EventManager>();
    m_renderScheduler = std::make_unique<RenderScheduler>();
    m_messageHandler = std::make_unique<WindowMessageHandler>();
    return true;
}

bool AppInitializer::InitializeUI() {
    if (!m_renderer || !m_windowManager || !m_windowManager->GetWindow() || !m_configProvider) {
        return false;
    }
    
    // 初始化文字渲染器
    m_textRenderer = new TextRenderer();
    bool textRendererInitialized = false;
    if (m_textRenderer->Initialize(
            m_renderer->GetDevice(),
            m_renderer->GetPhysicalDevice(),
            m_renderer->GetCommandPool(),
            m_renderer->GetGraphicsQueue(),
            m_renderer->GetRenderPass())) {
        textRendererInitialized = true;
        m_textRenderer->LoadFont("Microsoft YaHei", 24);
    }
    
    // 初始化UI管理器
    if (!m_uiManager->Initialize(m_renderer, m_textRenderer, m_windowManager->GetWindow(), m_configProvider->GetStretchMode())) {
        return false;
    }
    
    // 设置UI组件的回调函数（传递配置提供者）
    m_uiManager->SetupCallbacks(m_sceneManager.get(), m_renderer, m_configProvider);
    
    return true;
}

bool AppInitializer::InitializeEventSystem() {
    if (!m_inputHandler || !m_uiManager || !m_renderer || !m_windowManager || !m_sceneManager || !m_configProvider) {
        return false;
    }
    
    // 初始化事件管理器（使用ISceneProvider接口）
    m_eventManager->Initialize(m_inputHandler, m_uiManager.get(), m_renderer, 
                               m_windowManager->GetWindow(), m_sceneManager.get());
    
    // 初始化窗口消息处理器
    m_messageHandler->Initialize(m_eventManager.get(), m_windowManager->GetWindow(), 
                                m_configProvider->GetStretchMode(), m_renderer);
    
    return true;
}

bool AppInitializer::InitializeRenderScheduler() {
    if (!m_renderer || !m_sceneManager || !m_uiManager || !m_textRenderer || !m_windowManager || !m_configProvider) {
        return false;
    }
    
    m_renderScheduler->Initialize(m_renderer, m_sceneManager.get(), 
                                  m_uiManager.get(), m_textRenderer, 
                                  m_windowManager->GetWindow(), m_configProvider->GetStretchMode());
    
    return true;
}

ISceneProvider* AppInitializer::GetSceneProvider() const {
    return m_sceneManager.get();  // SceneManager 继承自 ISceneProvider，可以隐式转换
}

void AppInitializer::Cleanup() {
    if (!m_initialized) {
        return;
    }
    
    LOG_INFO("Application cleaning up...");
    
    // 清理顺序与初始化顺序相反（RAII原则）
    // 1. 清理事件总线和订阅
    EventBus::GetInstance().Clear();
    
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
    Logger::GetInstance().Shutdown();
    
    m_initialized = false;
    LOG_INFO("Application cleanup completed");
}


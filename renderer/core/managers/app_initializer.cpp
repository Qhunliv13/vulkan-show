#include "core/managers/app_initializer.h"  // 1. 对应头文件

#include <fcntl.h>  // 2. 系统头文件
#include <io.h>  // 2. 系统头文件
#include <windows.h>  // 2. 系统头文件

#include "core/managers/app_initialization_config.h"  // 4. 项目头文件（配置）
#include "core/managers/initialization_result.h"  // 4. 项目头文件（初始化结果）
#include "core/interfaces/icamera_controller.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iconfig_provider.h"  // 4. 项目头文件（接口）
#include "core/interfaces/ievent_bus.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iinput_provider.h"  // 4. 项目头文件（接口）
#include "core/interfaces/ilogger.h"  // 4. 项目头文件（接口）
#include "core/interfaces/ipipeline_manager.h"  // 4. 项目头文件（接口）
#include "core/interfaces/irender_device.h"  // 4. 项目头文件（接口）
#include "core/interfaces/irenderer.h"  // 4. 项目头文件（接口）
#include "core/interfaces/irenderer_factory.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iscene_provider.h"  // 4. 项目头文件（接口）
#include "core/interfaces/itext_renderer.h"  // 4. 项目头文件（接口）
#include "core/interfaces/itext_renderer_factory.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iwindow_factory.h"  // 4. 项目头文件（接口）
#include "core/handlers/window_message_handler.h"  // 4. 项目头文件（处理器）
#include "core/managers/event_manager.h"  // 4. 项目头文件（管理器）
#include "core/managers/render_scheduler.h"  // 4. 项目头文件（管理器）
#include "core/managers/scene_manager.h"  // 4. 项目头文件（管理器）
#include "core/managers/window_manager.h"  // 4. 项目头文件（管理器）
#include "core/ui/ui_manager.h"  // 4. 项目头文件（UI）
#include "core/ui/ui_render_provider_adapter.h"  // 4. 项目头文件（UI）
#include "core/ui/ui_window_resize_adapter.h"  // 4. 项目头文件（UI）
#include "core/utils/event_bus.h"  // 4. 项目头文件（工具）
#include "core/utils/input_handler.h"  // 4. 项目头文件（工具）
#include "core/utils/logger.h"  // 4. 项目头文件（工具）
#include "text/text_renderer.h"  // 4. 项目头文件（文字渲染器）
#include "window/window.h"  // 4. 项目头文件（窗口）

AppInitializer::AppInitializer() {
}

AppInitializer::~AppInitializer() {
    Cleanup();
}

bool AppInitializer::Initialize(const AppInitializationConfig& config) {
    if (m_initialized) {
        return true;
    }
    
    // 验证配置有效性
    if (!config.IsValid()) {
        printf("[ERROR] AppInitializer::Initialize: Invalid configuration\n");
        return false;
    }
    
    // 设置依赖注入的组件
    m_configProvider = config.configProvider;
    m_logger = config.logger;
    m_eventBus = config.eventBus;
    m_windowFactory = config.windowFactory;
    m_textRendererFactory = config.textRendererFactory;
    
    // 初始化配置（最先，其他组件依赖配置）
    m_configProvider->Initialize(config.lpCmdLine);
    
    // 初始化步骤计数器（用于回滚）
    int initializedSteps = 0;
    
    // 1. 初始化控制台（日志系统需要）
    InitializeConsole();
    initializedSteps = 1;
    
    // 2. 初始化日志系统
    InitializationResult loggerResult = InitializeLogger();
    if (!loggerResult.success) {
        // 日志系统失败时继续，但记录警告
        printf("[WARNING] Failed to initialize logger: %s, continuing without file logging\n", 
               loggerResult.errorMessage.c_str());
    }
    initializedSteps = 2;
    
    if (m_logger) {
        m_logger->Info("Application initializing...");
    }
    
    // 3. 初始化窗口（渲染器依赖窗口）
    InitializationResult windowResult = InitializeWindow(config.hInstance);
    if (!windowResult.success) {
        if (m_logger) {
            m_logger->Error(windowResult.errorMessage.empty() ? 
                           "Failed to initialize window" : windowResult.errorMessage);
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 3;
    
    // 4. 初始化渲染器（UI和场景依赖渲染器）
    InitializationResult rendererResult = InitializeRenderer(config.rendererFactory, config.hInstance);
    if (!rendererResult.success) {
        if (m_logger) {
            m_logger->Error(rendererResult.errorMessage.empty() ? 
                           "Failed to initialize renderer" : rendererResult.errorMessage);
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 4;
    
    // 5. 初始化输入处理器（事件管理器依赖）
    InitializationResult inputResult = InitializeInputHandler();
    if (!inputResult.success) {
        if (m_logger) {
            m_logger->Error(inputResult.errorMessage.empty() ? 
                           "Failed to initialize input handler" : inputResult.errorMessage);
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 5;
    
    // 6. 初始化管理器（基础组件）
    if (!InitializeManagers()) {
        if (m_logger) {
            m_logger->Error("Failed to initialize managers");
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 6;
    
    // 7. 初始化UI（依赖渲染器和窗口）
    InitializationResult uiResult = InitializeUI();
    if (!uiResult.success) {
        if (m_logger) {
            m_logger->Error(uiResult.errorMessage.empty() ? 
                           "Failed to initialize UI" : uiResult.errorMessage);
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 7;
    
    // 8. 初始化事件系统（依赖UI、场景、输入处理器）
    InitializationResult eventResult = InitializeEventSystem();
    if (!eventResult.success) {
        if (m_logger) {
            m_logger->Error(eventResult.errorMessage.empty() ? 
                           "Failed to initialize event system" : eventResult.errorMessage);
        }
        CleanupPartial(initializedSteps);
        return false;
    }
    initializedSteps = 8;
    
    // 9. 初始化渲染调度器（依赖所有其他组件）
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

bool AppInitializer::InitializeConsole() {
    AllocConsole();
    freopen_s(&m_pCout, "CONOUT$", "w", stdout);
    freopen_s(&m_pCin, "CONIN$", "r", stdin);
    freopen_s(&m_pCerr, "CONOUT$", "w", stderr);
    SetConsoleTitleA("Shader App Debug Console");
    return true;
}

InitializationResult AppInitializer::InitializeLogger() {
    if (!m_configProvider || !m_logger) {
        return InitializationResult::Failure("ConfigProvider or Logger not initialized");
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
    if (!m_configProvider || !m_windowFactory || !m_eventBus) {
        return InitializationResult::Failure("Required dependencies not initialized");
    }
    
    m_windowManager = std::make_unique<WindowManager>();
    // 使用依赖注入传递所有必需的依赖（包括日志器）
    if (!m_windowManager->Initialize(hInstance, m_configProvider, m_windowFactory, m_eventBus, m_logger)) {
        return InitializationResult::Failure("Failed to initialize WindowManager");
    }
    return InitializationResult::Success();
}

InitializationResult AppInitializer::InitializeRenderer(IRendererFactory* rendererFactory, HINSTANCE hInstance) {
    if (!rendererFactory || !m_windowManager || !m_windowManager->GetWindow()) {
        return InitializationResult::Failure("Invalid parameters for renderer initialization");
    }
    
    m_rendererFactory = rendererFactory;
    
    // 使用工厂创建渲染器（通过 unique_ptr 自动管理生命周期）
    m_renderer = rendererFactory->CreateRenderer();
    if (!m_renderer) {
        return InitializationResult::Failure("Failed to create renderer from factory");
    }
    
    if (!m_renderer->Initialize(m_windowManager->GetWindow()->GetHandle(), hInstance)) {
        m_renderer.reset();  // unique_ptr 自动清理
        return InitializationResult::Failure("Failed to initialize renderer");
    }
    
    // 从配置提供者获取配置（依赖注入）
    if (!m_configProvider) {
        m_renderer.reset();  // unique_ptr 自动清理
        return InitializationResult::Failure("ConfigProvider not initialized");
    }
    m_renderer->SetStretchMode(m_configProvider->GetStretchMode());
    m_renderer->SetBackgroundStretchMode(m_configProvider->GetBackgroundStretchMode());
    
    // 通过事件总线订阅鼠标移动事件（替代回调函数）
    // 使用 SubscribeWithId 保存订阅ID，以便在 Cleanup() 时取消订阅
    m_mouseMovedSubscriptionId = m_eventBus->SubscribeWithId(EventType::MouseMoved, [this](const Event& e) {
        const MouseMovedEvent& mouseEvent = static_cast<const MouseMovedEvent&>(e);
        if (m_renderer) {
            ICameraController* cameraController = m_renderer->GetCameraController();
            if (cameraController) {
                cameraController->SetMouseInput(mouseEvent.deltaX, mouseEvent.deltaY, mouseEvent.leftButtonDown);
            }
        }
    });
    
    // 通过事件总线订阅键盘事件（替代回调函数）
    // 使用 SubscribeWithId 保存订阅ID，以便在 Cleanup() 时取消订阅
    m_keyPressedSubscriptionId = m_eventBus->SubscribeWithId(EventType::KeyPressed, [](const Event& e) {
        const KeyPressedEvent& keyEvent = static_cast<const KeyPressedEvent&>(e);
        // 按键事件通过事件总线分发，具体处理逻辑在订阅者中实现，解耦输入和渲染逻辑
    });
    
    // 加载背景纹理（非关键，失败时继续）
    if (!m_renderer->LoadBackgroundTexture(m_configProvider->GetBackgroundTexturePath().c_str())) {
        if (m_logger) {
            m_logger->Warning("Failed to load background texture, continuing without background");
        }
    }
    
    // 尝试创建ray tracing pipeline（非关键，失败时继续）
    IPipelineManager* pipelineManager = m_renderer->GetPipelineManager();
    if (pipelineManager && pipelineManager->IsRayTracingSupported()) {
        if (m_logger) {
            m_logger->Info("Hardware ray tracing is supported, attempting to create pipeline...");
        }
        if (pipelineManager->CreateRayTracingPipeline()) {
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
    
    auto inputHandler = std::make_unique<InputHandler>();
    if (!inputHandler) {
        return InitializationResult::Failure("Failed to create InputHandler");
    }
    
    inputHandler->Initialize(m_renderer.get(), m_windowManager->GetWindow(), m_configProvider->GetStretchMode());
    m_inputHandler = inputHandler.get();  // InputHandler 实现了 IInputHandler 接口
    m_inputHandlerImpl = std::move(inputHandler);  // 使用 unique_ptr 管理生命周期
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
    
    // 使用工厂创建文字渲染器
    if (!m_textRendererFactory) {
        return InitializationResult::Failure("TextRendererFactory not initialized");
    }
    m_textRenderer = m_textRendererFactory->CreateTextRenderer();
    if (!m_textRenderer) {
        return InitializationResult::Failure("Failed to create TextRenderer from factory");
    }
    
    // 通过 IRenderDevice 接口获取设备信息（遵循接口隔离原则）
    IRenderDevice* renderDevice = m_renderer->GetRenderDevice();
    if (!renderDevice) {
        m_textRenderer.reset();
        return InitializationResult::Failure("Renderer does not provide IRenderDevice interface");
    }
    
    if (!m_textRenderer->Initialize(
            renderDevice->GetDevice(),
            renderDevice->GetPhysicalDevice(),
            renderDevice->GetCommandPool(),
            renderDevice->GetGraphicsQueue(),
            renderDevice->GetRenderPass())) {
        m_textRenderer.reset();
        return InitializationResult::Failure("Failed to initialize TextRenderer");
    }
    
    m_textRenderer->LoadFont("Microsoft YaHei", 24);
    
    // 初始化UI管理器
    if (!m_uiManager->Initialize(m_renderer.get(), m_textRenderer.get(), m_windowManager->GetWindow(), m_configProvider->GetStretchMode())) {
        m_textRenderer.reset();
        return InitializationResult::Failure("Failed to initialize UIManager");
    }
    
    // 设置UI组件的回调函数（使用事件总线解耦）
    m_uiManager->SetupCallbacks(m_eventBus);
    
    // 创建适配器（实现接口职责单一原则）
    m_uiRenderProviderAdapter = std::make_unique<UIRenderProviderAdapter>(m_uiManager.get());
    m_uiWindowResizeAdapter = std::make_unique<UIWindowResizeAdapter>(m_uiManager.get());
    
    return InitializationResult::Success();
}

InitializationResult AppInitializer::InitializeEventSystem() {
    if (!m_inputHandler || !m_uiManager || !m_renderer || !m_windowManager || !m_sceneManager || !m_configProvider) {
        return InitializationResult::Failure("Invalid parameters for event system initialization");
    }
    
    // 初始化事件管理器（使用接口而不是具体类，只通过事件总线通信）
    m_eventManager->Initialize(m_inputHandler, m_renderer.get(), 
                               m_windowManager->GetWindow(), m_sceneManager.get(), m_eventBus);
    
    // 让 UIManager 订阅事件（通过事件总线实现解耦）
    if (m_uiManager) {
        m_uiManager->SubscribeToEvents(m_eventBus);
    }
    
    // 设置事件处理器（通过事件总线订阅按钮点击事件，处理场景切换）
    // 将事件处理逻辑放在AppInitializer中，避免EventManager直接依赖SceneManager
    // 使用 SubscribeWithId 保存订阅ID，以便在 Cleanup() 时取消订阅
    m_buttonClickedSubscriptionId = m_eventBus->SubscribeWithId(EventType::ButtonClicked, [this](const Event& e) {
        const ButtonClickedEvent& buttonEvent = static_cast<const ButtonClickedEvent&>(e);
        
        if (buttonEvent.buttonId == "enter") {
            if (m_logger) {
                m_logger->Info("Button clicked! Switching to Shader mode");
            }
            m_sceneManager->SwitchToShader(m_renderer.get(), m_configProvider);
        } else if (buttonEvent.buttonId == "left") {
            if (m_logger) {
                m_logger->Info("Left button clicked! Entering 3D scene (LoadingCubes)");
            }
            m_sceneManager->SwitchToLoadingCubes(m_renderer.get(), m_configProvider);
        }
    });
    
    // 初始化窗口消息处理器（返回 void，无需检查）
    m_messageHandler->Initialize(m_eventManager.get(), m_windowManager->GetWindow(), 
                                m_configProvider->GetStretchMode(), m_renderer.get());
    
    return InitializationResult::Success();
}

InitializationResult AppInitializer::InitializeRenderScheduler() {
    if (!m_renderer || !m_sceneManager || !m_uiManager || !m_textRenderer.get() || !m_windowManager || !m_configProvider || !m_inputHandler) {
        return InitializationResult::Failure("Invalid parameters for render scheduler initialization");
    }
    
    // 使用接口而不是具体类（依赖注入）
    // InputHandler 同时实现了 IInputProvider 和 IInputHandler 接口
    // 通过保存的具体实现指针进行类型转换
    IInputProvider* inputProvider = static_cast<IInputProvider*>(m_inputHandlerImpl.get());
    // 使用适配器而不是直接使用 UIManager，实现接口职责单一
    IUIRenderProvider* uiRenderProvider = m_uiRenderProviderAdapter.get();
    m_renderScheduler->Initialize(m_renderer.get(), 
                                  m_sceneManager.get(),  // ISceneProvider*
                                  uiRenderProvider,      // IUIRenderProvider* (通过适配器)
                                  inputProvider,         // IInputProvider*
                                  m_textRenderer.get(), 
                                  m_windowManager->GetWindow(), 
                                  m_configProvider->GetStretchMode());
    
    return InitializationResult::Success();
}

ISceneProvider* AppInitializer::GetSceneProvider() const {
    return m_sceneManager.get();  // SceneManager 继承自 ISceneProvider，可以隐式转换
}

IUIManager* AppInitializer::GetUIManager() const {
    return m_uiManager.get();  // UIManager 实现了 IUIManager 接口，可以隐式转换
}

void AppInitializer::CleanupPartial(int initializedSteps) {
    // 按初始化顺序的逆序进行部分清理
    // 注意：这里只清理已初始化的步骤，避免访问未初始化的资源
    
    // 步骤9: 清理渲染调度器
    if (initializedSteps >= 10) {
        m_renderScheduler.reset();
    }
    
    // 步骤8: 清理事件系统（包括取消事件订阅）
    if (initializedSteps >= 9) {
        // 取消所有事件订阅（在清理组件之前取消，避免悬空指针）
        if (m_eventBus) {
            if (m_buttonClickedSubscriptionId != 0) {
                m_eventBus->Unsubscribe(EventType::ButtonClicked, m_buttonClickedSubscriptionId);
                m_buttonClickedSubscriptionId = 0;
            }
            // 同时取消在 InitializeRenderer() 中订阅的事件
            if (m_mouseMovedSubscriptionId != 0) {
                m_eventBus->Unsubscribe(EventType::MouseMoved, m_mouseMovedSubscriptionId);
                m_mouseMovedSubscriptionId = 0;
            }
            if (m_keyPressedSubscriptionId != 0) {
                m_eventBus->Unsubscribe(EventType::KeyPressed, m_keyPressedSubscriptionId);
                m_keyPressedSubscriptionId = 0;
            }
        }
        m_messageHandler.reset();
        m_eventManager.reset();
    }
    
    // 步骤5: 如果渲染器已初始化但事件系统未初始化，需要取消在 InitializeRenderer() 中订阅的事件
    if (initializedSteps >= 5 && initializedSteps < 9) {
        // 只取消在 InitializeRenderer() 中订阅的事件（MouseMoved 和 KeyPressed）
        if (m_eventBus) {
            if (m_mouseMovedSubscriptionId != 0) {
                m_eventBus->Unsubscribe(EventType::MouseMoved, m_mouseMovedSubscriptionId);
                m_mouseMovedSubscriptionId = 0;
            }
            if (m_keyPressedSubscriptionId != 0) {
                m_eventBus->Unsubscribe(EventType::KeyPressed, m_keyPressedSubscriptionId);
                m_keyPressedSubscriptionId = 0;
            }
        }
    }
    
    // 步骤7: 清理UI和适配器
    if (initializedSteps >= 8) {
        m_uiRenderProviderAdapter.reset();
        m_uiWindowResizeAdapter.reset();
        m_uiManager.reset();
    }
    
    // 步骤6: 清理管理器
    if (initializedSteps >= 7) {
        m_sceneManager.reset();
    }
    
    // 步骤5: 清理输入处理器
    if (initializedSteps >= 6) {
        if (m_inputHandlerImpl) {
            m_inputHandlerImpl.reset();
            m_inputHandler = nullptr;
        }
    }
    
    // 步骤4: 清理渲染器
    if (initializedSteps >= 5) {
        if (m_textRenderer) {
            m_textRenderer->Cleanup();
            m_textRenderer.reset();
        }
        // unique_ptr 自动管理渲染器生命周期，调用 Cleanup() 后重置
        if (m_renderer) {
            m_renderer->Cleanup();
            m_renderer.reset();
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
    // 1. 取消所有事件订阅（必须在清理组件之前取消，避免悬空指针）
    if (m_eventBus) {
        if (m_mouseMovedSubscriptionId != 0) {
            m_eventBus->Unsubscribe(EventType::MouseMoved, m_mouseMovedSubscriptionId);
            m_mouseMovedSubscriptionId = 0;
        }
        if (m_keyPressedSubscriptionId != 0) {
            m_eventBus->Unsubscribe(EventType::KeyPressed, m_keyPressedSubscriptionId);
            m_keyPressedSubscriptionId = 0;
        }
        if (m_buttonClickedSubscriptionId != 0) {
            m_eventBus->Unsubscribe(EventType::ButtonClicked, m_buttonClickedSubscriptionId);
            m_buttonClickedSubscriptionId = 0;
        }
    }
    
    // 2. 清理管理器（按依赖顺序：依赖其他管理器的先清理）
    m_renderScheduler.reset();
    m_messageHandler.reset();
    m_eventManager.reset();
    m_uiRenderProviderAdapter.reset();
    m_uiWindowResizeAdapter.reset();
    m_uiManager.reset();
    m_sceneManager.reset();
    
    // 3. 清理渲染相关资源
    if (m_textRenderer) {
        m_textRenderer->Cleanup();
        m_textRenderer.reset();
    }
    
    // 4. 清理渲染器（unique_ptr 自动管理生命周期）
    if (m_renderer) {
        m_renderer->Cleanup();
        m_renderer.reset();
    }
    
    // 5. 清理输入处理器
    if (m_inputHandlerImpl) {
        m_inputHandlerImpl.reset();
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
    }
    
    m_initialized = false;
    if (m_logger) {
        m_logger->Info("Application cleanup completed");
    }
}


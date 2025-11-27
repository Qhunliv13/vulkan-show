#include "core/managers/application.h"  // 1. 对应头文件

#include <memory>  // 2. 系统头文件
#include <windows.h>  // 2. 系统头文件

#include "core/interfaces/irenderer_factory.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iconfig_provider.h"  // 4. 项目头文件（接口）
#include "core/managers/app_initializer.h"  // 4. 项目头文件（管理器）
#include "core/managers/app_initialization_config.h"  // 4. 项目头文件（配置）
#include "core/managers/window_manager.h"  // 4. 项目头文件（管理器）
#include "core/managers/event_manager.h"  // 4. 项目头文件（管理器）
#include "core/managers/render_scheduler.h"  // 4. 项目头文件（管理器）
#include "core/managers/config_manager.h"  // 4. 项目头文件（管理器）
#include "core/utils/fps_monitor.h"  // 4. 项目头文件（工具）
#include "core/utils/logger.h"  // 4. 项目头文件（工具）
#include "core/utils/event_bus.h"  // 4. 项目头文件（工具）
#include "core/factories/window_factory.h"  // 4. 项目头文件（工厂）
#include "core/factories/text_renderer_factory.h"  // 4. 项目头文件（工厂）
#include "window/window.h"  // 4. 项目头文件（窗口）

Application::Application() {
}

Application::~Application() {
    Cleanup();
}

bool Application::Initialize(IRendererFactory* rendererFactory, HINSTANCE hInstance, const char* lpCmdLine) {
    if (m_initialized) {
        return true;
    }
    
    // 创建FPS监控器
    m_fpsMonitor = std::make_unique<FPSMonitor>();
    m_fpsMonitor->Initialize();
    
    // 创建所有依赖对象（使用成员变量管理生命周期，符合依赖注入原则）
    m_configManager = std::make_unique<ConfigManager>();
    m_logger = std::make_unique<Logger>();
    m_eventBus = std::make_unique<EventBus>();
    m_windowFactory = std::make_unique<WindowFactory>();
    m_textRendererFactory = std::make_unique<TextRendererFactory>();
    
    // 使用初始化器管理所有组件的初始化
    m_initializer = std::make_unique<AppInitializer>();
    
    // 创建初始化配置对象，封装所有参数
    AppInitializationConfig config;
    config.rendererFactory = rendererFactory;
    config.hInstance = hInstance;
    config.lpCmdLine = lpCmdLine;
    config.configProvider = m_configManager.get();
    config.logger = m_logger.get();
    config.eventBus = m_eventBus.get();
    config.windowFactory = m_windowFactory.get();
    config.textRendererFactory = m_textRendererFactory.get();
    
    if (!m_initializer->Initialize(config)) {
        m_initializer.reset();
        m_fpsMonitor.reset();
        m_configManager.reset();
        m_logger.reset();
        m_eventBus.reset();
        m_windowFactory.reset();
        m_textRendererFactory.reset();
        return false;
    }
    
    m_initialized = true;
    return true;
}

void Application::Cleanup() {
    if (!m_initialized) {
        return;
    }
    
    // 委托给初始化器清理所有资源
    if (m_initializer) {
        m_initializer->Cleanup();
        m_initializer.reset();
    }
    
    // 清理FPS监控器
    m_fpsMonitor.reset();
    
    // 清理依赖对象（按逆序清理）
    m_textRendererFactory.reset();
    m_windowFactory.reset();
    m_eventBus.reset();
    m_logger.reset();
    m_configManager.reset();
    
    m_initialized = false;
}

int Application::Run() {
    if (!m_initialized || !m_initializer || !m_fpsMonitor) {
        return 1;
    }
    
    auto* windowManager = m_initializer->GetWindowManager();
    auto* eventManager = m_initializer->GetEventManager();
    auto* renderScheduler = m_initializer->GetRenderScheduler();
    auto* configProvider = m_initializer->GetConfigProvider();
    
    if (!windowManager || !configProvider) {
        return 1;
    }
    
    // 主循环（固定时间步 + 可变渲染插值）
    while (windowManager->IsRunning()) {
        // 使用事件管理器统一处理所有消息
        if (eventManager && !eventManager->ProcessMessages(configProvider->GetStretchMode())) {
            // 收到退出消息
            break;
        }
        
        if (windowManager->IsRunning()) {
            // 由WindowManager处理窗口最小化
            if (windowManager->HandleMinimized()) {
                continue;  // 窗口最小化，跳过渲染
            }
            
            // 更新FPS监控器（获取可变帧时间）
            m_fpsMonitor->Update();
            float deltaTime = m_fpsMonitor->GetDeltaTime();
            float time = m_fpsMonitor->GetTotalTime();
            float fps = m_fpsMonitor->GetFPS();
            
            // 设置启动时间
            if (!m_startTimeSet) {
                m_startTime = time;
                m_startTimeSet = true;
            }
            
            // 固定时间步更新逻辑
            m_accumulator += deltaTime;
            
            // 执行固定时间步更新（可能多次）
            while (m_accumulator >= FIXED_DELTA_TIME) {
                // 这里可以调用逻辑更新方法（如物理更新、游戏逻辑等）
                // UpdateLogic(FIXED_DELTA_TIME);
                
                m_accumulator -= FIXED_DELTA_TIME;
            }
            
            // 计算插值因子（用于渲染插值）
            m_alpha = m_accumulator / FIXED_DELTA_TIME;
            
            // 可变时间步渲染（带插值因子）
            RenderFrame(time, deltaTime, fps);
            
            // 控制帧率
            Sleep(1);
        }
    }
    
    return 0;
}

void Application::RenderFrame(float time, float deltaTime, float fps) {
    // 委托给渲染调度器处理
    auto* renderScheduler = m_initializer->GetRenderScheduler();
    if (renderScheduler) {
        float fpsValue = fps;  // 创建局部变量以传递引用
        renderScheduler->RenderFrame(time, deltaTime, fpsValue);
        // 注意：fpsValue的修改不会影响调用者，因为FPSMonitor已经管理了FPS计算
    }
}


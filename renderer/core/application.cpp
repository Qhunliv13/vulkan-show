#include "core/application.h"
#include "core/app_initializer.h"
#include "core/window_manager.h"
#include "core/event_manager.h"
#include "core/render_scheduler.h"
#include "core/irenderer_factory.h"
#include "core/fps_monitor.h"
#include "core/iconfig_provider.h"
#include "window/window.h"
#include <windows.h>

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
    
    // 使用初始化器管理所有组件的初始化
    m_initializer = std::make_unique<AppInitializer>();
    
    if (!m_initializer->Initialize(rendererFactory, hInstance, lpCmdLine)) {
        m_initializer.reset();
        m_fpsMonitor.reset();
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
    
    // 主循环
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
            
            // 更新FPS监控器
            m_fpsMonitor->Update();
            float deltaTime = m_fpsMonitor->GetDeltaTime();
            float time = m_fpsMonitor->GetTotalTime();
            float fps = m_fpsMonitor->GetFPS();
            
            // 设置启动时间
            if (!m_startTimeSet) {
                m_startTime = time;
                m_startTimeSet = true;
            }
            
            // 渲染帧
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


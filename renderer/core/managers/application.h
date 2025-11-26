#pragma once

#include <windows.h>
#include <memory>

// 前向声明
class IRendererFactory;
class AppInitializer;
class WindowManager;
class EventManager;
class RenderScheduler;
class Window;
class FPSMonitor;
class IConfigProvider;

// 应用类 - 管理整个应用的生命周期（职责简化，使用初始化器模式）
class Application {
public:
    Application();
    ~Application();
    
    // 初始化应用（使用工厂模式创建渲染器）
    bool Initialize(IRendererFactory* rendererFactory, HINSTANCE hInstance, const char* lpCmdLine);
    
    // 运行主循环
    int Run();
    
    // 清理资源
    void Cleanup();

private:
    // 渲染帧
    void RenderFrame(float time, float deltaTime, float fps);
    
    // 初始化器（管理所有组件的初始化）
    std::unique_ptr<AppInitializer> m_initializer;
    
    // FPS监控器
    std::unique_ptr<FPSMonitor> m_fpsMonitor;
    
    // 时间相关
    float m_startTime = 0.0f;
    bool m_startTimeSet = false;
    
    bool m_initialized = false;
};


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
class ConfigManager;
class Logger;
class EventBus;
class WindowFactory;
class TextRendererFactory;

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
    
    // 依赖对象（拥有所有权，确保生命周期足够长）
    std::unique_ptr<ConfigManager> m_configManager;
    std::unique_ptr<Logger> m_logger;
    std::unique_ptr<EventBus> m_eventBus;
    std::unique_ptr<WindowFactory> m_windowFactory;
    std::unique_ptr<TextRendererFactory> m_textRendererFactory;
    
    // 时间相关
    float m_startTime = 0.0f;
    bool m_startTimeSet = false;
    
    // 固定时间步相关
    static constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;  // 固定时间步（60Hz）
    float m_accumulator = 0.0f;  // 时间累积器
    float m_alpha = 0.0f;  // 插值因子（0.0 - 1.0）
    
    bool m_initialized = false;
};


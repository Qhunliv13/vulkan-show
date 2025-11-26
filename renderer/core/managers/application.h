#pragma once

#include <windows.h>  // 2. 系统头文件
#include <memory>  // 2. 系统头文件

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

/**
 * 应用类 - 管理整个应用的生命周期
 * 
 * 职责：管理应用的主循环、时间管理和资源生命周期
 * 设计：使用初始化器模式简化职责，通过 AppInitializer 管理组件初始化
 * 时间管理：支持固定时间步逻辑更新和可变时间步渲染插值
 * 
 * 使用方式：
 * 1. 创建 Application 实例
 * 2. 调用 Initialize() 初始化所有组件
 * 3. 调用 Run() 运行主循环
 * 4. 调用 Cleanup() 清理资源（析构函数自动调用）
 */
class Application {
public:
    Application();
    ~Application();
    
    /**
     * 初始化应用（使用工厂模式创建渲染器）
     * 
     * 通过依赖注入传入渲染器工厂，支持可替换的渲染器实现
     * 内部创建所有依赖对象（ConfigManager、Logger、EventBus等）并管理生命周期
     * 
     * @param rendererFactory 渲染器工厂（不拥有所有权，由外部管理生命周期）
     * @param hInstance Windows 实例句柄
     * @param lpCmdLine 命令行参数
     * @return true 如果初始化成功，false 如果失败
     */
    bool Initialize(IRendererFactory* rendererFactory, HINSTANCE hInstance, const char* lpCmdLine);
    
    /**
     * 运行主循环
     * 
     * 实现固定时间步逻辑更新和可变时间步渲染插值
     * 固定时间步：逻辑更新使用固定时间步（60Hz），确保逻辑稳定性
     * 可变时间步：渲染使用可变时间步，支持平滑插值
     * 
     * @return 退出代码（0表示正常退出）
     */
    int Run();
    
    /**
     * 清理资源
     * 
     * 按逆序清理所有资源，确保依赖关系正确
     */
    void Cleanup();

private:
    /**
     * 渲染一帧
     * 
     * 委托给渲染调度器处理，支持不同场景的渲染逻辑
     * 
     * @param time 总时间
     * @param deltaTime 帧时间
     * @param fps 帧率
     */
    void RenderFrame(float time, float deltaTime, float fps);
    
    // 初始化器（管理所有组件的初始化，拥有所有权）
    std::unique_ptr<AppInitializer> m_initializer;
    
    // FPS监控器（拥有所有权，用于时间管理和帧率计算）
    std::unique_ptr<FPSMonitor> m_fpsMonitor;
    
    // 依赖对象（拥有所有权，确保生命周期足够长）
    std::unique_ptr<ConfigManager> m_configManager;  // 配置管理器
    std::unique_ptr<Logger> m_logger;  // 日志器
    std::unique_ptr<EventBus> m_eventBus;  // 事件总线
    std::unique_ptr<WindowFactory> m_windowFactory;  // 窗口工厂
    std::unique_ptr<TextRendererFactory> m_textRendererFactory;  // 文字渲染器工厂
    
    // 时间相关
    float m_startTime = 0.0f;  // 应用启动时间
    bool m_startTimeSet = false;  // 启动时间是否已设置
    
    // 固定时间步相关（用于逻辑更新）
    static constexpr float FIXED_DELTA_TIME = 1.0f / 60.0f;  // 固定时间步（60Hz）
    float m_accumulator = 0.0f;  // 时间累积器，用于处理可变帧时间
    float m_alpha = 0.0f;  // 插值因子（0.0 - 1.0），用于渲染插值
    
    bool m_initialized = false;  // 初始化状态标志，防止重复初始化
};


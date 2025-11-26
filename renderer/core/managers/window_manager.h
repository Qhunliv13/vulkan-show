#pragma once

#include <windows.h>
#include <memory>

// 前向声明
class Window;
class IConfigProvider;
class IWindowFactory;
class IEventBus;
class ILogger;

// 窗口管理器 - 负责窗口的创建、管理和生命周期
class WindowManager {
public:
    WindowManager();
    ~WindowManager();
    
    // 初始化窗口（使用依赖注入：配置提供者、窗口工厂、事件总线、日志器）
    bool Initialize(HINSTANCE hInstance, IConfigProvider* configProvider, 
                    IWindowFactory* windowFactory, IEventBus* eventBus, ILogger* logger);
    
    // 获取窗口实例
    Window* GetWindow() const { return m_window.get(); }
    
    // 检查窗口是否运行
    bool IsRunning() const;
    
    // 检查窗口是否最小化
    bool IsMinimized() const;
    
    // 处理窗口最小化（返回true表示应该跳过渲染）
    bool HandleMinimized();
    
    // 清理资源
    void Cleanup();

private:
    std::unique_ptr<Window> m_window;
    ILogger* m_logger = nullptr;  // 日志器（依赖注入，不拥有所有权）
    bool m_initialized = false;
};


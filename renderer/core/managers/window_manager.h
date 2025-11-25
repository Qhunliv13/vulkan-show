#pragma once

#include <windows.h>
#include <memory>

// 前向声明
class Window;
class IConfigProvider;

// 窗口管理器 - 负责窗口的创建、管理和生命周期
class WindowManager {
public:
    WindowManager();
    ~WindowManager();
    
    // 初始化窗口（支持依赖注入配置提供者，如果为nullptr则使用单例）
    bool Initialize(HINSTANCE hInstance, IConfigProvider* configProvider = nullptr);
    
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
    bool m_initialized = false;
};


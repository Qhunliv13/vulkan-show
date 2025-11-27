#pragma once

#include <memory>  // 2. 系统头文件
#include <windows.h>  // 2. 系统头文件

// 前向声明
class Window;
class IConfigProvider;
class IWindowFactory;
class IEventBus;
class ILogger;

/**
 * 窗口管理器 - 负责窗口的创建、管理和生命周期
 * 
 * 职责：管理窗口的创建、初始化和生命周期
 * 设计：通过依赖注入传入所有依赖，使用工厂模式创建窗口
 * 
 * 使用方式：
 * 1. 通过依赖注入传入所有依赖（IConfigProvider、IWindowFactory、IEventBus、ILogger）
 * 2. 调用 Initialize() 创建和初始化窗口
 * 3. 使用 GetWindow() 获取窗口实例
 * 4. 调用 Cleanup() 清理资源
 */
class WindowManager {
public:
    WindowManager();
    ~WindowManager();
    
    /**
     * 初始化窗口（使用依赖注入）
     * 
     * 通过工厂模式创建窗口，设置事件总线，配置窗口属性
     * 
     * @param hInstance Windows 实例句柄
     * @param configProvider 配置提供者（不拥有所有权，用于获取窗口配置）
     * @param windowFactory 窗口工厂（不拥有所有权，用于创建窗口）
     * @param eventBus 事件总线（不拥有所有权，用于设置到窗口）
     * @param logger 日志器（不拥有所有权，用于日志输出）
     * @return true 如果初始化成功，false 如果失败
     */
    bool Initialize(HINSTANCE hInstance, IConfigProvider* configProvider, 
                    IWindowFactory* windowFactory, IEventBus* eventBus, ILogger* logger);
    
    /**
     * 获取窗口实例
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由 WindowManager 管理生命周期
     * 
     * @return Window* 窗口指针，可能为 nullptr
     */
    Window* GetWindow() const { return m_window.get(); }
    
    /**
     * 检查窗口是否运行
     * 
     * @return true 如果窗口正在运行，false 如果窗口已关闭
     */
    bool IsRunning() const;
    
    /**
     * 检查窗口是否最小化
     * 
     * @return true 如果窗口最小化，false 如果窗口正常显示
     */
    bool IsMinimized() const;
    
    /**
     * 处理窗口最小化
     * 
     * 如果窗口最小化，休眠以减少 CPU 占用
     * 
     * @return true 如果窗口最小化（应该跳过渲染），false 如果窗口正常显示
     */
    bool HandleMinimized();
    
    /**
     * 清理资源
     * 
     * 销毁窗口并清理所有资源
     */
    void Cleanup();

private:
    std::unique_ptr<Window> m_window;  // 窗口实例（拥有所有权）
    ILogger* m_logger = nullptr;  // 日志器（不拥有所有权，由外部管理生命周期）
    bool m_initialized = false;  // 初始化状态标志，防止重复初始化
};


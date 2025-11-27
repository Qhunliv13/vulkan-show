#pragma once

#define NOMINMAX  // 禁用Windows.h中的min/max宏，避免与Gdiplus冲突
#include <string>     // 2. 系统头文件
#include <windows.h>  // 2. 系统头文件

#include "core/interfaces/iwindow.h"  // 4. 项目头文件（接口）

// 前向声明
class IEventBus;

/**
 * 窗口管理器 - 管理Windows窗口的创建、消息处理和输入事件
 * 
 * 使用实例成员而非静态成员，支持依赖注入和多窗口实例
 * 通过事件总线处理输入事件，实现组件间解耦
 * 实现 IWindow 接口以支持窗口实现的替换和测试
 */
class Window : public IWindow {
public:
    Window();
    ~Window();
    
    /**
     * 创建窗口
     * @param hInstance 应用程序实例句柄
     * @param width 窗口宽度
     * @param height 窗口高度
     * @param title 窗口标题
     * @param className 窗口类名
     * @param fullscreen 是否全屏模式
     * @param iconPath 图标文件路径（可选）
     * @return 成功返回 true，失败返回 false
     */
    bool Create(HINSTANCE hInstance, int width, int height, const char* title, const char* className, bool fullscreen = true, const char* iconPath = nullptr) override;
    
    /**
     * 销毁窗口并清理资源
     */
    void Destroy() override;
    
    HWND GetHandle() const override { return m_hwnd; }
    HINSTANCE GetInstance() const override { return m_hInstance; }
    int GetWidth() const override { return m_width; }
    int GetHeight() const override { return m_height; }
    bool IsRunning() const override { return m_running; }
    void SetRunning(bool running) override { m_running = running; }
    bool IsFullscreen() const override { return m_fullscreen; }
    bool IsMinimized() const override;
    
    /**
     * 切换全屏模式
     * 接口保留用于支持窗口模式和全屏模式之间的切换
     */
    void ToggleFullscreen() override;
    
    /**
     * 处理窗口消息
     * 从消息队列中获取并分发窗口消息，应在主循环中定期调用
     */
    void ProcessMessages() override;
    
    /**
     * 设置窗口图标
     * @param iconPath 图标文件路径
     * @return 成功返回 true，失败返回 false
     */
    bool SetIcon(const std::string& iconPath) override;
    
    /**
     * 检查指定按键是否按下
     * @param keyCode 按键代码
     * @return 按键按下返回 true，否则返回 false
     */
    bool IsKeyPressed(int keyCode) const override;
    
    /**
     * 显示错误消息框
     * @param message 错误消息
     */
    static void ShowError(const std::string& message);
    
    /**
     * 设置事件总线（用于发布输入事件）
     * 通过依赖注入接收事件总线，实现组件间解耦
     * @param eventBus 事件总线接口指针（不拥有所有权）
     */
    void SetEventBus(IEventBus* eventBus) { m_eventBus = eventBus; }
    
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    HWND m_hwnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    int m_width = 0;
    int m_height = 0;
    bool m_running = true;
    bool m_fullscreen = false;
    int m_windowedWidth = 0;
    int m_windowedHeight = 0;
    int m_windowedX = 0;
    int m_windowedY = 0;
    DWORD m_windowedStyle = 0;
    const char* m_className = "VulkanShaderWindow";
    IEventBus* m_eventBus = nullptr;  // 事件总线接口（不拥有所有权，用于发布输入事件）
    int m_lastMouseX = 0;
    int m_lastMouseY = 0;
    bool m_leftButtonDown = false;
    bool m_keyStates[256] = {false};
};


#pragma once

#define NOMINMAX  // 禁用Windows.h中的min/max宏，避免与Gdiplus冲突
#include <string>     // 2. 系统头文件
#include <windows.h>  // 2. 系统头文件

#include "core/interfaces/iwindow.h"  // 4. 项目头文件（接口）

// 前向声明
class IEventBus;

// Window类 - 使用实例成员而非静态成员，支持依赖注入和多窗口
// 通过事件总线处理输入事件，而不是使用回调函数
// 实现 IWindow 接口以支持窗口实现的替换
class Window : public IWindow {
public:
    Window();
    ~Window();
    
    // IWindow 接口实现
    bool Create(HINSTANCE hInstance, int width, int height, const char* title, const char* className, bool fullscreen = true, const char* iconPath = nullptr) override;
    void Destroy() override;
    
    HWND GetHandle() const override { return m_hwnd; }
    HINSTANCE GetInstance() const override { return m_hInstance; }
    int GetWidth() const override { return m_width; }
    int GetHeight() const override { return m_height; }
    bool IsRunning() const override { return m_running; }
    void SetRunning(bool running) override { m_running = running; }
    bool IsFullscreen() const override { return m_fullscreen; }
    bool IsMinimized() const override;  // 检查窗口是否最小化
    
    void ToggleFullscreen() override;
    void ProcessMessages() override;
    bool SetIcon(const std::string& iconPath) override;  // 设置窗口图标
    
    bool IsKeyPressed(int keyCode) const override;  // 检查按键是否按下
    
    // IWindow 接口的静态方法实现
    static void ShowError(const std::string& message);
    
    // 设置事件总线（用于发布输入事件）- Window 特有方法
    void SetEventBus(IEventBus* eventBus) { m_eventBus = eventBus; }
    
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    // 实例成员（替代静态成员）
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
    IEventBus* m_eventBus = nullptr;  // 事件总线（用于发布输入事件）
    int m_lastMouseX = 0;  // 上次鼠标X坐标
    int m_lastMouseY = 0;  // 上次鼠标Y坐标
    bool m_leftButtonDown = false;  // 左键是否按下
    bool m_keyStates[256] = {false};  // 按键状态数组（支持256个按键）
};


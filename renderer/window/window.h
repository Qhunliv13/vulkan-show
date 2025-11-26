#pragma once

#include <windows.h>
#include <string>

// 前向声明
class IEventBus;

// Window类 - 使用实例成员而非静态成员，支持依赖注入和多窗口
// 通过事件总线处理输入事件，而不是使用回调函数
class Window {
public:
    Window();
    ~Window();
    
    // 创建窗口
    bool Create(HINSTANCE hInstance, int width, int height, const char* title, const char* className, bool fullscreen = true, const char* iconPath = nullptr);
    void Destroy();
    
    // 设置事件总线（用于发布输入事件）
    void SetEventBus(IEventBus* eventBus) { m_eventBus = eventBus; }
    
    // 获取窗口属性
    HWND GetHandle() const { return m_hwnd; }
    HINSTANCE GetInstance() const { return m_hInstance; }
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    bool IsRunning() const { return m_running; }
    void SetRunning(bool running) { m_running = running; }
    bool IsFullscreen() const { return m_fullscreen; }
    bool IsMinimized() const;  // 检查窗口是否最小化
    
    void ToggleFullscreen();
    static void ShowError(const std::string& message);  // 保持静态，因为不依赖实例
    void ProcessMessages();
    bool SetIcon(const std::string& iconPath);  // 设置窗口图标
    
    // 键盘输入
    bool IsKeyPressed(int keyCode) const;  // 检查按键是否按下
    
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


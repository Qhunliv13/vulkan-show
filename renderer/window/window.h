#pragma once

#include <windows.h>
#include <string>
#include <functional>

// Window类 - 使用实例成员而非静态成员，支持依赖注入和多窗口
class Window {
public:
    Window();
    ~Window();
    
    // 创建窗口
    bool Create(HINSTANCE hInstance, int width, int height, const char* title, const char* className, bool fullscreen = true, const char* iconPath = nullptr);
    void Destroy();
    
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
    
    // 鼠标输入回调
    void SetMouseMoveCallback(std::function<void(float, float, bool)> callback);  // 设置鼠标移动回调（deltaX, deltaY, leftButtonDown）
    
    // 键盘输入
    void SetKeyCallback(std::function<void(int, bool)> callback);  // 设置键盘回调（keyCode, isPressed）
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
    std::function<void(float, float, bool)> m_mouseMoveCallback = nullptr;  // 鼠标移动回调
    int m_lastMouseX = 0;  // 上次鼠标X坐标
    int m_lastMouseY = 0;  // 上次鼠标Y坐标
    bool m_leftButtonDown = false;  // 左键是否按下
    
    std::function<void(int, bool)> m_keyCallback = nullptr;  // 键盘回调
    bool m_keyStates[256] = {false};  // 按键状态数组（支持256个按键）
};


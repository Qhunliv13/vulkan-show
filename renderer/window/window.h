#pragma once

#include <windows.h>
#include <string>
#include <functional>

class Window {
public:
    static bool Create(HINSTANCE hInstance, int width, int height, const char* title, const char* className, bool fullscreen = true, const char* iconPath = nullptr);
    static void Destroy();
    
    static HWND GetHandle() { return s_hwnd; }
    static HINSTANCE GetInstance() { return s_hInstance; }
    static int GetWidth() { return s_width; }
    static int GetHeight() { return s_height; }
    static bool IsRunning() { return s_running; }
    static void SetRunning(bool running) { s_running = running; }
    static bool IsFullscreen() { return s_fullscreen; }
    static bool IsMinimized();  // 检查窗口是否最小化
    
    static void ToggleFullscreen();
    static void ShowError(const std::string& message);
    static void ProcessMessages();
    static bool SetIcon(const std::string& iconPath);  // 设置窗口图标
    
    // 鼠标输入回调
    static void SetMouseMoveCallback(std::function<void(float, float, bool)> callback);  // 设置鼠标移动回调（deltaX, deltaY, leftButtonDown）
    
    // 键盘输入
    static void SetKeyCallback(std::function<void(int, bool)> callback);  // 设置键盘回调（keyCode, isPressed）
    static bool IsKeyPressed(int keyCode);  // 检查按键是否按下
    
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    
    static HWND s_hwnd;
    static HINSTANCE s_hInstance;
    static int s_width;
    static int s_height;
    static bool s_running;
    static bool s_fullscreen;
    static int s_windowedWidth;
    static int s_windowedHeight;
    static int s_windowedX;
    static int s_windowedY;
    static DWORD s_windowedStyle;
    static const char* s_className;
    static std::function<void(float, float, bool)> s_mouseMoveCallback;  // 鼠标移动回调
    static int s_lastMouseX;  // 上次鼠标X坐标
    static int s_lastMouseY;  // 上次鼠标Y坐标
    static bool s_leftButtonDown;  // 左键是否按下
    
    static std::function<void(int, bool)> s_keyCallback;  // 键盘回调
    static bool s_keyStates[256];  // 按键状态数组（支持256个按键）
};


#pragma once

#include <string>
#include <cstdint>

// 前向声明（避免包含平台特定头文件）
struct HWND__;
typedef HWND__* HWND;
struct HINSTANCE__;
typedef HINSTANCE__* HINSTANCE;

// 窗口接口 - 抽象窗口操作，解耦组件与具体窗口实现
class IWindow {
public:
    virtual ~IWindow() = default;
    
    // 创建窗口
    virtual bool Create(HINSTANCE hInstance, int width, int height, 
                       const char* title, const char* className, 
                       bool fullscreen = true, const char* iconPath = nullptr) = 0;
    
    // 销毁窗口
    virtual void Destroy() = 0;
    
    // 获取窗口句柄（平台特定，但通过接口访问）
    virtual HWND GetHandle() const = 0;
    virtual HINSTANCE GetInstance() const = 0;
    
    // 获取窗口属性
    virtual int GetWidth() const = 0;
    virtual int GetHeight() const = 0;
    virtual bool IsRunning() const = 0;
    virtual void SetRunning(bool running) = 0;
    virtual bool IsFullscreen() const = 0;
    virtual bool IsMinimized() const = 0;
    
    // 窗口操作
    virtual void ToggleFullscreen() = 0;
    virtual void ProcessMessages() = 0;
    virtual bool SetIcon(const std::string& iconPath) = 0;
    
    // 键盘输入
    virtual bool IsKeyPressed(int keyCode) const = 0;
    
    // 静态错误显示（保持兼容性）
    static void ShowError(const std::string& message);
};


#pragma once

#include <windows.h>  // 2. 系统头文件
#include <string>     // 2. 系统头文件
#include <cstdint>    // 2. 系统头文件

/**
 * 窗口接口 - 抽象窗口操作，解耦组件与具体窗口实现
 * 
 * 职责：提供平台无关的窗口操作接口，隐藏平台特定实现细节
 * 设计：通过接口抽象，支持多种窗口实现（Windows、Linux、macOS等）
 * 
 * 使用方式：
 * 1. 通过工厂接口创建实现（如 WindowFactory）
 * 2. 使用接口指针操作，无需了解具体实现
 */
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


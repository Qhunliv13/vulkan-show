#pragma once

#include <memory>

// 前向声明
class Window;

// 窗口工厂接口 - 用于依赖注入，支持可替换的窗口实现
class IWindowFactory {
public:
    virtual ~IWindowFactory() = default;
    
    // 创建窗口实例（使用 CreateWindowInstance 避免与 Windows API 宏冲突）
    virtual std::unique_ptr<Window> CreateWindowInstance() = 0;
    
    // 销毁窗口实例（如果需要特殊清理逻辑）
    virtual void DestroyWindow(Window* window) = 0;
};


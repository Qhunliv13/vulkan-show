#pragma once

#include <memory>  // 2. 系统头文件

// 前向声明
class Window;

/**
 * 窗口工厂接口 - 用于依赖注入，支持可替换的窗口实现
 * 
 * 职责：提供窗口创建接口，支持可替换的窗口实现
 * 设计：通过工厂模式实现依赖倒置，解耦创建逻辑与使用逻辑
 * 
 * 使用方式：
 * 1. 实现此接口创建具体的窗口（如 WindowFactory）
 * 2. 通过依赖注入传入工厂实例
 * 3. 使用 CreateWindowInstance() 创建窗口实例
 */
class IWindowFactory {
public:
    virtual ~IWindowFactory() = default;
    
    // 创建窗口实例（使用 CreateWindowInstance 避免与 Windows API 宏冲突）
    virtual std::unique_ptr<Window> CreateWindowInstance() = 0;
    
    // 销毁窗口实例（如果需要特殊清理逻辑）
    virtual void DestroyWindow(Window* window) = 0;
};


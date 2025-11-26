#pragma once

#include "core/interfaces/iwindow_factory.h"  // 4. 项目头文件（接口）

/**
 * 默认窗口工厂实现
 * 
 * 实现 IWindowFactory 接口，提供标准的窗口创建功能
 * 使用 std::unique_ptr 管理窗口生命周期，确保自动内存管理
 */
class WindowFactory : public IWindowFactory {
public:
    WindowFactory() = default;
    ~WindowFactory() = default;
    
    /**
     * 创建窗口实例
     * 
     * 所有权：[TRANSFER] 调用方获得所有权，通过 std::unique_ptr 自动管理内存
     * @return std::unique_ptr<Window> 窗口实例，调用方获得所有权
     */
    std::unique_ptr<Window> CreateWindowInstance() override;
    
    /**
     * 销毁窗口实例
     * 
     * 注意：由于使用 std::unique_ptr 管理，此方法主要用于特殊清理逻辑
     * 正常情况下窗口会通过 unique_ptr 自动销毁
     * 
     * @param window 窗口指针（不拥有所有权，仅用于清理）
     */
    void DestroyWindow(Window* window) override;
};


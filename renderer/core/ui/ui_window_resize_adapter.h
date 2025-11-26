#pragma once

#include "core/interfaces/iwindow_resize_handler.h"  // 4. 项目头文件（接口）

// 前向声明
class UIManager;

/**
 * UI窗口大小变化处理适配器 - 实现 IWindowResizeHandler 接口，委托给 UIManager
 * 
 * 职责：将窗口大小变化处理接口与 UI 管理器解耦，实现接口职责单一原则
 * 设计：通过适配器模式，将 IWindowResizeHandler 接口的实现委托给 UIManager
 * 
 * 使用方式：
 * 1. 创建适配器实例，传入 UIManager 指针
 * 2. 使用适配器作为 IWindowResizeHandler 接口传递给需要该接口的组件
 */
class UIWindowResizeAdapter : public IWindowResizeHandler {
public:
    /**
     * 构造函数
     * 
     * @param uiManager UI管理器（不拥有所有权，由外部管理生命周期）
     */
    explicit UIWindowResizeAdapter(UIManager* uiManager);
    
    /**
     * IWindowResizeHandler 接口实现
     * 
     * 委托给 UIManager 处理窗口大小变化
     * 
     * @param stretchMode 拉伸模式
     * @param renderer 渲染器（不拥有所有权，用于获取拉伸参数）
     */
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;

private:
    UIManager* m_uiManager = nullptr;  // UI管理器（不拥有所有权，由外部管理生命周期）
};


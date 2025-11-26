#pragma once

#include <memory>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件
#include "core/config/constants.h"  // 4. 项目头文件（配置）
#include "core/interfaces/iwindow_resize_handler.h"  // 4. 项目头文件（接口）
#include "ui/color_controller/color_controller.h"  // 4. 项目头文件（UI组件）

// 前向声明
class ITextRenderer;
class IRenderer;
class LoadingAnimation;
class IWindow;
class IRenderContext;

/**
 * 颜色UI管理器 - 负责管理所有颜色相关的UI组件
 * 
 * 职责：管理所有颜色控制器组件的创建、初始化和生命周期
 * 设计：实现 IWindowResizeHandler 接口，支持窗口大小变化时更新颜色控制器位置
 * 
 * 使用方式：
 * 1. 通过依赖注入传入所有依赖（IRenderer、IRenderContext、ITextRenderer、IWindow）
 * 2. 调用 Initialize() 初始化所有颜色控制器组件
 * 3. 使用 Get 方法获取颜色控制器组件
 * 4. 调用 Cleanup() 清理所有资源
 */
class ColorUIManager : public IWindowResizeHandler {
public:
    ColorUIManager();
    ~ColorUIManager();
    
    /**
     * 初始化颜色UI组件
     * 
     * 通过依赖注入传入所有依赖，创建和初始化所有颜色控制器组件
     * 
     * @param renderer 渲染器（不拥有所有权，用于创建颜色控制器）
     * @param renderContext 渲染上下文（不拥有所有权，用于创建颜色控制器）
     * @param textRenderer 文字渲染器（不拥有所有权，用于颜色控制器文字渲染）
     * @param window 窗口（不拥有所有权，用于获取窗口信息）
     * @param stretchMode 拉伸模式
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     * @param loadingAnim 加载动画（不拥有所有权，用于颜色控制器）
     * @return true 如果初始化成功，false 如果失败
     */
    bool Initialize(IRenderer* renderer,
                   const IRenderContext& renderContext,
                   ITextRenderer* textRenderer,
                   IWindow* window,
                   StretchMode stretchMode,
                   float screenWidth,
                   float screenHeight,
                   LoadingAnimation* loadingAnim);
    
    /**
     * 清理资源
     * 
     * 清理所有颜色控制器组件和资源
     */
    void Cleanup();
    
    /**
     * IWindowResizeHandler 接口实现
     * 
     * 处理窗口大小变化，更新所有颜色控制器的位置
     * 
     * @param stretchMode 拉伸模式
     * @param renderer 渲染器（不拥有所有权，用于获取拉伸参数）
     */
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;
    
    /**
     * 获取颜色控制器组件
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由 ColorUIManager 管理生命周期
     */
    ColorController* GetColorController() const { return m_colorController.get(); }
    const std::vector<std::unique_ptr<ColorController>>& GetBoxColorControllers() const { return m_boxColorControllers; }
    
    /**
     * 获取/设置按钮颜色
     */
    void GetButtonColor(float& r, float& g, float& b, float& a) const {
        r = m_buttonColorR; g = m_buttonColorG; b = m_buttonColorB; a = m_buttonColorA;
    }
    void SetButtonColor(float r, float g, float b, float a) {
        m_buttonColorR = r; m_buttonColorG = g; m_buttonColorB = b; m_buttonColorA = a;
    }
    
private:
    bool InitializeColorController(IRenderer* renderer, IRenderContext& renderContext,
                                   StretchMode stretchMode, float screenWidth, float screenHeight);
    bool InitializeBoxColorControllers(IRenderer* renderer, IRenderContext& renderContext,
                                      StretchMode stretchMode, float screenWidth, float screenHeight);
    
    void UpdateColorControllerPositions(float screenWidth, float screenHeight, StretchMode stretchMode, IRenderer* renderer);
    
    // 颜色控制器组件（拥有所有权）
    std::unique_ptr<ColorController> m_colorController;  // 主颜色控制器
    std::vector<std::unique_ptr<ColorController>> m_boxColorControllers;  // 盒子颜色控制器组
    
    // 初始化状态标志
    bool m_colorControllerInitialized = false;  // 主颜色控制器是否已初始化
    std::vector<bool> m_boxColorControllersInitialized;  // 盒子颜色控制器初始化状态
    
    // 按钮颜色
    float m_buttonColorR = 1.0f;  // 按钮红色分量
    float m_buttonColorG = 1.0f;  // 按钮绿色分量
    float m_buttonColorB = 1.0f;  // 按钮蓝色分量
    float m_buttonColorA = 1.0f;  // 按钮透明度分量
    
    // 依赖对象（不拥有所有权，由外部管理生命周期）
    LoadingAnimation* m_loadingAnim = nullptr;  // 加载动画
    IWindow* m_window = nullptr;  // 窗口
};


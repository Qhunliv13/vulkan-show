#pragma once

#include <memory>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件

#include "core/config/constants.h"  // 4. 项目头文件（配置）
#include "core/interfaces/irender_context.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iwindow_resize_handler.h"  // 4. 项目头文件（接口）
#include "ui/button/button.h"  // 4. 项目头文件（UI组件）
// 注意：必须包含完整定义，因为 std::unique_ptr<Button> 作为成员变量需要完整类型（析构函数需要知道如何删除）

// 前向声明
class ITextRenderer;
class IRenderer;
class IWindow;

/**
 * 按钮UI管理器 - 负责管理所有按钮组件
 * 
 * 职责：管理所有按钮组件的创建、初始化和生命周期
 * 设计：实现 IWindowResizeHandler 接口，支持窗口大小变化时更新按钮位置
 * 
 * 使用方式：
 * 1. 通过依赖注入传入所有依赖（IRenderContext、ITextRenderer、IWindow）
 * 2. 调用 Initialize() 初始化所有按钮组件
 * 3. 使用 Get 方法获取按钮组件
 * 4. 调用 Cleanup() 清理所有资源
 */
class ButtonUIManager : public IWindowResizeHandler {
public:
    ButtonUIManager();
    ~ButtonUIManager();
    
    /**
     * 初始化按钮组件
     * 
     * 通过依赖注入传入所有依赖，创建和初始化所有按钮组件
     * 
     * @param renderContext 渲染上下文（不拥有所有权，用于创建按钮）
     * @param textRenderer 文字渲染器（不拥有所有权，用于按钮文字渲染）
     * @param window 窗口（不拥有所有权，用于获取窗口信息）
     * @param stretchMode 拉伸模式
     * @param screenWidth 屏幕宽度
     * @param screenHeight 屏幕高度
     * @return true 如果初始化成功，false 如果失败
     */
    bool Initialize(const IRenderContext& renderContext, 
                   ITextRenderer* textRenderer,
                   IWindow* window,
                   StretchMode stretchMode,
                   float screenWidth, 
                   float screenHeight);
    
    /**
     * 清理资源
     * 
     * 清理所有按钮组件和资源
     */
    void Cleanup();
    
    /**
     * IWindowResizeHandler 接口实现
     * 
     * 处理窗口大小变化，更新所有按钮的位置
     * 
     * @param stretchMode 拉伸模式
     * @param renderer 渲染器（不拥有所有权，用于获取拉伸参数）
     */
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;
    
    /**
     * 获取按钮组件
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由 ButtonUIManager 管理生命周期
     */
    Button* GetEnterButton() const { return m_enterButton.get(); }
    Button* GetColorButton() const { return m_colorButton.get(); }
    Button* GetLeftButton() const { return m_leftButton.get(); }
    Button* GetColorAdjustButton() const { return m_colorAdjustButton.get(); }
    const std::vector<std::unique_ptr<Button>>& GetColorButtons() const { return m_colorButtons; }
    const std::vector<std::unique_ptr<Button>>& GetBoxColorButtons() const { return m_boxColorButtons; }
    
    /**
     * 获取所有按钮（用于渲染）
     * 
     * 收集所有按钮指针到向量中，用于批量渲染
     * 
     * @param buttons 输出参数，存储所有按钮指针
     */
    void GetAllButtons(std::vector<Button*>& buttons) const;
    
    /**
     * 设置按钮颜色
     * 
     * @param r 红色分量
     * @param g 绿色分量
     * @param b 蓝色分量
     * @param a 透明度分量
     */
    void SetButtonColor(float r, float g, float b, float a);
    
    /**
     * 获取按钮颜色
     * 
     * @param r 输出参数，红色分量
     * @param g 输出参数，绿色分量
     * @param b 输出参数，蓝色分量
     * @param a 输出参数，透明度分量
     */
    void GetButtonColor(float& r, float& g, float& b, float& a) const {
        r = m_buttonColorR; g = m_buttonColorG; b = m_buttonColorB; a = m_buttonColorA;
    }
    
private:
    bool InitializeEnterButton(IRenderContext& renderContext, StretchMode stretchMode);
    bool InitializeColorButton(IRenderContext& renderContext, StretchMode stretchMode);
    bool InitializeLeftButton(IRenderContext& renderContext, StretchMode stretchMode);
    bool InitializeColorButtons(IRenderContext& renderContext, StretchMode stretchMode, 
                               float screenWidth, float screenHeight);
    bool InitializeBoxColorButtons(IRenderContext& renderContext, StretchMode stretchMode,
                                  float screenWidth, float screenHeight);
    bool InitializeColorAdjustButton(IRenderContext& renderContext, StretchMode stretchMode);
    
    void UpdateButtonPositions(float screenWidth, float screenHeight, StretchMode stretchMode, IRenderer* renderer);
    
    // 按钮组件（拥有所有权）
    std::unique_ptr<Button> m_enterButton;  // 进入按钮
    std::unique_ptr<Button> m_colorButton;  // 颜色按钮
    std::unique_ptr<Button> m_leftButton;  // 左按钮
    std::unique_ptr<Button> m_colorAdjustButton;  // 颜色调整按钮
    std::vector<std::unique_ptr<Button>> m_colorButtons;  // 颜色按钮组
    std::vector<std::unique_ptr<Button>> m_boxColorButtons;  // 盒子颜色按钮组
    
    // 初始化状态标志
    bool m_textRendererInitialized = false;  // 文字渲染器是否已初始化
    std::vector<bool> m_colorButtonsInitialized;  // 颜色按钮初始化状态
    std::vector<bool> m_boxColorButtonsInitialized;  // 盒子颜色按钮初始化状态
    bool m_colorAdjustButtonInitialized = false;  // 颜色调整按钮是否已初始化
    
    // 按钮颜色
    float m_buttonColorR = 1.0f;  // 按钮红色分量
    float m_buttonColorG = 1.0f;  // 按钮绿色分量
    float m_buttonColorB = 1.0f;  // 按钮蓝色分量
    float m_buttonColorA = 1.0f;  // 按钮透明度分量
    
    // 依赖对象（不拥有所有权，由外部管理生命周期）
    ITextRenderer* m_textRenderer = nullptr;  // 文字渲染器
    IWindow* m_window = nullptr;  // 窗口
};


#pragma once

#include <memory>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件

#include "core/config/constants.h"  // 4. 项目头文件（配置）
#include "core/interfaces/ibutton.h"  // 4. 项目头文件（接口）
#include "core/interfaces/icolor_controller.h"  // 4. 项目头文件（接口）
#include "core/interfaces/irender_context.h"  // 4. 项目头文件（接口）
#include "core/interfaces/irenderer.h"  // 4. 项目头文件（接口）
#include "core/interfaces/islider.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iuimanager.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iwindow.h"  // 4. 项目头文件（接口）

// 前向声明
class ITextRenderer;
class IEventBus;
class Button;  // 具体实现类，用于std::unique_ptr
class Slider;  // 具体实现类，用于std::unique_ptr
class ColorController;  // 具体实现类，用于std::unique_ptr
class LoadingAnimation;
class ButtonUIManager;
class ColorUIManager;
class SliderUIManager;

/**
 * UI管理器 - 负责所有UI组件的生命周期管理
 * 
 * 职责：管理所有UI组件的创建、初始化和生命周期
 * 设计：只实现核心 IUIManager 接口，其他接口（IWindowResizeHandler、IUIRenderProvider）通过适配器类实现
 * 原则：保持接口职责单一，通过适配器模式实现接口隔离
 * 
 * 使用方式：
 * 1. 通过依赖注入传入所有依赖（IRenderer、ITextRenderer、IWindow）
 * 2. 调用 Initialize() 初始化所有UI组件
 * 3. 使用 Get 方法获取UI组件
 * 4. 调用 Cleanup() 清理所有资源
 */
class UIManager : public IUIManager {
public:
    UIManager();
    ~UIManager();
    
    /**
     * 初始化所有UI组件（使用接口而不是具体类）
     * 
     * 通过依赖注入传入所有依赖，创建和管理所有UI子管理器
     * 
     * @param renderer 渲染器（不拥有所有权，由外部管理生命周期）
     * @param textRenderer 文字渲染器（不拥有所有权，由外部管理生命周期）
     * @param window 窗口（不拥有所有权，由外部管理生命周期）
     * @param stretchMode 拉伸模式
     * @return true 如果初始化成功，false 如果失败
     */
    bool Initialize(IRenderer* renderer, 
                    ITextRenderer* textRenderer,
                    IWindow* window,
                    StretchMode stretchMode);
    
    /**
     * 清理所有UI组件
     * 
     * 按逆序清理所有子管理器和UI组件，确保资源正确释放
     */
    void Cleanup();
    
    /**
     * UI组件获取方法（供适配器使用）
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由 UIManager 管理生命周期
     */
    LoadingAnimation* GetLoadingAnimation() const { return m_loadingAnim.get(); }
    IButton* GetEnterButton() const;
    IButton* GetColorButton() const;
    IButton* GetLeftButton() const;
    ISlider* GetOrangeSlider() const;
    void GetAllButtons(std::vector<IButton*>& buttons) const;
    void GetAllSliders(std::vector<ISlider*>& sliders) const;
    
    /**
     * 其他UI组件获取方法
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由 UIManager 管理生命周期
     */
    IButton* GetColorAdjustButton() const;
    IColorController* GetColorController() const;
    
    const std::vector<std::unique_ptr<Button>>& GetColorButtons() const;
    const std::vector<std::unique_ptr<Button>>& GetBoxColorButtons() const;
    const std::vector<std::unique_ptr<ColorController>>& GetBoxColorControllers() const;
    
    /**
     * 获取/设置颜色按钮展开状态
     */
    bool AreColorButtonsExpanded() const { return m_colorButtonsExpanded; }
    void SetColorButtonsExpanded(bool expanded) { m_colorButtonsExpanded = expanded; }
    
    bool AreBoxColorButtonsExpanded() const { return m_boxColorButtonsExpanded; }
    void SetBoxColorButtonsExpanded(bool expanded) { m_boxColorButtonsExpanded = expanded; }
    
    /**
     * 获取/设置按钮颜色
     */
    void GetButtonColor(float& r, float& g, float& b, float& a) const;
    void SetButtonColor(float r, float g, float b, float a);
    
    // IUIManager 接口实现
    bool HandleClick(float x, float y) override;
    void HandleMouseMove(float x, float y) override;
    void HandleMouseUp() override;
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;
    
    /**
     * 设置UI组件的回调函数（使用事件总线解耦）
     * 
     * 通过事件总线发布事件，而不是直接调用具体类的方法
     * 实现组件间完全解耦
     * 
     * @param eventBus 事件总线（不拥有所有权，由外部管理生命周期）
     */
    void SetupCallbacks(IEventBus* eventBus);
    
    /**
     * 订阅事件总线的事件（统一通信模式）
     * 
     * 订阅UI相关事件，实现事件驱动的UI交互
     * 使用 SubscribeWithId 保存订阅ID，以便后续取消订阅
     * 
     * @param eventBus 事件总线（不拥有所有权，由外部管理生命周期）
     */
    void SubscribeToEvents(IEventBus* eventBus);
    
    /**
     * 取消所有事件订阅
     * 
     * 在 Cleanup() 之前调用，取消所有已订阅的事件
     * 避免在对象销毁后仍接收到事件导致悬空指针
     * 
     * @param eventBus 事件总线（不拥有所有权，由外部管理生命周期）
     */
    void UnsubscribeFromEvents(IEventBus* eventBus);

private:
    // 初始化加载动画
    bool InitializeLoadingAnimation(IRenderer* renderer, const IRenderContext& renderContext, 
                                    StretchMode stretchMode, float screenWidth, float screenHeight);
    
    // UI组件
    std::unique_ptr<LoadingAnimation> m_loadingAnim;  // 加载动画（拥有所有权）
    
    // 子管理器（职责分离，拥有所有权）
    std::unique_ptr<ButtonUIManager> m_buttonManager;  // 按钮管理器
    std::unique_ptr<ColorUIManager> m_colorManager;  // 颜色管理器
    std::unique_ptr<SliderUIManager> m_sliderManager;  // 滑块管理器
    
    // 状态变量
    bool m_colorButtonsExpanded = false;  // 颜色按钮是否展开
    bool m_boxColorButtonsExpanded = false;  // 盒子颜色按钮是否展开
    
    // 依赖对象（不拥有所有权，由外部管理生命周期）
    IRenderer* m_renderer = nullptr;  // 渲染器（使用接口而不是具体类）
    ITextRenderer* m_textRenderer = nullptr;  // 文字渲染器
    IWindow* m_window = nullptr;  // 窗口
    
    // 事件订阅ID（用于清理时取消订阅）
    size_t m_uiClickSubscriptionId = 0;  // UI点击事件订阅ID
    size_t m_mouseMoveUISubscriptionId = 0;  // 鼠标移动UI事件订阅ID
    size_t m_mouseUpSubscriptionId = 0;  // 鼠标释放事件订阅ID
    size_t m_windowResizeSubscriptionId = 0;  // 窗口大小变化事件订阅ID
};


#pragma once

#include <memory>
#include <vector>
#include "core/constants.h"
#include "core/render_context.h"
#include "core/irenderer.h"
#include "core/iwindow_resize_handler.h"

// 前向声明
class TextRenderer;
class Button;
class Slider;
class ColorController;
class LoadingAnimation;
class ButtonUIManager;
class ColorUIManager;
class SliderUIManager;
class Window;

// UI管理器 - 负责所有UI组件的生命周期管理（实现 IWindowResizeHandler 接口）
class UIManager : public IWindowResizeHandler {
public:
    UIManager();
    ~UIManager();
    
    // 初始化所有UI组件（使用接口而不是具体类）
    bool Initialize(IRenderer* renderer, 
                    TextRenderer* textRenderer,
                    Window* window,
                    StretchMode stretchMode);
    
    // 清理所有UI组件
    void Cleanup();
    
    // IWindowResizeHandler 接口实现
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;
    
    // 获取UI组件（用于渲染和事件处理）
    Button* GetEnterButton() const;
    Button* GetColorButton() const;
    Button* GetLeftButton() const;
    Button* GetColorAdjustButton() const;
    Slider* GetOrangeSlider() const;
    ColorController* GetColorController() const;
    
    const std::vector<std::unique_ptr<Button>>& GetColorButtons() const;
    const std::vector<std::unique_ptr<Button>>& GetBoxColorButtons() const;
    const std::vector<std::unique_ptr<ColorController>>& GetBoxColorControllers() const;
    
    LoadingAnimation* GetLoadingAnimation() const { return m_loadingAnim; }
    
    // 获取所有按钮和滑块（用于渲染）
    void GetAllButtons(std::vector<Button*>& buttons) const;
    void GetAllSliders(std::vector<Slider*>& sliders) const;
    
    // 获取颜色按钮展开状态
    bool AreColorButtonsExpanded() const { return m_colorButtonsExpanded; }
    void SetColorButtonsExpanded(bool expanded) { m_colorButtonsExpanded = expanded; }
    
    bool AreBoxColorButtonsExpanded() const { return m_boxColorButtonsExpanded; }
    void SetBoxColorButtonsExpanded(bool expanded) { m_boxColorButtonsExpanded = expanded; }
    
    // 获取/设置按钮颜色
    void GetButtonColor(float& r, float& g, float& b, float& a) const;
    void SetButtonColor(float r, float g, float b, float a);
    
    // 统一的事件处理接口（解耦 EventManager）
    bool HandleClick(float x, float y);
    void HandleMouseMove(float x, float y);
    void HandleMouseUp();
    
    // 设置UI组件的回调函数（将业务逻辑从 Application 移入）
    // 注意：通过依赖注入方式传入 SceneManager、IRenderer 和 IConfigProvider，避免循环依赖
    // UIManager 不持有这些对象的指针，只在回调中捕获，确保生命周期由 Application 管理
    void SetupCallbacks(class SceneManager* sceneManager, IRenderer* renderer, class IConfigProvider* configProvider);

private:
    // 初始化加载动画
    bool InitializeLoadingAnimation(IRenderer* renderer, const VulkanRenderContext& renderContext, 
                                    StretchMode stretchMode, float screenWidth, float screenHeight);
    
    // UI组件
    LoadingAnimation* m_loadingAnim = nullptr;
    
    // 子管理器（职责分离）
    std::unique_ptr<ButtonUIManager> m_buttonManager;
    std::unique_ptr<ColorUIManager> m_colorManager;
    std::unique_ptr<SliderUIManager> m_sliderManager;
    
    // 状态变量
    bool m_colorButtonsExpanded = false;
    bool m_boxColorButtonsExpanded = false;
    
    IRenderer* m_renderer = nullptr;  // 使用接口而不是具体类
    TextRenderer* m_textRenderer = nullptr;
    Window* m_window = nullptr;
};


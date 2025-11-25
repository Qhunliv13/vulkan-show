#pragma once

#include <memory>
#include <vector>
#include "core/config/constants.h"
#include "core/config/render_context.h"
#include "core/interfaces/irenderer.h"
#include "core/interfaces/iwindow_resize_handler.h"
#include "core/interfaces/iuirender_provider.h"
#include "core/interfaces/iuimanager.h"

// 前向声明
class ITextRenderer;
class Button;
class IEventBus;
class Slider;
class ColorController;
class LoadingAnimation;
class ButtonUIManager;
class ColorUIManager;
class SliderUIManager;
class Window;

// UI管理器 - 负责所有UI组件的生命周期管理（实现多个接口以支持不同使用场景）
class UIManager : public IWindowResizeHandler, public IUIRenderProvider, public IUIManager {
public:
    UIManager();
    ~UIManager();
    
    // 初始化所有UI组件（使用接口而不是具体类）
    bool Initialize(IRenderer* renderer, 
                    ITextRenderer* textRenderer,
                    Window* window,
                    StretchMode stretchMode);
    
    // 清理所有UI组件
    void Cleanup();
    
    // IWindowResizeHandler 接口实现
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;
    
    // IUIRenderProvider 接口实现
    LoadingAnimation* GetLoadingAnimation() const override { return m_loadingAnim; }
    Button* GetEnterButton() const override;
    Button* GetColorButton() const override;
    Button* GetLeftButton() const override;
    Slider* GetOrangeSlider() const override;
    void GetAllButtons(std::vector<Button*>& buttons) const override;
    void GetAllSliders(std::vector<Slider*>& sliders) const override;
    
    // 其他UI组件获取方法（非接口方法）
    Button* GetColorAdjustButton() const;
    ColorController* GetColorController() const;
    
    const std::vector<std::unique_ptr<Button>>& GetColorButtons() const;
    const std::vector<std::unique_ptr<Button>>& GetBoxColorButtons() const;
    const std::vector<std::unique_ptr<ColorController>>& GetBoxColorControllers() const;
    
    // 获取颜色按钮展开状态
    bool AreColorButtonsExpanded() const { return m_colorButtonsExpanded; }
    void SetColorButtonsExpanded(bool expanded) { m_colorButtonsExpanded = expanded; }
    
    bool AreBoxColorButtonsExpanded() const { return m_boxColorButtonsExpanded; }
    void SetBoxColorButtonsExpanded(bool expanded) { m_boxColorButtonsExpanded = expanded; }
    
    // 获取/设置按钮颜色
    void GetButtonColor(float& r, float& g, float& b, float& a) const;
    void SetButtonColor(float r, float g, float b, float a);
    
    // IUIManager 接口实现
    bool HandleClick(float x, float y) override;
    void HandleMouseMove(float x, float y) override;
    void HandleMouseUp() override;
    // HandleWindowResize 已在 IWindowResizeHandler 中实现
    
    // 设置UI组件的回调函数（使用事件总线解耦）
    // 通过事件总线发布事件，而不是直接调用具体类的方法
    void SetupCallbacks(class IEventBus* eventBus);

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
    ITextRenderer* m_textRenderer = nullptr;
    Window* m_window = nullptr;
};


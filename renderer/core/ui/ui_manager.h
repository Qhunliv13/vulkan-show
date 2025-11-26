#pragma once

#include <memory>
#include <vector>
#include "core/config/constants.h"
#include "core/config/render_context.h"
#include "core/interfaces/irenderer.h"
#include "core/interfaces/iuimanager.h"
#include "core/interfaces/iwindow.h"

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

// UI管理器 - 负责所有UI组件的生命周期管理（只实现核心 IUIManager 接口）
// 注意：其他接口（IWindowResizeHandler、IUIRenderProvider）通过适配器类实现，保持接口职责单一
class UIManager : public IUIManager {
public:
    UIManager();
    ~UIManager();
    
    // 初始化所有UI组件（使用接口而不是具体类）
    bool Initialize(IRenderer* renderer, 
                    ITextRenderer* textRenderer,
                    IWindow* window,
                    StretchMode stretchMode);
    
    // 清理所有UI组件
    void Cleanup();
    
    // UI组件获取方法（供适配器使用）
    LoadingAnimation* GetLoadingAnimation() const { return m_loadingAnim; }
    Button* GetEnterButton() const;
    Button* GetColorButton() const;
    Button* GetLeftButton() const;
    Slider* GetOrangeSlider() const;
    void GetAllButtons(std::vector<Button*>& buttons) const;
    void GetAllSliders(std::vector<Slider*>& sliders) const;
    
    // 其他UI组件获取方法
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
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;
    
    // 设置UI组件的回调函数（使用事件总线解耦）
    // 通过事件总线发布事件，而不是直接调用具体类的方法
    void SetupCallbacks(class IEventBus* eventBus);
    
    // 订阅事件总线的事件（统一通信模式）
    void SubscribeToEvents(class IEventBus* eventBus);

private:
    // 初始化加载动画
    bool InitializeLoadingAnimation(IRenderer* renderer, const IRenderContext& renderContext, 
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
    IWindow* m_window = nullptr;
    
    // 事件订阅ID（用于清理）
    size_t m_uiClickSubscriptionId = 0;
    size_t m_mouseMoveUISubscriptionId = 0;
    size_t m_mouseUpSubscriptionId = 0;
    size_t m_windowResizeSubscriptionId = 0;
};


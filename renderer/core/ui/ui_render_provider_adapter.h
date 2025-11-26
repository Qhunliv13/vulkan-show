#pragma once

#include "core/interfaces/iuirender_provider.h"

// 前向声明
class UIManager;

// UI渲染提供者适配器 - 实现 IUIRenderProvider 接口，委托给 UIManager
// 职责：将 UI 渲染提供者接口与 UI 管理器解耦，实现接口职责单一原则
class UIRenderProviderAdapter : public IUIRenderProvider {
public:
    explicit UIRenderProviderAdapter(UIManager* uiManager);
    
    // IUIRenderProvider 接口实现
    LoadingAnimation* GetLoadingAnimation() const override;
    Button* GetEnterButton() const override;
    Button* GetColorButton() const override;
    Button* GetLeftButton() const override;
    Slider* GetOrangeSlider() const override;
    void GetAllButtons(std::vector<Button*>& buttons) const override;
    void GetAllSliders(std::vector<Slider*>& sliders) const override;
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;

private:
    UIManager* m_uiManager = nullptr;
};


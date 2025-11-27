#include "core/ui/ui_render_provider_adapter.h"  // 1. 对应头文件

// 2. 系统头文件（本文件不需要）
// 3. 第三方库头文件（本文件不需要）

#include "core/interfaces/ibutton.h"  // 4. 项目头文件（接口）
#include "core/interfaces/islider.h"  // 4. 项目头文件（接口）
#include "core/ui/ui_manager.h"  // 4. 项目头文件（UI管理器）
#include "ui/button/button.h"  // 4. 项目头文件（UI组件）
#include "ui/slider/slider.h"  // 4. 项目头文件（UI组件）

UIRenderProviderAdapter::UIRenderProviderAdapter(UIManager* uiManager)
    : m_uiManager(uiManager) {
}

LoadingAnimation* UIRenderProviderAdapter::GetLoadingAnimation() const {
    return m_uiManager ? m_uiManager->GetLoadingAnimation() : nullptr;
}

Button* UIRenderProviderAdapter::GetEnterButton() const {
    IButton* button = m_uiManager ? m_uiManager->GetEnterButton() : nullptr;
    return button ? static_cast<Button*>(button) : nullptr;
}

Button* UIRenderProviderAdapter::GetColorButton() const {
    IButton* button = m_uiManager ? m_uiManager->GetColorButton() : nullptr;
    return button ? static_cast<Button*>(button) : nullptr;
}

Button* UIRenderProviderAdapter::GetLeftButton() const {
    IButton* button = m_uiManager ? m_uiManager->GetLeftButton() : nullptr;
    return button ? static_cast<Button*>(button) : nullptr;
}

Slider* UIRenderProviderAdapter::GetOrangeSlider() const {
    ISlider* slider = m_uiManager ? m_uiManager->GetOrangeSlider() : nullptr;
    return slider ? static_cast<Slider*>(slider) : nullptr;
}

void UIRenderProviderAdapter::GetAllButtons(std::vector<Button*>& buttons) const {
    buttons.clear();
    if (m_uiManager) {
        std::vector<IButton*> interfaceButtons;
        m_uiManager->GetAllButtons(interfaceButtons);
        for (IButton* btn : interfaceButtons) {
            if (btn) {
                buttons.push_back(static_cast<Button*>(btn));
            }
        }
    }
}

void UIRenderProviderAdapter::GetAllSliders(std::vector<Slider*>& sliders) const {
    sliders.clear();
    if (m_uiManager) {
        std::vector<ISlider*> interfaceSliders;
        m_uiManager->GetAllSliders(interfaceSliders);
        for (ISlider* slider : interfaceSliders) {
            if (slider) {
                sliders.push_back(static_cast<Slider*>(slider));
            }
        }
    }
}

void UIRenderProviderAdapter::HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) {
    if (m_uiManager) {
        m_uiManager->HandleWindowResize(stretchMode, renderer);
    }
}


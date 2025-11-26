#include "core/ui/ui_render_provider_adapter.h"
#include "core/ui/ui_manager.h"

UIRenderProviderAdapter::UIRenderProviderAdapter(UIManager* uiManager)
    : m_uiManager(uiManager) {
}

LoadingAnimation* UIRenderProviderAdapter::GetLoadingAnimation() const {
    return m_uiManager ? m_uiManager->GetLoadingAnimation() : nullptr;
}

Button* UIRenderProviderAdapter::GetEnterButton() const {
    return m_uiManager ? m_uiManager->GetEnterButton() : nullptr;
}

Button* UIRenderProviderAdapter::GetColorButton() const {
    return m_uiManager ? m_uiManager->GetColorButton() : nullptr;
}

Button* UIRenderProviderAdapter::GetLeftButton() const {
    return m_uiManager ? m_uiManager->GetLeftButton() : nullptr;
}

Slider* UIRenderProviderAdapter::GetOrangeSlider() const {
    return m_uiManager ? m_uiManager->GetOrangeSlider() : nullptr;
}

void UIRenderProviderAdapter::GetAllButtons(std::vector<Button*>& buttons) const {
    if (m_uiManager) {
        m_uiManager->GetAllButtons(buttons);
    }
}

void UIRenderProviderAdapter::GetAllSliders(std::vector<Slider*>& sliders) const {
    if (m_uiManager) {
        m_uiManager->GetAllSliders(sliders);
    }
}

void UIRenderProviderAdapter::HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) {
    if (m_uiManager) {
        m_uiManager->HandleWindowResize(stretchMode, renderer);
    }
}


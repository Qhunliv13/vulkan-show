#include "core/ui/ui_manager.h"  // 1. 对应头文件

// 2. 系统头文件（本文件不需要）
// 3. 第三方库头文件（本文件不需要）

#include "core/ui/button_ui_manager.h"  // 4. 项目头文件（UI管理器）
#include "core/ui/color_ui_manager.h"  // 4. 项目头文件（UI管理器）
#include "core/ui/slider_ui_manager.h"  // 4. 项目头文件（UI管理器）

IButton* UIManager::GetEnterButton() const {
    return m_buttonManager ? m_buttonManager->GetEnterButton() : nullptr;
}

IButton* UIManager::GetColorButton() const {
    return m_buttonManager ? m_buttonManager->GetColorButton() : nullptr;
}

IButton* UIManager::GetLeftButton() const {
    return m_buttonManager ? m_buttonManager->GetLeftButton() : nullptr;
}

IButton* UIManager::GetColorAdjustButton() const {
    return m_buttonManager ? m_buttonManager->GetColorAdjustButton() : nullptr;
}

ISlider* UIManager::GetOrangeSlider() const {
    return m_sliderManager ? m_sliderManager->GetOrangeSlider() : nullptr;
}

IColorController* UIManager::GetColorController() const {
    return m_colorManager ? m_colorManager->GetColorController() : nullptr;
}

const std::vector<std::unique_ptr<Button>>& UIManager::GetColorButtons() const {
    static std::vector<std::unique_ptr<Button>> empty;
    return m_buttonManager ? m_buttonManager->GetColorButtons() : empty;
}

const std::vector<std::unique_ptr<Button>>& UIManager::GetBoxColorButtons() const {
    static std::vector<std::unique_ptr<Button>> empty;
    return m_buttonManager ? m_buttonManager->GetBoxColorButtons() : empty;
}

const std::vector<std::unique_ptr<ColorController>>& UIManager::GetBoxColorControllers() const {
    static std::vector<std::unique_ptr<ColorController>> empty;
    return m_colorManager ? m_colorManager->GetBoxColorControllers() : empty;
}

void UIManager::GetButtonColor(float& r, float& g, float& b, float& a) const {
    if (m_colorManager) {
        m_colorManager->GetButtonColor(r, g, b, a);
    } else {
        r = g = b = a = 1.0f;
    }
}

void UIManager::SetButtonColor(float r, float g, float b, float a) {
    if (m_buttonManager) {
        m_buttonManager->SetButtonColor(r, g, b, a);
    }
    if (m_colorManager) {
        m_colorManager->SetButtonColor(r, g, b, a);
    }
}


// UIManager getter方法实现
// 先包含子管理器的完整定义，然后包含 UIManager 的头文件
#include "core/ui/button_ui_manager.h"  // 4. 项目头文件（UI管理器）
#include "core/ui/color_ui_manager.h"  // 4. 项目头文件（UI管理器）
#include "core/ui/slider_ui_manager.h"  // 4. 项目头文件（UI管理器）
#include "core/ui/ui_manager.h"  // 4. 项目头文件（UI管理器）

Button* UIManager::GetEnterButton() const {
    return m_buttonManager ? m_buttonManager->GetEnterButton() : nullptr;
}

Button* UIManager::GetColorButton() const {
    return m_buttonManager ? m_buttonManager->GetColorButton() : nullptr;
}

Button* UIManager::GetLeftButton() const {
    return m_buttonManager ? m_buttonManager->GetLeftButton() : nullptr;
}

Button* UIManager::GetColorAdjustButton() const {
    return m_buttonManager ? m_buttonManager->GetColorAdjustButton() : nullptr;
}

Slider* UIManager::GetOrangeSlider() const {
    return m_sliderManager ? m_sliderManager->GetOrangeSlider() : nullptr;
}

ColorController* UIManager::GetColorController() const {
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


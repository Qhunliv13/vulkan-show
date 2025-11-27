#include "core/ui/button_ui_manager.h"  // 1. 对应头文件

#include <stdio.h>  // 2. 系统头文件
#include <windows.h>  // 2. 系统头文件

#include <vulkan/vulkan.h>  // 3. 第三方库头文件

#include "core/interfaces/irender_context.h"  // 4. 项目头文件（接口）
#include "core/interfaces/irenderer.h"  // 4. 项目头文件（接口）
#include "core/interfaces/itext_renderer.h"  // 4. 项目头文件（接口）
#include "core/types/render_types.h"  // 4. 项目头文件（类型）
#include "renderer/vulkan/vulkan_render_context_factory.h"  // 4. 项目头文件（工厂函数）
#include "text/text_renderer.h"  // 4. 项目头文件（文字渲染器）
#include "ui/button/button.h"  // 4. 项目头文件（UI组件）
#include "window/window.h"  // 4. 项目头文件（窗口）

/**
 * 辅助函数：将 ITextRenderer* 转换为 TextRenderer*（用于向后兼容）
 * 
 * 注意：此函数违反接口隔离原则（第15.1节），应修改 Button 组件使用 ITextRenderer 接口
 * 当前保留此函数的原因：Button 组件尚未支持接口，需要后续重构
 * 建议：修改 Button::Initialize() 方法接受 ITextRenderer* 参数，移除此转换函数
 */
// ToTextRenderer() 函数已移除 - Button 现在直接使用 ITextRenderer* 接口

ButtonUIManager::ButtonUIManager() {
}

ButtonUIManager::~ButtonUIManager() {
    Cleanup();
}

bool ButtonUIManager::Initialize(const IRenderContext& renderContext, 
                                 ITextRenderer* textRenderer,
                                 IWindow* window,
                                 StretchMode stretchMode,
                                 float screenWidth, 
                                 float screenHeight) {
    m_textRenderer = textRenderer;
    m_window = window;
    m_textRendererInitialized = (textRenderer != nullptr);
    
    // 预分配向量大小（9个颜色按钮和9个方块颜色按钮）
    m_colorButtons.resize(9);
    m_boxColorButtons.resize(9);
    m_colorButtonsInitialized.resize(9, false);
    m_boxColorButtonsInitialized.resize(9, false);
    
    // 创建非const IRenderContext 副本（Button组件需要非const引用）
    // 使用抽象类型创建渲染上下文（工厂函数内部进行 Vulkan 类型转换）
    Extent2D extent = renderContext.GetSwapchainExtent();
    std::unique_ptr<IRenderContext> nonConstContextPtr(CreateVulkanRenderContext(
        renderContext.GetDevice(),
        renderContext.GetPhysicalDevice(),
        renderContext.GetCommandPool(),
        renderContext.GetGraphicsQueue(),
        renderContext.GetRenderPass(),
        extent
    ));
    IRenderContext& nonConstContext = *nonConstContextPtr;
    
    if (!InitializeEnterButton(nonConstContext, stretchMode)) {
        return false;
    }
    if (!InitializeColorButton(nonConstContext, stretchMode)) {
        return false;
    }
    if (!InitializeLeftButton(nonConstContext, stretchMode)) {
        return false;
    }
    if (!InitializeColorButtons(nonConstContext, stretchMode, screenWidth, screenHeight)) {
        return false;
    }
    if (!InitializeBoxColorButtons(nonConstContext, stretchMode, screenWidth, screenHeight)) {
        return false;
    }
    if (!InitializeColorAdjustButton(nonConstContext, stretchMode)) {
        return false;
    }
    
    return true;
}

void ButtonUIManager::Cleanup() {
    if (m_enterButton) {
        m_enterButton->Cleanup();
        m_enterButton.reset();
    }
    
    if (m_colorButton) {
        m_colorButton->Cleanup();
        m_colorButton.reset();
    }
    
    if (m_leftButton) {
        m_leftButton->Cleanup();
        m_leftButton.reset();
    }
    
    if (m_colorAdjustButton) {
        m_colorAdjustButton->Cleanup();
        m_colorAdjustButton.reset();
    }
    
    for (auto& button : m_colorButtons) {
        if (button) {
            button->Cleanup();
        }
    }
    m_colorButtons.clear();
    
    for (auto& button : m_boxColorButtons) {
        if (button) {
            button->Cleanup();
        }
    }
    m_boxColorButtons.clear();
}

void ButtonUIManager::HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) {
    if (stretchMode == StretchMode::Scaled) {
        // Scaled模式：更新拉伸参数
        if (m_enterButton) {
            m_enterButton->SetStretchParams(renderer->GetStretchParams());
        }
        if (m_colorButton) {
            m_colorButton->SetStretchParams(renderer->GetStretchParams());
        }
        if (m_leftButton) {
            m_leftButton->SetStretchParams(renderer->GetStretchParams());
        }
        if (m_colorAdjustButtonInitialized && m_colorAdjustButton) {
            m_colorAdjustButton->SetStretchParams(renderer->GetStretchParams());
        }
        for (int i = 0; i < 9; i++) {
            if (m_colorButtonsInitialized[i] && m_colorButtons[i]) {
                m_colorButtons[i]->SetStretchParams(renderer->GetStretchParams());
            }
            if (m_boxColorButtonsInitialized[i] && m_boxColorButtons[i]) {
                m_boxColorButtons[i]->SetStretchParams(renderer->GetStretchParams());
            }
        }
    } else if (stretchMode != StretchMode::Fit && m_window) {
        // 非FIT/Scaled模式：更新UI位置
        RECT clientRect;
        GetClientRect(m_window->GetHandle(), &clientRect);
        float newScreenWidth = (float)(clientRect.right - clientRect.left);
        float newScreenHeight = (float)(clientRect.bottom - clientRect.top);
        
        UpdateButtonPositions(newScreenWidth, newScreenHeight, stretchMode, renderer);
    }
}

void ButtonUIManager::GetAllButtons(std::vector<IButton*>& buttons) const {
    buttons.clear();
    
    for (int i = 0; i < 9; i++) {
        if (m_colorButtonsInitialized[i] && m_colorButtons[i]) {
            buttons.push_back(m_colorButtons[i].get());
        }
    }
    for (int i = 0; i < 9; i++) {
        if (m_boxColorButtonsInitialized[i] && m_boxColorButtons[i]) {
            buttons.push_back(m_boxColorButtons[i].get());
        }
    }
    if (m_colorAdjustButtonInitialized && m_colorAdjustButton) {
        buttons.push_back(m_colorAdjustButton.get());
    }
    if (m_enterButton) {
        buttons.push_back(m_enterButton.get());
    }
    if (m_colorButton) {
        buttons.push_back(m_colorButton.get());
    }
    if (m_leftButton) {
        buttons.push_back(m_leftButton.get());
    }
}

void ButtonUIManager::SetButtonColor(float r, float g, float b, float a) {
    m_buttonColorR = r;
    m_buttonColorG = g;
    m_buttonColorB = b;
    m_buttonColorA = a;
}

bool ButtonUIManager::InitializeEnterButton(IRenderContext& renderContext, StretchMode stretchMode) {
    m_enterButton = std::make_unique<Button>();
    ButtonConfig buttonConfig = ButtonConfig::CreateRelativeWithText(
        0.5f, 0.75f, 300.0f, 50.0f,
        1.0f, 0.0f, 0.0f, 1.0f,
        "点击进入",
        1.0f, 1.0f, 1.0f, 1.0f);
    buttonConfig.zIndex = 25;
    buttonConfig.enableHoverEffect = true;
    buttonConfig.hoverEffectType = 0;
    buttonConfig.hoverEffectStrength = 0.3f;
    
    if (m_enterButton->Initialize(
            &renderContext,
            buttonConfig,
            m_textRenderer)) {
        if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
            m_enterButton->SetFixedScreenSize(true);
        }
        return true;
    }
    return false;
}

bool ButtonUIManager::InitializeColorButton(IRenderContext& renderContext, StretchMode stretchMode) {
    m_colorButton = std::make_unique<Button>();
    ButtonConfig colorButtonConfig = ButtonConfig::CreateRelative(0.75f, 0.5f, 80.0f, 40.0f, 0.0f, 0.0f, 1.0f, 1.0f);
    if (m_colorButton->Initialize(
            &renderContext,
            colorButtonConfig,
            m_textRenderer)) {
        if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
            m_colorButton->SetFixedScreenSize(true);
        }
        return true;
    }
    return false;
}

bool ButtonUIManager::InitializeLeftButton(IRenderContext& renderContext, StretchMode stretchMode) {
    m_leftButton = std::make_unique<Button>();
    ButtonConfig leftButtonConfig = ButtonConfig::CreateRelativeWithTexture(
        0.1f, 0.9f, 60.0f, 60.0f,
        "assets/shell.png");
    leftButtonConfig.zIndex = 0;
    leftButtonConfig.enableText = true;
    leftButtonConfig.text = "3D";
    leftButtonConfig.textColorR = 1.0f;
    leftButtonConfig.textColorG = 1.0f;
    leftButtonConfig.textColorB = 1.0f;
    leftButtonConfig.textColorA = 1.0f;
    leftButtonConfig.enableHoverEffect = true;
    leftButtonConfig.hoverEffectType = 0;
    leftButtonConfig.hoverEffectStrength = 0.3f;
    
    if (m_leftButton->Initialize(
            &renderContext,
            leftButtonConfig,
            m_textRenderer,
            false)) {
        if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
            m_leftButton->SetFixedScreenSize(true);
        }
        return true;
    } else {
        // 回退到颜色模式
        ButtonConfig fallbackConfig = ButtonConfig::CreateRelativeWithText(
            0.1f, 0.9f, 120.0f, 120.0f,
            0.2f, 0.6f, 1.0f, 1.0f,
            "3D",
            1.0f, 1.0f, 1.0f, 1.0f);
        if (m_leftButton->Initialize(
                &renderContext,
                fallbackConfig,
                m_textRenderer,
                false)) {
            if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
                m_leftButton->SetFixedScreenSize(true);
            }
            return true;
        }
    }
    return false;
}

bool ButtonUIManager::InitializeColorButtons(IRenderContext& renderContext, StretchMode stretchMode, 
                                           float screenWidth, float screenHeight) {
    Extent2D extent = renderContext.GetSwapchainExtent();
    
    struct ColorInfo {
        float r, g, b;
        const char* name;
    };
    ColorInfo colors[9] = {
        {1.0f, 0.0f, 0.0f, "红"},
        {0.0f, 1.0f, 0.0f, "绿"},
        {0.0f, 0.0f, 1.0f, "蓝"},
        {1.0f, 1.0f, 0.0f, "黄"},
        {1.0f, 0.0f, 1.0f, "紫"},
        {0.0f, 1.0f, 1.0f, "青"},
        {1.0f, 0.5f, 0.0f, "橙"},
        {1.0f, 1.0f, 1.0f, "白"},
        {0.0f, 0.0f, 0.0f, "黑"}
    };
    
    float buttonSize = 50.0f;
    float spacing = 10.0f;
    float centerX = 0.9f;
    float centerY = 0.1f;
    
    float colorBtnScreenWidth = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                 (float)extent.width : screenWidth;
    float colorBtnScreenHeight = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                  (float)extent.height : screenHeight;
    
    float matrixCenterX = centerX;
    float matrixCenterY = centerY + (80.0f + spacing + 80.0f) / colorBtnScreenHeight;
    
    float buttonSizeRel = buttonSize / colorBtnScreenWidth;
    float buttonSizeRelY = buttonSize / colorBtnScreenHeight;
    float spacingRelX = spacing / colorBtnScreenWidth;
    float spacingRelY = spacing / colorBtnScreenHeight;
    
    float matrixWidth = 3.0f * buttonSizeRel + 2.0f * spacingRelX;
    float matrixHeight = 3.0f * buttonSizeRelY + 2.0f * spacingRelY;
    
    float matrixStartX = matrixCenterX - matrixWidth / 2.0f;
    float matrixStartY = matrixCenterY - matrixHeight / 2.0f;
    
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int index = row * 3 + col;
            float relX = matrixStartX + col * (buttonSizeRel + spacingRelX);
            float relY = matrixStartY + row * (buttonSizeRelY + spacingRelY);
            
            m_colorButtons[index] = std::make_unique<Button>();
            ButtonConfig colorBtnConfig = ButtonConfig::CreateRelativeWithText(
                relX, relY, buttonSize, buttonSize,
                colors[index].r, colors[index].g, colors[index].b, 1.0f,
                colors[index].name,
                1.0f - colors[index].r, 1.0f - colors[index].g, 1.0f - colors[index].b, 1.0f);
            colorBtnConfig.zIndex = 15;
            colorBtnConfig.shapeType = 1;
            
            if (m_colorButtons[index]->Initialize(
                    &renderContext,
                    colorBtnConfig,
                    m_textRenderer)) {
                if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
                    m_colorButtons[index]->SetFixedScreenSize(true);
                }
                m_colorButtons[index]->SetVisible(false);
                m_colorButtonsInitialized[index] = true;
            }
        }
    }
    
    return true;
}

bool ButtonUIManager::InitializeBoxColorButtons(IRenderContext& renderContext, StretchMode stretchMode,
                                               float screenWidth, float screenHeight) {
    Extent2D extent = renderContext.GetSwapchainExtent();
    
    float boxBtnMatrixCenterX = 0.85f;
    float boxBtnMatrixCenterY = 0.5f;
    float boxBtnButtonSize = 40.0f;
    float boxBtnSpacing = 8.0f;
    
    float boxBtnButtonSizeRel = boxBtnButtonSize / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                     (float)extent.width : screenWidth);
    float boxBtnButtonSizeRelY = boxBtnButtonSize / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                      (float)extent.height : screenHeight);
    float boxBtnSpacingRelX = boxBtnSpacing / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                (float)extent.width : screenWidth);
    float boxBtnSpacingRelY = boxBtnSpacing / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                (float)extent.height : screenHeight);
    
    float boxBtnMatrixWidth = 3.0f * boxBtnButtonSizeRel + 2.0f * boxBtnSpacingRelX;
    float boxBtnMatrixHeight = 3.0f * boxBtnButtonSizeRelY + 2.0f * boxBtnSpacingRelY;
    
    float boxBtnMatrixStartX = boxBtnMatrixCenterX - boxBtnMatrixWidth / 2.0f;
    float boxBtnMatrixStartY = boxBtnMatrixCenterY - boxBtnMatrixHeight / 2.0f;
    
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int index = row * 3 + col;
            float relX = boxBtnMatrixStartX + col * (boxBtnButtonSizeRel + boxBtnSpacingRelX);
            float relY = boxBtnMatrixStartY + row * (boxBtnButtonSizeRelY + boxBtnSpacingRelY);
            
            char buttonText[4];
            sprintf_s(buttonText, "%d", index);
            
            m_boxColorButtons[index] = std::make_unique<Button>();
            ButtonConfig boxBtnConfig = ButtonConfig::CreateRelativeWithText(
                relX, relY, boxBtnButtonSize, boxBtnButtonSize,
                0.3f, 0.3f, 0.8f, 1.0f,
                buttonText,
                1.0f, 1.0f, 1.0f, 1.0f);
            boxBtnConfig.zIndex = 15;
            boxBtnConfig.shapeType = 0;
            
            if (m_boxColorButtons[index]->Initialize(
                    &renderContext,
                    boxBtnConfig,
                    m_textRenderer)) {
                if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
                    m_boxColorButtons[index]->SetFixedScreenSize(true);
                }
                m_boxColorButtons[index]->SetVisible(false);
                m_boxColorButtonsInitialized[index] = true;
            }
        }
    }
    
    return true;
}

bool ButtonUIManager::InitializeColorAdjustButton(IRenderContext& renderContext, StretchMode stretchMode) {
    m_colorAdjustButton = std::make_unique<Button>();
    ButtonConfig colorAdjustButtonConfig = ButtonConfig::CreateRelativeWithTexture(
        0.1f, 0.3f, 60.0f, 60.0f,
        "assets/test.png");
    colorAdjustButtonConfig.zIndex = 18;
    colorAdjustButtonConfig.enableText = false;
    
    if (m_colorAdjustButton->Initialize(
            &renderContext,
            colorAdjustButtonConfig,
            m_textRenderer,
            false)) {
        if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
            m_colorAdjustButton->SetFixedScreenSize(true);
        }
        m_colorAdjustButtonInitialized = true;
        printf("[DEBUG] Color adjust button initialized successfully\n");
        return true;
    }
    return false;
}

void ButtonUIManager::UpdateButtonPositions(float screenWidth, float screenHeight, StretchMode stretchMode, IRenderer* renderer) {
    // 更新所有按钮的位置（窗口大小变化时重新计算相对坐标）
    if (m_enterButton) {
        m_enterButton->UpdateForWindowResize(screenWidth, screenHeight);
    }
    if (m_colorButton) {
        m_colorButton->UpdateForWindowResize(screenWidth, screenHeight);
    }
    if (m_leftButton) {
        m_leftButton->UpdateForWindowResize(screenWidth, screenHeight);
    }
    
    // 更新颜色按钮和方块颜色按钮的位置
    for (int i = 0; i < 9; i++) {
        if (m_colorButtonsInitialized[i] && m_colorButtons[i]) {
            m_colorButtons[i]->UpdateForWindowResize(screenWidth, screenHeight);
        }
        if (m_boxColorButtonsInitialized[i] && m_boxColorButtons[i]) {
            m_boxColorButtons[i]->UpdateForWindowResize(screenWidth, screenHeight);
        }
    }
    if (m_colorAdjustButtonInitialized && m_colorAdjustButton) {
        m_colorAdjustButton->UpdateForWindowResize(screenWidth, screenHeight);
    }
}


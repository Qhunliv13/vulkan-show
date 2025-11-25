#include "core/ui/color_ui_manager.h"
#include "ui/color_controller/color_controller.h"
#include "text/text_renderer.h"
#include "core/interfaces/irenderer.h"
#include "loading/loading_animation.h"
#include "window/window.h"
#include <stdio.h>
#include <windows.h>

ColorUIManager::ColorUIManager() {
}

ColorUIManager::~ColorUIManager() {
    Cleanup();
}

bool ColorUIManager::Initialize(IRenderer* renderer,
                                const VulkanRenderContext& renderContext,
                                TextRenderer* textRenderer,
                                Window* window,
                                StretchMode stretchMode,
                                float screenWidth,
                                float screenHeight,
                                LoadingAnimation* loadingAnim) {
    m_loadingAnim = loadingAnim;
    m_window = window;
    
    // 初始化向量大小
    m_boxColorControllers.resize(9);
    m_boxColorControllersInitialized.resize(9, false);
    
    // 创建非const副本以传递给需要修改的方法
    VulkanRenderContext nonConstContext(
        renderContext.GetDevice(),
        renderContext.GetPhysicalDevice(),
        renderContext.GetCommandPool(),
        renderContext.GetGraphicsQueue(),
        renderContext.GetRenderPass(),
        renderContext.GetSwapchainExtent()
    );
    
    if (!InitializeColorController(renderer, nonConstContext, stretchMode, screenWidth, screenHeight)) {
        return false;
    }
    if (!InitializeBoxColorControllers(renderer, nonConstContext, stretchMode, screenWidth, screenHeight)) {
        return false;
    }
    
    return true;
}

void ColorUIManager::Cleanup() {
    if (m_colorController) {
        m_colorController->Cleanup();
        m_colorController.reset();
    }
    
    for (auto& controller : m_boxColorControllers) {
        if (controller) {
            controller->Cleanup();
        }
    }
    m_boxColorControllers.clear();
}

void ColorUIManager::HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) {
    if (stretchMode != StretchMode::Fit && stretchMode != StretchMode::Scaled && m_window) {
        // 非FIT/Scaled模式：更新UI位置
        RECT clientRect;
        GetClientRect(m_window->GetHandle(), &clientRect);
        float newScreenWidth = (float)(clientRect.right - clientRect.left);
        float newScreenHeight = (float)(clientRect.bottom - clientRect.top);
        
        UpdateColorControllerPositions(newScreenWidth, newScreenHeight, stretchMode, renderer);
    } else if (stretchMode == StretchMode::Scaled) {
        // Scaled模式：更新拉伸参数
        // ColorController 内部使用 Slider 和 Button，需要分别设置它们的拉伸参数
        if (m_colorControllerInitialized && m_colorController) {
            auto sliders = m_colorController->GetSliders();
            auto buttons = m_colorController->GetButtons();
            const auto& stretchParams = renderer->GetStretchParams();
            for (auto* slider : sliders) {
                if (slider) {
                    slider->SetStretchParams(stretchParams);
                }
            }
            for (auto* button : buttons) {
                if (button) {
                    button->SetStretchParams(stretchParams);
                }
            }
        }
        for (int i = 0; i < 9; i++) {
            if (m_boxColorControllersInitialized[i] && m_boxColorControllers[i]) {
                auto sliders = m_boxColorControllers[i]->GetSliders();
                auto buttons = m_boxColorControllers[i]->GetButtons();
                const auto& stretchParams = renderer->GetStretchParams();
                for (auto* slider : sliders) {
                    if (slider) {
                        slider->SetStretchParams(stretchParams);
                    }
                }
                for (auto* button : buttons) {
                    if (button) {
                        button->SetStretchParams(stretchParams);
                    }
                }
            }
        }
    }
}

bool ColorUIManager::InitializeColorController(IRenderer* renderer, VulkanRenderContext& renderContext,
                                               StretchMode stretchMode, float screenWidth, float screenHeight) {
    VkExtent2D uiExtent = renderContext.GetSwapchainExtent();
    
    m_colorController = std::make_unique<ColorController>();
    ColorControllerConfig colorControllerConfig;
    colorControllerConfig.relativeX = 0.1f;
    colorControllerConfig.relativeY = 0.3f + 80.0f / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                      (float)uiExtent.height : screenHeight);
    colorControllerConfig.sliderWidth = 200.0f;
    colorControllerConfig.sliderHeight = 6.0f;
    colorControllerConfig.sliderSpacing = 50.0f;
    colorControllerConfig.displayWidth = 100.0f;
    colorControllerConfig.displayHeight = 50.0f;
    colorControllerConfig.displayOffsetY = 30.0f;
    colorControllerConfig.initialR = m_buttonColorR;
    colorControllerConfig.initialG = m_buttonColorG;
    colorControllerConfig.initialB = m_buttonColorB;
    colorControllerConfig.initialA = m_buttonColorA;
    colorControllerConfig.zIndex = 19;
    colorControllerConfig.visible = false;
    colorControllerConfig.screenWidth = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                       (float)uiExtent.width : screenWidth;
    colorControllerConfig.screenHeight = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                        (float)uiExtent.height : screenHeight;
    
    if (m_colorController->Initialize(
            renderer->GetDevice(),
            renderer->GetPhysicalDevice(),
            renderer->GetCommandPool(),
            renderer->GetGraphicsQueue(),
            renderer->GetRenderPass(),
            uiExtent,
            colorControllerConfig,
            nullptr)) {  // TextRenderer will be set later if needed
        if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
            m_colorController->SetFixedScreenSize(true);
        }
        m_colorControllerInitialized = true;
        printf("[DEBUG] Color controller initialized successfully\n");
        
        // 设置颜色控制器的回调
        m_colorController->SetOnColorChangedCallback([this](float r, float g, float b, float a) {
            m_buttonColorR = r;
            m_buttonColorG = g;
            m_buttonColorB = b;
            m_buttonColorA = a;
            
            printf("[DEBUG] Color changed to (%.2f, %.2f, %.2f, %.2f)\n", 
                   m_buttonColorR, m_buttonColorG, m_buttonColorB, m_buttonColorA);
        });
        
        return true;
    }
    return false;
}

bool ColorUIManager::InitializeBoxColorControllers(IRenderer* renderer, VulkanRenderContext& renderContext,
                                                   StretchMode stretchMode, float screenWidth, float screenHeight) {
    VkExtent2D uiExtent = renderContext.GetSwapchainExtent();
    
    // 计算方块按钮矩阵的位置（与InitializeBoxColorButtons中的计算保持一致）
    float boxBtnMatrixCenterX = 0.85f;
    float boxBtnMatrixCenterY = 0.5f;
    float boxBtnButtonSize = 40.0f;
    float boxBtnSpacing = 8.0f;
    
    float boxBtnButtonSizeRel = boxBtnButtonSize / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                     (float)uiExtent.width : screenWidth);
    float boxBtnSpacingRelX = boxBtnSpacing / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                (float)uiExtent.width : screenWidth);
    float boxBtnMatrixWidth = 3.0f * boxBtnButtonSizeRel + 2.0f * boxBtnSpacingRelX;
    
    float controllerBaseX = boxBtnMatrixCenterX + boxBtnMatrixWidth / 2.0f + 20.0f / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                                                      (float)uiExtent.width : screenWidth);
    float controllerBaseY = boxBtnMatrixCenterY;
    
    for (int i = 0; i < 9; i++) {
        m_boxColorControllers[i] = std::make_unique<ColorController>();
        ColorControllerConfig boxControllerConfig;
        boxControllerConfig.relativeX = controllerBaseX;
        boxControllerConfig.relativeY = controllerBaseY;
        boxControllerConfig.sliderWidth = 80.0f;
        boxControllerConfig.sliderHeight = 2.4f;
        boxControllerConfig.sliderSpacing = 20.0f;
        boxControllerConfig.displayWidth = 40.0f;
        boxControllerConfig.displayHeight = 20.0f;
        boxControllerConfig.displayOffsetY = 12.0f;
        boxControllerConfig.initialR = 1.0f;
        boxControllerConfig.initialG = 1.0f;
        boxControllerConfig.initialB = 1.0f;
        boxControllerConfig.initialA = 1.0f;
        boxControllerConfig.zIndex = 30;
        boxControllerConfig.visible = false;
        boxControllerConfig.screenWidth = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                          (float)uiExtent.width : screenWidth;
        boxControllerConfig.screenHeight = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                           (float)uiExtent.height : screenHeight;
        
        if (m_boxColorControllers[i]->Initialize(
                renderer->GetDevice(),
                renderer->GetPhysicalDevice(),
                renderer->GetCommandPool(),
                renderer->GetGraphicsQueue(),
                renderer->GetRenderPass(),
                uiExtent,
                boxControllerConfig,
                nullptr)) {  // TextRenderer will be set later if needed
            if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
                m_boxColorControllers[i]->SetFixedScreenSize(true);
            }
            m_boxColorControllersInitialized[i] = true;
            
            int boxIndex = i;
            m_boxColorControllers[i]->SetOnColorChangedCallback([this, boxIndex](float r, float g, float b, float a) {
                printf("[DEBUG] Box %d color changed to (%.2f, %.2f, %.2f, %.2f)\n", 
                       boxIndex, r, g, b, a);
                if (m_loadingAnim) {
                    m_loadingAnim->SetBoxColor(boxIndex, r, g, b, a);
                }
            });
            
            printf("[DEBUG] Box color controller %d initialized successfully\n", i);
        }
    }
    
    return true;
}

void ColorUIManager::UpdateColorControllerPositions(float screenWidth, float screenHeight, StretchMode stretchMode, IRenderer* renderer) {
    if (m_colorControllerInitialized && m_colorController) {
        m_colorController->UpdateScreenSize(screenWidth, screenHeight);
    }
    for (int i = 0; i < 9; i++) {
        if (m_boxColorControllersInitialized[i] && m_boxColorControllers[i]) {
            m_boxColorControllers[i]->UpdateScreenSize(screenWidth, screenHeight);
        }
    }
}


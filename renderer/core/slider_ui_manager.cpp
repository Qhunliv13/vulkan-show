#include "core/slider_ui_manager.h"
#include "ui/slider/slider.h"
#include "ui/color_controller/color_controller.h"
#include "core/irenderer.h"
#include "window/window.h"
#include <stdio.h>
#include <windows.h>

SliderUIManager::SliderUIManager() {
}

SliderUIManager::~SliderUIManager() {
    Cleanup();
}

bool SliderUIManager::Initialize(const VulkanRenderContext& renderContext, Window* window, StretchMode stretchMode) {
    m_window = window;
    // 创建非const副本以传递给需要修改的方法
    VulkanRenderContext nonConstContext(
        renderContext.GetDevice(),
        renderContext.GetPhysicalDevice(),
        renderContext.GetCommandPool(),
        renderContext.GetGraphicsQueue(),
        renderContext.GetRenderPass(),
        renderContext.GetSwapchainExtent()
    );
    return InitializeOrangeSlider(nonConstContext, stretchMode);
}

void SliderUIManager::Cleanup() {
    if (m_orangeSlider) {
        m_orangeSlider->Cleanup();
        m_orangeSlider.reset();
    }
}

void SliderUIManager::HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) {
    if (stretchMode == StretchMode::Scaled) {
        // Scaled模式：更新拉伸参数
        if (m_sliderInitialized && m_orangeSlider) {
            m_orangeSlider->SetStretchParams(renderer->GetStretchParams());
        }
    } else if (stretchMode != StretchMode::Fit && m_window) {
        // 非FIT/Scaled模式：更新UI位置
        RECT clientRect;
        GetClientRect(m_window->GetHandle(), &clientRect);
        float newScreenWidth = (float)(clientRect.right - clientRect.left);
        float newScreenHeight = (float)(clientRect.bottom - clientRect.top);
        
        if (m_sliderInitialized && m_orangeSlider) {
            m_orangeSlider->UpdateForWindowResize(newScreenWidth, newScreenHeight);
        }
    }
}

void SliderUIManager::GetAllSliders(std::vector<Slider*>& sliders, 
                                   ColorController* colorController,
                                   const std::vector<std::unique_ptr<ColorController>>* boxColorControllers) const {
    sliders.clear();
    
    if (m_sliderInitialized && m_orangeSlider) {
        sliders.push_back(m_orangeSlider.get());
    }
    
    // 添加颜色控制器的滑块
    if (colorController && colorController->IsVisible()) {
        std::vector<Slider*> colorControllerSliders = colorController->GetSliders();
        for (Slider* slider : colorControllerSliders) {
            sliders.push_back(slider);
        }
    }
    
    // 添加方块颜色控制器的滑块
    if (boxColorControllers) {
        for (const auto& controller : *boxColorControllers) {
            if (controller && controller->IsVisible()) {
                std::vector<Slider*> boxControllerSliders = controller->GetSliders();
                for (Slider* slider : boxControllerSliders) {
                    sliders.push_back(slider);
                }
            }
        }
    }
}

bool SliderUIManager::InitializeOrangeSlider(VulkanRenderContext& renderContext, StretchMode stretchMode) {
    m_orangeSlider = std::make_unique<Slider>();
    SliderConfig sliderConfig(20.0f, 20.0f, 300.0f, 6.0f, 0.0f, 100.0f, 50.0f);
    sliderConfig.trackColorR = 0.3f;
    sliderConfig.trackColorG = 0.3f;
    sliderConfig.trackColorB = 0.3f;
    sliderConfig.fillColorR = 1.0f;
    sliderConfig.fillColorG = 0.5f;
    sliderConfig.fillColorB = 0.0f;
    sliderConfig.thumbColorR = 0.5f;
    sliderConfig.thumbColorG = 0.8f;
    sliderConfig.thumbColorB = 1.0f;
    sliderConfig.thumbWidth = 20.0f;
    sliderConfig.thumbHeight = 20.0f;
    sliderConfig.zIndex = 10;
    sliderConfig.useRelativePosition = false;
    
    if (m_orangeSlider->Initialize(
            &renderContext,
            sliderConfig,
            false)) {
        m_sliderInitialized = true;
        printf("[DEBUG] Orange slider initialized successfully\n");
        return true;
    }
    return false;
}


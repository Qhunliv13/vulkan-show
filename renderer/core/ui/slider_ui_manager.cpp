#include "core/ui/slider_ui_manager.h"  // 1. 对应头文件

#include <stdio.h>  // 2. 系统头文件
#include <windows.h>  // 2. 系统头文件

#include <vulkan/vulkan.h>  // 3. 第三方库头文件

#include "core/interfaces/irender_context.h"  // 4. 项目头文件（接口）
#include "core/interfaces/irenderer.h"  // 4. 项目头文件（接口）
#include "renderer/vulkan/vulkan_render_context_factory.h"  // 4. 项目头文件（工厂函数）
#include "ui/color_controller/color_controller.h"  // 4. 项目头文件（UI组件）
#include "ui/slider/slider.h"  // 4. 项目头文件（UI组件）
#include "window/window.h"  // 4. 项目头文件（窗口）

SliderUIManager::SliderUIManager() {
}

SliderUIManager::~SliderUIManager() {
    Cleanup();
}

bool SliderUIManager::Initialize(const IRenderContext& renderContext, IWindow* window, StretchMode stretchMode) {
    m_window = window;
    // 创建非const IRenderContext 副本（Slider组件需要非const引用）
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

void SliderUIManager::GetAllSliders(std::vector<ISlider*>& sliders, 
                                   IColorController* colorController,
                                   const std::vector<std::unique_ptr<ColorController>>* boxColorControllers) const {
    sliders.clear();
    
    // 添加橙色滑块
    if (m_sliderInitialized && m_orangeSlider) {
        sliders.push_back(m_orangeSlider.get());  // Slider继承ISlider，可以直接使用
    }
    
    // 收集颜色控制器中的滑块（颜色控制器内部包含RGBA四个滑块）
    if (colorController && colorController->IsVisible()) {
        // ColorController已实现IColorController接口，直接使用
        std::vector<ISlider*> colorControllerSliders = colorController->GetSliders();
        for (ISlider* slider : colorControllerSliders) {
            sliders.push_back(slider);
        }
    }
    
    // 收集方块颜色控制器中的滑块（每个方块有独立的颜色控制器）
    if (boxColorControllers) {
        for (const auto& controller : *boxColorControllers) {
            if (controller && controller->IsVisible()) {
                // ColorController已实现IColorController接口，直接使用
                std::vector<ISlider*> boxControllerSliders = controller->GetSliders();
                for (ISlider* slider : boxControllerSliders) {
                    sliders.push_back(slider);
                }
            }
        }
    }
}

bool SliderUIManager::InitializeOrangeSlider(IRenderContext& renderContext, StretchMode stretchMode) {
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


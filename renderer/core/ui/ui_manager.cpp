#include "core/ui/ui_manager.h"  // 1. 对应头文件

#include <windows.h>  // 2. 系统头文件
#include <stdio.h>  // 2. 系统头文件

#include <vulkan/vulkan.h>  // 3. 第三方库头文件

#include "core/interfaces/iconfig_provider.h"  // 4. 项目头文件（接口）
#include "core/interfaces/ievent_bus.h"  // 4. 项目头文件（接口）
#include "core/interfaces/irender_device.h"  // 4. 项目头文件（接口）
#include "core/config/render_context.h"  // 4. 项目头文件（配置）
#include "core/config/vulkan_render_context_factory.h"  // 4. 项目头文件（配置）
#include "core/managers/scene_manager.h"  // 4. 项目头文件（管理器）
#include "core/ui/button_ui_manager.h"  // 4. 项目头文件（UI管理器）
#include "core/ui/color_ui_manager.h"  // 4. 项目头文件（UI管理器）
#include "core/ui/slider_ui_manager.h"  // 4. 项目头文件（UI管理器）
#include "loading/loading_animation.h"  // 4. 项目头文件（加载动画）
#include "text/text_renderer.h"  // 4. 项目头文件（文字渲染器）
#include "ui/button/button.h"  // 4. 项目头文件（UI组件）
#include "ui/slider/slider.h"  // 4. 项目头文件（UI组件）
#include "ui/color_controller/color_controller.h"  // 4. 项目头文件（UI组件）
#include "window/window.h"  // 4. 项目头文件（窗口）

UIManager::UIManager() {
}

UIManager::~UIManager() {
    Cleanup();
}

bool UIManager::Initialize(IRenderer* renderer, 
                           ITextRenderer* textRenderer,
                           IWindow* window,
                           StretchMode stretchMode) {
    m_renderer = renderer;
    m_textRenderer = textRenderer;
    m_window = window;
    
    if (!m_window) {
        return false;
    }
    
    // 获取窗口尺寸
    RECT clientRect;
    GetClientRect(m_window->GetHandle(), &clientRect);
    float screenWidth = (float)(clientRect.right - clientRect.left);
    float screenHeight = (float)(clientRect.bottom - clientRect.top);
    
    // 计算UI基准尺寸
    Extent2D uiExtent = {};
    if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
        uiExtent = m_renderer->GetUIBaseSize();
    } else {
        IRenderDevice* renderDevice = m_renderer->GetRenderDevice();
        if (renderDevice) {
            uiExtent = renderDevice->GetSwapchainExtent();
        } else {
            uiExtent = m_renderer->GetSwapchainExtent();  // 使用便捷方法作为后备
        }
    }
    
    // 创建渲染上下文（使用工厂函数，避免头文件包含 vulkan.h）
    // 通过 IRenderDevice 接口获取设备信息（遵循接口隔离原则）
    IRenderDevice* renderDevice = m_renderer->GetRenderDevice();
    if (!renderDevice) {
        return false;
    }
    
    VkExtent2D vkExtent = { uiExtent.width, uiExtent.height };
    std::unique_ptr<IRenderContext> renderContext(CreateVulkanRenderContext(
        static_cast<VkDevice>(renderDevice->GetDevice()),
        static_cast<VkPhysicalDevice>(renderDevice->GetPhysicalDevice()),
        static_cast<VkCommandPool>(renderDevice->GetCommandPool()),
        static_cast<VkQueue>(renderDevice->GetGraphicsQueue()),
        static_cast<VkRenderPass>(renderDevice->GetRenderPass()),
        vkExtent
    ));
    
    // 初始化加载动画
    if (!InitializeLoadingAnimation(m_renderer, *renderContext, stretchMode, screenWidth, screenHeight)) {
        return false;
    }
    
    // 创建并初始化子管理器
    m_buttonManager = std::make_unique<ButtonUIManager>();
    if (!m_buttonManager->Initialize(*renderContext, textRenderer, m_window, stretchMode, screenWidth, screenHeight)) {
        return false;
    }
    
    m_sliderManager = std::make_unique<SliderUIManager>();
    if (!m_sliderManager->Initialize(*renderContext, m_window, stretchMode)) {
        return false;
    }
    
    m_colorManager = std::make_unique<ColorUIManager>();
    if (!m_colorManager->Initialize(m_renderer, *renderContext, textRenderer, m_window, stretchMode, screenWidth, screenHeight, m_loadingAnim)) {
        return false;
    }
    
    // 同步按钮颜色
    float r, g, b, a;
    m_buttonManager->GetButtonColor(r, g, b, a);
    m_colorManager->SetButtonColor(r, g, b, a);
    
    return true;
}

void UIManager::Cleanup() {
    // 清理子管理器
    m_colorManager.reset();
    m_sliderManager.reset();
    m_buttonManager.reset();
    
    if (m_loadingAnim) {
        m_loadingAnim->Cleanup();
        delete m_loadingAnim;
        m_loadingAnim = nullptr;
    }
}

void UIManager::HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) {
    // 委托给子管理器
    if (m_buttonManager) {
        m_buttonManager->HandleWindowResize(stretchMode, renderer);
    }
    if (m_sliderManager) {
        m_sliderManager->HandleWindowResize(stretchMode, renderer);
    }
    if (m_colorManager) {
        m_colorManager->HandleWindowResize(stretchMode, renderer);
    }
    
    // 更新加载动画位置
    if (stretchMode != StretchMode::Fit && stretchMode != StretchMode::Scaled && m_window) {
        RECT clientRect;
        GetClientRect(m_window->GetHandle(), &clientRect);
        float newScreenWidth = (float)(clientRect.right - clientRect.left);
        float newScreenHeight = (float)(clientRect.bottom - clientRect.top);
        
        if (m_loadingAnim) {
            float centerX = newScreenWidth / 2.0f - 36.0f;
            float picCenterY = newScreenHeight * 0.4f - 36.0f;
            m_loadingAnim->SetPosition(centerX, picCenterY);
        }
    }
}

void UIManager::GetAllButtons(std::vector<Button*>& buttons) const {
    buttons.clear();
    
    // 从按钮管理器获取所有按钮
    if (m_buttonManager) {
        m_buttonManager->GetAllButtons(buttons);
    }
    
    // 添加颜色控制器的按钮
    if (m_colorManager) {
        auto* colorController = m_colorManager->GetColorController();
        if (colorController) {
            std::vector<Button*> colorControllerButtons = colorController->GetButtons();
            for (Button* btn : colorControllerButtons) {
                buttons.push_back(btn);
            }
        }
        
        // 添加方块颜色控制器的按钮
        const auto& boxColorControllers = m_colorManager->GetBoxColorControllers();
        for (const auto& controller : boxColorControllers) {
            if (controller && controller->IsVisible()) {
                std::vector<Button*> boxControllerButtons = controller->GetButtons();
                for (Button* btn : boxControllerButtons) {
                    buttons.push_back(btn);
                }
            }
        }
    }
}

void UIManager::GetAllSliders(std::vector<Slider*>& sliders) const {
    sliders.clear();
    
    // 从滑块管理器和颜色管理器获取所有滑块
    if (m_sliderManager && m_colorManager) {
        m_sliderManager->GetAllSliders(sliders, 
                                      m_colorManager->GetColorController(),
                                      &m_colorManager->GetBoxColorControllers());
    }
}

// 注意：所有 getter 方法在 ui_manager_getters.cpp 中实现，避免 ui_manager.cpp 文件过大

bool UIManager::InitializeLoadingAnimation(IRenderer* renderer, const IRenderContext& renderContext, 
                                          StretchMode stretchMode, float screenWidth, float screenHeight) {
    Extent2D uiExtent = renderContext.GetSwapchainExtent();
    
    m_loadingAnim = new LoadingAnimation();
    VkExtent2D vkUiExtent = { uiExtent.width, uiExtent.height };
    if (m_loadingAnim->Initialize(
            static_cast<VkDevice>(renderer->GetDevice()),
            static_cast<VkPhysicalDevice>(renderer->GetPhysicalDevice()),
            static_cast<VkCommandPool>(renderer->GetCommandPool()),
            static_cast<VkQueue>(renderer->GetGraphicsQueue()),
            static_cast<VkRenderPass>(renderer->GetRenderPass()),
            vkUiExtent)) {
        float baseWidth = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                          (float)uiExtent.width : screenWidth;
        float baseHeight = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                           (float)uiExtent.height : screenHeight;
        float centerX = baseWidth / 2.0f - 36.0f;
        float picCenterY = baseHeight * 0.4f - 36.0f;
        m_loadingAnim->SetPosition(centerX, picCenterY);
        return true;
    }
    return false;
}

bool UIManager::HandleClick(float x, float y) {
    // 按层级从高到低处理点击（封装内部组件遍历逻辑）
    bool clicked = false;
    
    // 检查颜色控制器（滑块应该在按钮之前检查，因为滑块层级更高）
    if (m_colorManager) {
        auto* colorController = m_colorManager->GetColorController();
        if (!clicked && colorController && colorController->IsVisible()) {
            clicked = colorController->HandleMouseDown(x, y);
            if (clicked) {
                printf("[DEBUG] Color controller clicked at (%.2f, %.2f)\n", x, y);
            }
        }
        
        // 检查方块颜色控制器
        if (!clicked) {
            const auto& boxColorControllers = m_colorManager->GetBoxColorControllers();
            for (size_t i = 0; i < boxColorControllers.size(); i++) {
                if (boxColorControllers[i] && boxColorControllers[i]->IsVisible() && !clicked) {
                    clicked = boxColorControllers[i]->HandleMouseDown(x, y);
                    if (clicked) {
                        printf("[DEBUG] Box color controller %zu clicked at (%.2f, %.2f)\n", i, x, y);
                        break;
                    }
                }
            }
        }
    }
    
    // 检查按钮（通过按钮管理器）
    if (!clicked && m_buttonManager) {
        const auto& colorButtons = m_buttonManager->GetColorButtons();
        for (size_t i = 0; i < colorButtons.size(); i++) {
            if (colorButtons[i] && colorButtons[i]->IsVisible() && !clicked) {
                clicked = colorButtons[i]->HandleClick(x, y);
                if (clicked) break;
            }
        }
        
        if (!clicked) {
            const auto& boxColorButtons = m_buttonManager->GetBoxColorButtons();
            for (size_t i = 0; i < boxColorButtons.size(); i++) {
                if (boxColorButtons[i] && boxColorButtons[i]->IsVisible() && !clicked) {
                    clicked = boxColorButtons[i]->HandleClick(x, y);
                    if (clicked) break;
                }
            }
        }
        
        if (!clicked) {
            auto* colorAdjustButton = m_buttonManager->GetColorAdjustButton();
            if (colorAdjustButton) {
                clicked = colorAdjustButton->HandleClick(x, y);
            }
        }
        
        if (!clicked) {
            auto* enterButton = m_buttonManager->GetEnterButton();
            if (enterButton) {
                clicked = enterButton->HandleClick(x, y);
            }
        }
        
        if (!clicked) {
            auto* colorButton = m_buttonManager->GetColorButton();
            if (colorButton) {
                clicked = colorButton->HandleClick(x, y);
            }
        }
        
        if (!clicked) {
            auto* leftButton = m_buttonManager->GetLeftButton();
            if (leftButton) {
                clicked = leftButton->HandleClick(x, y);
            }
        }
    }
    
    // 检查滑块
    if (!clicked && m_sliderManager) {
        auto* orangeSlider = m_sliderManager->GetOrangeSlider();
        if (orangeSlider) {
            clicked = orangeSlider->HandleMouseDown(x, y);
            if (clicked) {
                printf("[DEBUG] Slider clicked at (%.2f, %.2f)\n", x, y);
            }
        }
    }
    
    if (!clicked) {
        printf("[DEBUG] Click position is outside button bounds\n");
    }
    
    return clicked;
}

void UIManager::HandleMouseMove(float x, float y) {
    if (x >= 0.0f && y >= 0.0f) {
        // 处理按钮悬停效果（通过子管理器）
        if (m_buttonManager) {
            const auto& colorButtons = m_buttonManager->GetColorButtons();
            for (const auto& button : colorButtons) {
                if (button && button->IsVisible()) {
                    button->HandleMouseMove(x, y);
                }
            }
            
            const auto& boxColorButtons = m_buttonManager->GetBoxColorButtons();
            for (const auto& button : boxColorButtons) {
                if (button && button->IsVisible()) {
                    button->HandleMouseMove(x, y);
                }
            }
            
            auto* colorAdjustButton = m_buttonManager->GetColorAdjustButton();
            if (colorAdjustButton) {
                colorAdjustButton->HandleMouseMove(x, y);
            }
            
            auto* enterButton = m_buttonManager->GetEnterButton();
            if (enterButton) {
                enterButton->HandleMouseMove(x, y);
            }
            
            auto* colorButton = m_buttonManager->GetColorButton();
            if (colorButton) {
                colorButton->HandleMouseMove(x, y);
            }
            
            auto* leftButton = m_buttonManager->GetLeftButton();
            if (leftButton) {
                leftButton->HandleMouseMove(x, y);
            }
        }
        
        // 处理滑块
        if (m_sliderManager) {
            auto* orangeSlider = m_sliderManager->GetOrangeSlider();
            if (orangeSlider) {
                orangeSlider->HandleMouseMove(x, y);
            }
        }
        
        // 处理颜色控制器
        if (m_colorManager) {
            auto* colorController = m_colorManager->GetColorController();
            if (colorController && colorController->IsVisible()) {
                colorController->HandleMouseMove(x, y);
            }
            
            const auto& boxColorControllers = m_colorManager->GetBoxColorControllers();
            for (const auto& controller : boxColorControllers) {
                if (controller && controller->IsVisible()) {
                    controller->HandleMouseMove(x, y);
                }
            }
        }
    } else {
        // 鼠标在视口外，清除所有按钮的悬停状态
        if (m_buttonManager) {
            const auto& colorButtons = m_buttonManager->GetColorButtons();
            for (const auto& button : colorButtons) {
                if (button) {
                    button->HandleMouseMove(-1.0f, -1.0f);
                }
            }
            
            const auto& boxColorButtons = m_buttonManager->GetBoxColorButtons();
            for (const auto& button : boxColorButtons) {
                if (button) {
                    button->HandleMouseMove(-1.0f, -1.0f);
                }
            }
            
            auto* colorAdjustButton = m_buttonManager->GetColorAdjustButton();
            if (colorAdjustButton) {
                colorAdjustButton->HandleMouseMove(-1.0f, -1.0f);
            }
            
            auto* enterButton = m_buttonManager->GetEnterButton();
            if (enterButton) {
                enterButton->HandleMouseMove(-1.0f, -1.0f);
            }
            
            auto* colorButton = m_buttonManager->GetColorButton();
            if (colorButton) {
                colorButton->HandleMouseMove(-1.0f, -1.0f);
            }
            
            auto* leftButton = m_buttonManager->GetLeftButton();
            if (leftButton) {
                leftButton->HandleMouseMove(-1.0f, -1.0f);
            }
        }
    }
}

void UIManager::HandleMouseUp() {
    // 处理滑块
    if (m_sliderManager) {
        auto* orangeSlider = m_sliderManager->GetOrangeSlider();
        if (orangeSlider) {
            orangeSlider->HandleMouseUp();
        }
    }
    
    // 处理颜色控制器
    if (m_colorManager) {
        auto* colorController = m_colorManager->GetColorController();
        if (colorController) {
            colorController->HandleMouseUp();
        }
        
        const auto& boxColorControllers = m_colorManager->GetBoxColorControllers();
        for (const auto& controller : boxColorControllers) {
            if (controller) {
                controller->HandleMouseUp();
            }
        }
    }
}

void UIManager::SubscribeToEvents(IEventBus* eventBus) {
    if (!eventBus) {
        return;
    }
    
    // 订阅UI点击事件
    eventBus->Subscribe(EventType::UIClick, [this](const Event& e) {
        const UIClickEvent& clickEvent = static_cast<const UIClickEvent&>(e);
        HandleClick(clickEvent.uiX, clickEvent.uiY);
        // 如果拉伸模式不是Fit，需要更新UI组件位置
        if (clickEvent.stretchMode != StretchMode::Fit && m_renderer) {
            HandleWindowResize(clickEvent.stretchMode, m_renderer);
        }
    });
    
    // 订阅UI鼠标移动事件
    eventBus->Subscribe(EventType::MouseMovedUI, [this](const Event& e) {
        const MouseMovedUIEvent& moveEvent = static_cast<const MouseMovedUIEvent&>(e);
        HandleMouseMove(moveEvent.uiX, moveEvent.uiY);
    });
    
    // 订阅鼠标释放事件
    eventBus->Subscribe(EventType::MouseUp, [this](const Event& e) {
        HandleMouseUp();
    });
    
    // 订阅窗口大小变化事件
    eventBus->Subscribe(EventType::WindowResizeRequest, [this](const Event& e) {
        const WindowResizeRequestEvent& resizeEvent = static_cast<const WindowResizeRequestEvent&>(e);
        if (resizeEvent.renderer) {
            HandleWindowResize(resizeEvent.stretchMode, resizeEvent.renderer);
        }
    });
}

void UIManager::SetupCallbacks(IEventBus* eventBus) {
    if (!eventBus || !m_buttonManager || !m_colorManager) {
        return;
    }
    
    // 使用事件总线解耦：发布事件而不是直接调用具体类的方法
    // 这样 UIManager 不需要知道 SceneManager 的具体实现
    
    // 设置进入按钮的回调 - 发布事件
    auto* enterButton = m_buttonManager->GetEnterButton();
    if (enterButton) {
        enterButton->SetOnClickCallback([eventBus]() {
            printf("[DEBUG] Button clicked! Switching to Shader mode\n");
            ButtonClickedEvent event("enter");
            eventBus->Publish(event);
        });
    }
    
    // 设置颜色按钮的回调
    auto* colorButton = m_buttonManager->GetColorButton();
    if (colorButton) {
        colorButton->SetOnClickCallback([this]() {
            bool expanded = !m_boxColorButtonsExpanded;
            m_boxColorButtonsExpanded = expanded;
            printf("[DEBUG] Color button clicked! Box color buttons expanded: %s\n", 
                   expanded ? "true" : "false");
            
            const auto& boxColorButtons = m_buttonManager->GetBoxColorButtons();
            for (const auto& button : boxColorButtons) {
                if (button) {
                    button->SetVisible(expanded);
                }
            }
            
            const auto& boxColorControllers = m_colorManager->GetBoxColorControllers();
            for (const auto& controller : boxColorControllers) {
                if (controller) {
                    controller->SetVisible(false);
                }
            }
        });
    }
    
    // 设置左侧按钮的回调 - 发布事件
    auto* leftButton = m_buttonManager->GetLeftButton();
    if (leftButton) {
        leftButton->SetOnClickCallback([eventBus]() {
            printf("[DEBUG] Left button clicked! Entering 3D scene (LoadingCubes)\n");
            ButtonClickedEvent event("left");
            eventBus->Publish(event);
        });
    }
    
    // 设置颜色按钮的回调（9个颜色按钮）
    const auto& colorButtons = m_buttonManager->GetColorButtons();
    for (size_t i = 0; i < colorButtons.size(); i++) {
        if (colorButtons[i]) {
            size_t boxIndex = i;
            colorButtons[i]->SetOnClickCallback([this, boxIndex]() {
                printf("[DEBUG] Color button %zu clicked! Showing color controller for box %zu\n", 
                       boxIndex, boxIndex);
                const auto& boxColorControllers = m_colorManager->GetBoxColorControllers();
                for (size_t j = 0; j < boxColorControllers.size(); j++) {
                    if (boxColorControllers[j]) {
                        boxColorControllers[j]->SetVisible(j == boxIndex);
                    }
                }
            });
        }
    }
    
    // 设置方块颜色按钮的回调
    const auto& boxColorButtons = m_buttonManager->GetBoxColorButtons();
    for (size_t i = 0; i < boxColorButtons.size(); i++) {
        if (boxColorButtons[i]) {
            size_t boxIndex = i;
            boxColorButtons[i]->SetOnClickCallback([this, boxIndex]() {
                printf("[DEBUG] Box color button %zu clicked! Showing color controller for box %zu\n", 
                       boxIndex, boxIndex);
                const auto& boxColorControllers = m_colorManager->GetBoxColorControllers();
                for (size_t j = 0; j < boxColorControllers.size(); j++) {
                    if (boxColorControllers[j]) {
                        boxColorControllers[j]->SetVisible(j == boxIndex);
                    }
                }
            });
        }
    }
    
    // 设置颜色调整按钮的回调
    auto* colorAdjustButton = m_buttonManager->GetColorAdjustButton();
    if (colorAdjustButton) {
        colorAdjustButton->SetOnClickCallback([this]() {
            auto* colorController = m_colorManager->GetColorController();
            if (colorController) {
                bool visible = !colorController->IsVisible();
                colorController->SetVisible(visible);
                printf("[DEBUG] Color adjust button clicked! Color controller visible: %s\n", 
                       visible ? "true" : "false");
            }
        });
    }
    
    // 设置颜色控制器的回调，更新按钮颜色
    auto* colorController = m_colorManager->GetColorController();
    if (colorController) {
        colorController->SetOnColorChangedCallback([this](float r, float g, float b, float a) {
            // 更新颜色管理器的内部状态
            m_colorManager->SetButtonColor(r, g, b, a);
            // 更新按钮管理器的颜色
            m_buttonManager->SetButtonColor(r, g, b, a);
            // 实际更新颜色按钮的颜色
            auto* colorButton = m_buttonManager->GetColorButton();
            if (colorButton) {
                colorButton->SetColor(r, g, b, a);
            }
            printf("[DEBUG] Color changed to (%.2f, %.2f, %.2f, %.2f), button color updated\n", 
                   r, g, b, a);
        });
    }
}
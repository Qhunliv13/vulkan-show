#include "ui/color_controller/color_controller.h"  // 1. 对应头文件

#include <algorithm>       // 2. 系统头文件
#include <cmath>           // 2. 系统头文件
#include <memory>          // 2. 系统头文件

#include <vulkan/vulkan.h>     // 3. 第三方库头文件

#include "core/interfaces/itext_renderer.h"        // 4. 项目头文件（接口）
#include "core/types/render_types.h"               // 4. 项目头文件（抽象类型）
#include "renderer/vulkan/vulkan_render_context_factory.h"  // 4. 项目头文件（工厂函数）
#include "ui/button/button.h"                      // 4. 项目头文件（UI组件实现）
#include "ui/slider/slider.h"                     // 4. 项目头文件（UI组件实现）

ColorController::ColorController() 
    : m_colorDisplayButton(std::make_unique<Button>())
    , m_colorDisplayButtonInitialized(false)
    , m_visible(false)
    , m_fixedScreenSize(false)
    , m_initialized(false)
    , m_textRenderer(nullptr)
{
    m_slidersInitialized.resize(4, false);
    // 创建4个滑块的智能指针
    for (int i = 0; i < 4; i++) {
        m_sliders.push_back(std::make_unique<Slider>());
    }
}

ColorController::~ColorController() {
    Cleanup();
}

bool ColorController::Initialize(DeviceHandle device, PhysicalDeviceHandle physicalDevice,
                                CommandPoolHandle commandPool, QueueHandle graphicsQueue,
                                RenderPassHandle renderPass, Extent2D swapchainExtent,
                                const ColorControllerConfig& config,
                                ITextRenderer* textRenderer) {
    // 存储抽象类型（在实现层转换为Vulkan类型使用）
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_renderPass = renderPass;
    m_swapchainExtent = swapchainExtent;
    m_textRenderer = textRenderer;
    m_config = config;
    
    // 设置初始颜色
    m_colorR = config.initialR;
    m_colorG = config.initialG;
    m_colorB = config.initialB;
    m_colorA = config.initialA;
    m_visible = config.visible;
    
    // 滑块标签和颜色
    const char* sliderLabels[4] = {"R", "G", "B", "A"};
    float sliderColors[4][3] = {
        {1.0f, 0.0f, 0.0f},  // R - 红色
        {0.0f, 1.0f, 0.0f},  // G - 绿色
        {0.0f, 0.0f, 1.0f},  // B - 蓝色
        {0.5f, 0.5f, 0.5f}   // A - 灰色
    };
    
    // 计算滑块起始位置
    float sliderStartX = config.relativeX;
    float sliderStartY = config.relativeY;
    float screenHeight = config.screenHeight;
    
    // 初始化4个滑块（垂直排列）
    for (int i = 0; i < 4; i++) {
        SliderConfig sliderConfig = SliderConfig::CreateRelative(
            sliderStartX, 
            sliderStartY + i * (config.sliderSpacing / screenHeight),
            config.sliderWidth, 
            config.sliderHeight,
            0.0f, 255.0f, 255.0f);  // 范围0-255，默认255
        
        sliderConfig.trackColorR = 0.3f;
        sliderConfig.trackColorG = 0.3f;
        sliderConfig.trackColorB = 0.3f;
        sliderConfig.fillColorR = sliderColors[i][0];
        sliderConfig.fillColorG = sliderColors[i][1];
        sliderConfig.fillColorB = sliderColors[i][2];
        sliderConfig.thumbColorR = sliderColors[i][0];
        sliderConfig.thumbColorG = sliderColors[i][1];
        sliderConfig.thumbColorB = sliderColors[i][2];
        // 根据滑块尺寸自动计算拖拽点大小（保持比例，拖拽点约为滑块高度的3-4倍）
        sliderConfig.thumbWidth = config.sliderHeight * 3.3f;  // 原来20.0f对应6.0f高度，比例约为3.3
        sliderConfig.thumbHeight = config.sliderHeight * 3.3f;
        sliderConfig.zIndex = config.zIndex;
        sliderConfig.useRelativePosition = true;
        
        // 设置初始值（根据初始颜色）
        float initialValue = 255.0f;
        if (i == 0) initialValue = m_colorR * 255.0f;
        else if (i == 1) initialValue = m_colorG * 255.0f;
        else if (i == 2) initialValue = m_colorB * 255.0f;
        else if (i == 3) initialValue = m_colorA * 255.0f;
        sliderConfig.defaultValue = initialValue;
        
        // 使用废弃接口初始化滑块（直接使用抽象类型）
        if (m_sliders[i]->Initialize(
                device,
                physicalDevice,
                commandPool,
                graphicsQueue,
                renderPass,
                swapchainExtent,
                sliderConfig,
                false)) {  // 使用传统渲染方式
            
            m_sliders[i]->SetVisible(m_visible);
            m_sliders[i]->UpdateScreenSize(config.screenWidth, config.screenHeight);
            m_slidersInitialized[i] = true;
            
            // 设置滑块值变化回调
            int sliderIndex = i;  // 捕获索引
            m_sliders[i]->SetOnValueChangedCallback([this, sliderIndex](float value) {
                OnSliderValueChanged(sliderIndex, value);
            });
        }
    }
    
    // 创建颜色显示区域（在滑块下方）
    float displayY = sliderStartY + 4 * (config.sliderSpacing / screenHeight) + 
                     (config.displayOffsetY / screenHeight);
    
    ButtonConfig colorDisplayConfig = ButtonConfig::CreateRelative(
        sliderStartX, 
        displayY,
        config.displayWidth, 
        config.displayHeight,
        m_colorR, m_colorG, m_colorB, m_colorA);  // 初始颜色
    
    colorDisplayConfig.zIndex = config.zIndex;
    colorDisplayConfig.enableText = true;
    colorDisplayConfig.text = "颜色";
    colorDisplayConfig.textColorR = 1.0f - m_colorR;  // 文本颜色取反色以便看清
    colorDisplayConfig.textColorG = 1.0f - m_colorG;
    colorDisplayConfig.textColorB = 1.0f - m_colorB;
    colorDisplayConfig.textColorA = 1.0f;
    
    // 创建临时渲染上下文（直接使用抽象类型）
    std::unique_ptr<IRenderContext> tempContext(CreateVulkanRenderContext(
        device,
        physicalDevice,
        commandPool,
        graphicsQueue,
        renderPass,
        swapchainExtent));
    
    if (m_colorDisplayButton->Initialize(
            tempContext.get(),
            colorDisplayConfig,
            textRenderer)) {
        
        m_colorDisplayButton->SetVisible(m_visible);
        m_colorDisplayButtonInitialized = true;
    }
    
    m_initialized = true;
    return true;
}

void ColorController::Cleanup() {
    if (!m_initialized) {
        return;
    }
    
    // 清空回调函数，防止事件泄露
    m_onColorChangedCallback = nullptr;
    
    // 清理滑块
    for (int i = 0; i < 4; i++) {
        if (m_slidersInitialized[i]) {
            m_sliders[i]->Cleanup();
            m_slidersInitialized[i] = false;
        }
    }
    
    // 清理颜色显示按钮
    if (m_colorDisplayButtonInitialized && m_colorDisplayButton) {
        m_colorDisplayButton->Cleanup();
        m_colorDisplayButtonInitialized = false;
    }
    m_colorDisplayButton.reset();
    
    m_initialized = false;
}

void ColorController::SetColor(float r, float g, float b, float a) {
    m_colorR = std::max(0.0f, std::min(1.0f, r));
    m_colorG = std::max(0.0f, std::min(1.0f, g));
    m_colorB = std::max(0.0f, std::min(1.0f, b));
    m_colorA = std::max(0.0f, std::min(1.0f, a));
    
    // 更新滑块值
    if (m_slidersInitialized[0]) {
        m_sliders[0]->SetValue(m_colorR * 255.0f);
    }
    if (m_slidersInitialized[1]) {
        m_sliders[1]->SetValue(m_colorG * 255.0f);
    }
    if (m_slidersInitialized[2]) {
        m_sliders[2]->SetValue(m_colorB * 255.0f);
    }
    if (m_slidersInitialized[3]) {
        m_sliders[3]->SetValue(m_colorA * 255.0f);
    }
    
    // 更新颜色显示区域
    UpdateColorDisplay();
}

void ColorController::SetVisible(bool visible) {
    m_visible = visible;
    
    // 更新所有滑块的可见性
    for (int i = 0; i < 4; i++) {
        if (m_slidersInitialized[i]) {
            m_sliders[i]->SetVisible(visible);
        }
    }
    
    // 更新颜色显示区域的可见性
    if (m_colorDisplayButtonInitialized && m_colorDisplayButton) {
        m_colorDisplayButton->SetVisible(visible);
    }
}

void ColorController::UpdateScreenSize(float screenWidth, float screenHeight) {
    m_config.screenWidth = screenWidth;
    m_config.screenHeight = screenHeight;
    
    // 更新所有滑块的屏幕大小
    for (int i = 0; i < 4; i++) {
        if (m_slidersInitialized[i]) {
            m_sliders[i]->UpdateScreenSize(screenWidth, screenHeight);
        }
    }
    
    // 更新颜色显示按钮的屏幕大小
    if (m_colorDisplayButtonInitialized && m_colorDisplayButton) {
        m_colorDisplayButton->UpdateScreenSize(screenWidth, screenHeight);
    }
}

void ColorController::SetFixedScreenSize(bool fixed) {
    m_fixedScreenSize = fixed;
    
    // 设置颜色显示按钮的固定屏幕大小
    if (m_colorDisplayButtonInitialized && m_colorDisplayButton) {
        m_colorDisplayButton->SetFixedScreenSize(fixed);
    }
}

void ColorController::Render(CommandBufferHandle commandBuffer, Extent2D extent) {
    // 渲染所有滑块
    for (int i = 0; i < 4; i++) {
        if (m_slidersInitialized[i] && m_sliders[i]->IsVisible()) {
            // 使用接口方法（Slider已实现ISlider接口）
            m_sliders[i]->Render(commandBuffer, extent);
        }
    }
    
    // 渲染颜色显示按钮
    if (m_colorDisplayButtonInitialized && m_colorDisplayButton && m_colorDisplayButton->IsVisible()) {
        // 使用接口方法（Button已实现IButton接口）
        m_colorDisplayButton->Render(commandBuffer, extent);
    }
}

bool ColorController::HandleMouseDown(float clickX, float clickY) {
    // 处理滑块鼠标事件
    for (int i = 0; i < 4; i++) {
        if (m_slidersInitialized[i] && m_sliders[i]->IsVisible()) {
            if (m_sliders[i]->HandleMouseDown(clickX, clickY)) {
                return true;
            }
        }
    }
    
    // 颜色显示按钮不需要处理鼠标事件（它只是显示颜色）
    
    return false;
}

bool ColorController::HandleMouseMove(float mouseX, float mouseY) {
    // 处理滑块拖动
    bool handled = false;
    for (int i = 0; i < 4; i++) {
        if (m_slidersInitialized[i] && m_sliders[i]->IsVisible()) {
            if (m_sliders[i]->HandleMouseMove(mouseX, mouseY)) {
                handled = true;
            }
        }
    }
    return handled;
}

void ColorController::HandleMouseUp() {
    // 处理滑块鼠标释放
    for (int i = 0; i < 4; i++) {
        if (m_slidersInitialized[i]) {
            m_sliders[i]->HandleMouseUp();
        }
    }
    
    // 颜色显示按钮不需要处理鼠标释放事件
}

std::vector<IButton*> ColorController::GetButtons() const {
    std::vector<IButton*> buttons;
    if (m_colorDisplayButtonInitialized && m_colorDisplayButton) {
        // Button继承IButton，可以直接使用
        buttons.push_back(m_colorDisplayButton.get());
    }
    return buttons;
}

std::vector<ISlider*> ColorController::GetSliders() const {
    std::vector<ISlider*> sliders;
    for (int i = 0; i < 4; i++) {
        if (m_slidersInitialized[i]) {
            // Slider继承ISlider，可以直接使用
            sliders.push_back(m_sliders[i].get());
        }
    }
    return sliders;
}

void ColorController::UpdateColorDisplay() {
    if (!m_colorDisplayButtonInitialized || !m_colorDisplayButton) {
        return;
    }
    
    // 更新颜色显示按钮的颜色
    m_colorDisplayButton->SetColor(m_colorR, m_colorG, m_colorB, m_colorA);
    
    // 更新文本颜色（取反色以便看清）
    m_colorDisplayButton->SetTextColor(
        1.0f - m_colorR, 
        1.0f - m_colorG, 
        1.0f - m_colorB, 
        1.0f);
}

void ColorController::OnSliderValueChanged(int sliderIndex, float value) {
    // 将滑块值（0-255）转换为颜色值（0.0-1.0）
    float normalizedValue = value / 255.0f;
    
    // 更新对应的颜色分量
    if (sliderIndex == 0) {
        m_colorR = normalizedValue;
    } else if (sliderIndex == 1) {
        m_colorG = normalizedValue;
    } else if (sliderIndex == 2) {
        m_colorB = normalizedValue;
    } else if (sliderIndex == 3) {
        m_colorA = normalizedValue;
    }
    
    // 更新颜色显示区域
    UpdateColorDisplay();
    
    // 调用颜色变化回调
    if (m_onColorChangedCallback) {
        m_onColorChangedCallback(m_colorR, m_colorG, m_colorB, m_colorA);
    }
}


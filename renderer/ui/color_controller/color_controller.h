#pragma once

#include <vulkan/vulkan.h>
#include <functional>
#include <vector>
#include "ui/slider/slider.h"
#include "ui/button/button.h"

// 前向声明
class TextRenderer;

// 颜色控制器配置结构体
struct ColorControllerConfig {
    // 位置（相对位置，0.0-1.0）
    float relativeX = 0.1f;
    float relativeY = 0.3f;
    
    // 滑块尺寸
    float sliderWidth = 200.0f;
    float sliderHeight = 6.0f;
    float sliderSpacing = 50.0f;  // 滑块之间的间距
    
    // 颜色显示区域尺寸
    float displayWidth = 100.0f;
    float displayHeight = 50.0f;
    float displayOffsetY = 30.0f;  // 显示区域在最后一个滑块下方的偏移
    
    // 初始颜色（RGBA，0.0-1.0）
    float initialR = 1.0f;
    float initialG = 1.0f;
    float initialB = 1.0f;
    float initialA = 1.0f;
    
    // 渲染层级（z-index）
    int zIndex = 19;
    
    // 是否默认可见
    bool visible = false;
    
    // 屏幕尺寸（用于相对位置计算）
    float screenWidth = 800.0f;
    float screenHeight = 800.0f;
};

// 颜色控制器组件 - 封装4个RGBA滑块和颜色显示区域
class ColorController {
public:
    ColorController();
    ~ColorController();
    
    // 初始化颜色控制器
    bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice,
                    VkCommandPool commandPool, VkQueue graphicsQueue,
                    VkRenderPass renderPass, VkExtent2D swapchainExtent,
                    const ColorControllerConfig& config,
                    TextRenderer* textRenderer = nullptr);
    
    // 清理资源
    void Cleanup();
    
    // 设置颜色（RGBA，0.0-1.0）
    void SetColor(float r, float g, float b, float a = 1.0f);
    
    // 获取当前颜色（RGBA，0.0-1.0）
    void GetColor(float& r, float& g, float& b, float& a) const {
        r = m_colorR;
        g = m_colorG;
        b = m_colorB;
        a = m_colorA;
    }
    
    // 设置可见性
    void SetVisible(bool visible);
    
    // 获取可见性
    bool IsVisible() const { return m_visible; }
    
    // 更新窗口大小（用于相对位置计算）
    void UpdateScreenSize(float screenWidth, float screenHeight);
    
    // 设置固定屏幕大小（用于FIT模式）
    void SetFixedScreenSize(bool fixed);
    
    // 渲染所有组件
    void Render(VkCommandBuffer commandBuffer, VkExtent2D extent);
    
    // 处理鼠标事件
    bool HandleMouseDown(float clickX, float clickY);
    bool HandleMouseMove(float mouseX, float mouseY);
    void HandleMouseUp();
    
    // 设置颜色变化回调函数
    void SetOnColorChangedCallback(std::function<void(float, float, float, float)> callback) {
        m_onColorChangedCallback = callback;
    }
    
    // 获取所有按钮指针（用于添加到渲染列表）
    std::vector<Button*> GetButtons() const;
    
    // 获取所有滑块指针（用于添加到渲染列表）
    std::vector<Slider*> GetSliders() const;

private:
    // 更新颜色显示区域
    void UpdateColorDisplay();
    
    // 滑块值变化回调
    void OnSliderValueChanged(int sliderIndex, float value);
    
    // 配置
    ColorControllerConfig m_config;
    
    // 滑块
    std::vector<std::unique_ptr<Slider>> m_sliders;
    std::vector<bool> m_slidersInitialized;
    
    // 颜色显示按钮
    Button m_colorDisplayButton;
    bool m_colorDisplayButtonInitialized;
    
    // 当前颜色（RGBA，0.0-1.0）
    float m_colorR = 1.0f;
    float m_colorG = 1.0f;
    float m_colorB = 1.0f;
    float m_colorA = 1.0f;
    
    // 可见性
    bool m_visible = false;
    
    // 是否使用固定屏幕大小
    bool m_fixedScreenSize = false;
    
    // Vulkan对象
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkExtent2D m_swapchainExtent = {};
    TextRenderer* m_textRenderer = nullptr;
    
    // 颜色变化回调
    std::function<void(float, float, float, float)> m_onColorChangedCallback;
    
    // 初始化状态
    bool m_initialized = false;
};


#pragma once

#include <functional>    // 2. 系统头文件
#include <memory>         // 2. 系统头文件
#include <vector>         // 2. 系统头文件

#include "core/interfaces/icolor_controller.h"  // 4. 项目头文件（接口）
#include "core/types/render_types.h"  // 4. 项目头文件（类型）

// 前向声明
class ITextRenderer;
class Button;
class Slider;

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

/**
 * 颜色控制器组件 - 封装4个RGBA滑块和颜色显示区域
 * 
 * 实现IColorController接口，支持接口隔离原则
 * 通过依赖注入接收渲染上下文和文本渲染器接口，避免直接依赖具体实现类
 */
class ColorController : public IColorController {
public:
    ColorController();
    ~ColorController();
    
    // 初始化颜色控制器（使用抽象类型）
    bool Initialize(DeviceHandle device, PhysicalDeviceHandle physicalDevice,
                    CommandPoolHandle commandPool, QueueHandle graphicsQueue,
                    RenderPassHandle renderPass, Extent2D swapchainExtent,
                    const ColorControllerConfig& config,
                    ITextRenderer* textRenderer = nullptr) override;
    
    // 清理资源
    void Cleanup() override;
    
    // 设置颜色（RGBA，0.0-1.0）
    void SetColor(float r, float g, float b, float a = 1.0f) override;
    
    // 获取当前颜色（RGBA，0.0-1.0）
    void GetColor(float& r, float& g, float& b, float& a) const override {
        r = m_colorR;
        g = m_colorG;
        b = m_colorB;
        a = m_colorA;
    }
    
    // 设置可见性
    void SetVisible(bool visible) override;
    
    // 获取可见性
    bool IsVisible() const override { return m_visible; }
    
    // 更新窗口大小（用于相对位置计算）
    void UpdateScreenSize(float screenWidth, float screenHeight) override;
    
    // 设置固定屏幕大小（用于FIT模式）
    void SetFixedScreenSize(bool fixed) override;
    
    // 渲染所有组件（使用抽象类型）
    void Render(CommandBufferHandle commandBuffer, Extent2D extent) override;
    
    // 处理鼠标事件
    bool HandleMouseDown(float clickX, float clickY) override;
    bool HandleMouseMove(float mouseX, float mouseY) override;
    void HandleMouseUp() override;
    
    // 设置颜色变化回调函数
    void SetOnColorChangedCallback(std::function<void(float, float, float, float)> callback) override {
        m_onColorChangedCallback = callback;
    }
    
    // 获取所有按钮指针（用于添加到渲染列表）
    std::vector<IButton*> GetButtons() const override;
    
    // 获取所有滑块指针（用于添加到渲染列表）
    std::vector<ISlider*> GetSliders() const override;

private:
    // 更新颜色显示区域
    void UpdateColorDisplay();
    
    // 滑块值变化回调
    void OnSliderValueChanged(int sliderIndex, float value);
    
    // 配置
    ColorControllerConfig m_config;
    
    // 滑块（使用智能指针管理，通过接口访问）
    std::vector<std::unique_ptr<Slider>> m_sliders;
    std::vector<bool> m_slidersInitialized;
    
    // 颜色显示按钮（使用智能指针管理，通过接口访问）
    std::unique_ptr<Button> m_colorDisplayButton;
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
    // 渲染设备对象（使用抽象类型，在实现层转换为Vulkan类型）
    DeviceHandle m_device = nullptr;
    PhysicalDeviceHandle m_physicalDevice = nullptr;
    CommandPoolHandle m_commandPool = nullptr;
    QueueHandle m_graphicsQueue = nullptr;
    RenderPassHandle m_renderPass = nullptr;
    Extent2D m_swapchainExtent = {};
    ITextRenderer* m_textRenderer = nullptr;
    
    // 颜色变化回调
    std::function<void(float, float, float, float)> m_onColorChangedCallback;
    
    // 初始化状态
    bool m_initialized = false;
};


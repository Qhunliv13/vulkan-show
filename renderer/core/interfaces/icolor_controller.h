#pragma once

#include <functional>  // 2. 系统头文件
#include <vector>       // 2. 系统头文件

#include "core/types/render_types.h"  // 4. 项目头文件（类型）

// 前向声明
class IButton;
class ISlider;
class ITextRenderer;

// 颜色控制器配置结构体（与ColorController类共享，定义在color_controller.h中）
struct ColorControllerConfig;

/**
 * 颜色控制器组件接口 - 用于解耦颜色控制器组件与UI管理，不依赖具体实现
 * 
 * 职责：提供平台无关的颜色控制器组件接口，支持RGBA滑块和颜色显示
 * 设计：使用抽象类型，支持多种渲染后端（Vulkan、OpenGL、DirectX等）
 * 
 * 使用方式：
 * 1. 通过依赖注入接收渲染设备和ITextRenderer
 * 2. 使用接口指针操作，无需了解具体实现
 * 3. 支持颜色调整、滑块交互、颜色变化回调等功能
 */
class IColorController {
public:
    virtual ~IColorController() = default;
    
    /**
     * 初始化颜色控制器
     * 
     * @param device 设备句柄（使用抽象类型）
     * @param physicalDevice 物理设备句柄（使用抽象类型）
     * @param commandPool 命令池句柄（使用抽象类型）
     * @param graphicsQueue 图形队列句柄（使用抽象类型）
     * @param renderPass 渲染通道句柄（使用抽象类型）
     * @param swapchainExtent 交换链尺寸
     * @param config 颜色控制器配置
     * @param textRenderer 文字渲染器接口（可选，用于渲染文本）
     * @return 初始化成功返回true，失败返回false
     */
    virtual bool Initialize(DeviceHandle device, PhysicalDeviceHandle physicalDevice,
                           CommandPoolHandle commandPool, QueueHandle graphicsQueue,
                           RenderPassHandle renderPass, Extent2D swapchainExtent,
                           const ColorControllerConfig& config,
                           ITextRenderer* textRenderer = nullptr) = 0;
    
    /**
     * 清理资源
     */
    virtual void Cleanup() = 0;
    
    /**
     * 设置颜色（RGBA，0.0-1.0）
     */
    virtual void SetColor(float r, float g, float b, float a = 1.0f) = 0;
    
    /**
     * 获取当前颜色（RGBA，0.0-1.0）
     */
    virtual void GetColor(float& r, float& g, float& b, float& a) const = 0;
    
    /**
     * 设置可见性
     */
    virtual void SetVisible(bool visible) = 0;
    
    /**
     * 获取可见性
     */
    virtual bool IsVisible() const = 0;
    
    /**
     * 更新窗口大小（用于相对位置计算）
     */
    virtual void UpdateScreenSize(float screenWidth, float screenHeight) = 0;
    
    /**
     * 设置固定屏幕大小（用于FIT模式）
     */
    virtual void SetFixedScreenSize(bool fixed) = 0;
    
    /**
     * 渲染所有组件
     */
    virtual void Render(CommandBufferHandle commandBuffer, Extent2D extent) = 0;
    
    /**
     * 处理鼠标事件
     */
    virtual bool HandleMouseDown(float clickX, float clickY) = 0;
    virtual bool HandleMouseMove(float mouseX, float mouseY) = 0;
    virtual void HandleMouseUp() = 0;
    
    /**
     * 设置颜色变化回调函数
     */
    virtual void SetOnColorChangedCallback(std::function<void(float, float, float, float)> callback) = 0;
    
    /**
     * 获取所有按钮指针（用于添加到渲染列表）
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由IColorController管理生命周期
     */
    virtual std::vector<IButton*> GetButtons() const = 0;
    
    /**
     * 获取所有滑块指针（用于添加到渲染列表）
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由IColorController管理生命周期
     */
    virtual std::vector<ISlider*> GetSliders() const = 0;
};


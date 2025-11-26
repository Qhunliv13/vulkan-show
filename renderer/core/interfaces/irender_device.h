#pragma once

#include "core/types/render_types.h"  // 4. 项目头文件（类型）

/**
 * 渲染设备接口 - 提供渲染所需的底层设备资源，不暴露具体渲染后端类型
 * 
 * 职责：提供平台无关的设备资源访问接口，隐藏具体渲染后端实现
 * 设计：使用抽象句柄类型，支持多种渲染后端（Vulkan、OpenGL、DirectX等）
 * 
 * 使用方式：
 * 1. 通过 IRenderer::GetRenderDevice() 获取接口指针
 * 2. 使用接口指针访问设备资源，无需了解具体实现
 * 3. 注意：应优先使用 IRenderCommand 抽象层，而不是直接访问这些对象
 */
class IRenderDevice {
public:
    virtual ~IRenderDevice() = default;
    
    // 获取设备句柄（不透明指针，具体类型由实现决定）
    // 注意：应优先使用 IRenderCommand 抽象层，而不是直接访问这些对象
    virtual DeviceHandle GetDevice() const = 0;
    virtual PhysicalDeviceHandle GetPhysicalDevice() const = 0;
    virtual CommandPoolHandle GetCommandPool() const = 0;
    virtual QueueHandle GetGraphicsQueue() const = 0;
    virtual RenderPassHandle GetRenderPass() const = 0;
    
    // 获取交换链信息
    virtual Extent2D GetSwapchainExtent() const = 0;
    virtual ImageFormat GetSwapchainFormat() const = 0;
    virtual uint32_t GetSwapchainImageCount() const = 0;
};


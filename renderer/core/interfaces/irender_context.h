#pragma once

#include <cstdint>  // 2. 系统头文件
#include "core/types/render_types.h"  // 4. 项目头文件

/**
 * 渲染上下文接口 - 抽象层，用于解耦UI组件与底层渲染API
 * 
 * 此接口不依赖任何具体渲染后端（Vulkan、OpenGL、DirectX等），
 * 通过抽象句柄类型实现平台无关的渲染操作。
 * 
 * 使用方式：
 * 1. 通过 IRenderDevice 接口获取渲染设备句柄
 * 2. 使用具体后端的工厂函数创建实现（如 CreateVulkanRenderContext）
 * 3. 通过接口指针使用，无需了解具体实现细节
 */
class IRenderContext {
public:
    virtual ~IRenderContext() = default;
    
    /**
     * 获取设备对象句柄（抽象句柄，不暴露具体类型）
     */
    virtual DeviceHandle GetDevice() const = 0;
    
    /**
     * 获取物理设备句柄
     */
    virtual PhysicalDeviceHandle GetPhysicalDevice() const = 0;
    
    /**
     * 获取命令池句柄
     */
    virtual CommandPoolHandle GetCommandPool() const = 0;
    
    /**
     * 获取图形队列句柄
     */
    virtual QueueHandle GetGraphicsQueue() const = 0;
    
    /**
     * 获取渲染通道句柄
     */
    virtual RenderPassHandle GetRenderPass() const = 0;
    
    /**
     * 获取交换链尺寸
     */
    virtual Extent2D GetSwapchainExtent() const = 0;
    
    /**
     * 查找内存类型（用于缓冲区分配）
     * 
     * @param typeFilter 内存类型过滤器（位掩码）
     * @param properties 所需的内存属性标志
     * @return 匹配的内存类型索引，如果未找到则返回 UINT32_MAX
     * @note 调用者必须检查返回值，UINT32_MAX 表示查找失败
     */
    virtual uint32_t FindMemoryType(uint32_t typeFilter, MemoryPropertyFlag properties) const = 0;
};


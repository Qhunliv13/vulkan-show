#pragma once

#include <cstdint>
#include "core/types/render_types.h"

// 前向声明（避免包含 vulkan.h）
// 注意：如果文件已经包含了 vulkan.h，这些前向声明会被忽略
struct VkDevice_T;
struct VkPhysicalDevice_T;
struct VkCommandPool_T;
struct VkQueue_T;
struct VkRenderPass_T;
struct VkExtent2D;

typedef VkDevice_T* VkDevice;
typedef VkPhysicalDevice_T* VkPhysicalDevice;
typedef VkCommandPool_T* VkCommandPool;
typedef VkQueue_T* VkQueue;
typedef VkRenderPass_T* VkRenderPass;

// 渲染上下文接口 - 抽象层，用于解耦UI组件与底层渲染API
// 不依赖具体渲染后端类型
class IRenderContext {
public:
    virtual ~IRenderContext() = default;
    
    // 获取设备对象（使用抽象句柄）
    virtual DeviceHandle GetDevice() const = 0;
    virtual PhysicalDeviceHandle GetPhysicalDevice() const = 0;
    virtual CommandPoolHandle GetCommandPool() const = 0;
    virtual QueueHandle GetGraphicsQueue() const = 0;
    virtual RenderPassHandle GetRenderPass() const = 0;
    virtual Extent2D GetSwapchainExtent() const = 0;
    
    // 查找内存类型（用于缓冲区分配）
    virtual uint32_t FindMemoryType(uint32_t typeFilter, MemoryPropertyFlag properties) const = 0;
};

// 工厂函数：创建 Vulkan 渲染上下文（在 .cpp 中实现）
// 注意：此函数在 .cpp 文件中实现，避免头文件包含 vulkan.h
IRenderContext* CreateVulkanRenderContext(VkDevice device, 
                                          VkPhysicalDevice physicalDevice,
                                          VkCommandPool commandPool,
                                          VkQueue graphicsQueue,
                                          VkRenderPass renderPass,
                                          VkExtent2D swapchainExtent);


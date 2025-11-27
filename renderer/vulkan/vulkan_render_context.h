#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>  // 3. 第三方库头文件

#include "core/interfaces/irender_context.h"  // 4. 项目头文件（接口）
#include "core/types/render_types.h"  // 4. 项目头文件（类型）

/**
 * Vulkan 渲染上下文实现类
 * 实现 IRenderContext 接口，将 Vulkan 类型转换为抽象句柄类型
 * 
 * 此实现类将 Vulkan 依赖隔离在 vulkan/ 目录，保持接口层的平台无关性
 */
class VulkanRenderContext : public IRenderContext {
public:
    VulkanRenderContext(VkDevice device, 
                       VkPhysicalDevice physicalDevice,
                       VkCommandPool commandPool,
                       VkQueue graphicsQueue,
                       VkRenderPass renderPass,
                       VkExtent2D swapchainExtent)
        : m_device(device)
        , m_physicalDevice(physicalDevice)
        , m_commandPool(commandPool)
        , m_graphicsQueue(graphicsQueue)
        , m_renderPass(renderPass)
        , m_swapchainExtent(swapchainExtent) {}
    
    virtual ~VulkanRenderContext() = default;
    
    DeviceHandle GetDevice() const override { return static_cast<DeviceHandle>(m_device); }
    PhysicalDeviceHandle GetPhysicalDevice() const override { return static_cast<PhysicalDeviceHandle>(m_physicalDevice); }
    CommandPoolHandle GetCommandPool() const override { return static_cast<CommandPoolHandle>(m_commandPool); }
    QueueHandle GetGraphicsQueue() const override { return static_cast<QueueHandle>(m_graphicsQueue); }
    RenderPassHandle GetRenderPass() const override { return static_cast<RenderPassHandle>(m_renderPass); }
    Extent2D GetSwapchainExtent() const override { 
        return Extent2D(m_swapchainExtent.width, m_swapchainExtent.height); 
    }
    
    uint32_t FindMemoryType(uint32_t typeFilter, MemoryPropertyFlag properties) const override;
    
private:
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkCommandPool m_commandPool;
    VkQueue m_graphicsQueue;
    VkRenderPass m_renderPass;
    VkExtent2D m_swapchainExtent;
};


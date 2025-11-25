#pragma once

#include <vulkan/vulkan.h>
#include <cstdint>

// 渲染上下文接口 - 抽象层，用于解耦UI组件与底层渲染API
class IRenderContext {
public:
    virtual ~IRenderContext() = default;
    
    // 获取Vulkan设备对象
    virtual VkDevice GetDevice() const = 0;
    virtual VkPhysicalDevice GetPhysicalDevice() const = 0;
    virtual VkCommandPool GetCommandPool() const = 0;
    virtual VkQueue GetGraphicsQueue() const = 0;
    virtual VkRenderPass GetRenderPass() const = 0;
    virtual VkExtent2D GetSwapchainExtent() const = 0;
    
    // 查找内存类型（用于缓冲区分配）
    virtual uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const = 0;
};

// Vulkan渲染上下文实现
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
    
    VkDevice GetDevice() const override { return m_device; }
    VkPhysicalDevice GetPhysicalDevice() const override { return m_physicalDevice; }
    VkCommandPool GetCommandPool() const override { return m_commandPool; }
    VkQueue GetGraphicsQueue() const override { return m_graphicsQueue; }
    VkRenderPass GetRenderPass() const override { return m_renderPass; }
    VkExtent2D GetSwapchainExtent() const override { return m_swapchainExtent; }
    
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const override {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
        
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && 
                (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }
        
        return 0;
    }
    
private:
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkCommandPool m_commandPool;
    VkQueue m_graphicsQueue;
    VkRenderPass m_renderPass;
    VkExtent2D m_swapchainExtent;
};


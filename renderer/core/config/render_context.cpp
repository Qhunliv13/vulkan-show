#include "core/config/render_context.h"

// VulkanRenderContext 实现（在 .cpp 文件中包含 vulkan.h，避免头文件暴露）
#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>

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
    
    DeviceHandle GetDevice() const override { return static_cast<DeviceHandle>(m_device); }
    PhysicalDeviceHandle GetPhysicalDevice() const override { return static_cast<PhysicalDeviceHandle>(m_physicalDevice); }
    CommandPoolHandle GetCommandPool() const override { return static_cast<CommandPoolHandle>(m_commandPool); }
    QueueHandle GetGraphicsQueue() const override { return static_cast<QueueHandle>(m_graphicsQueue); }
    RenderPassHandle GetRenderPass() const override { return static_cast<RenderPassHandle>(m_renderPass); }
    Extent2D GetSwapchainExtent() const override { 
        return Extent2D(m_swapchainExtent.width, m_swapchainExtent.height); 
    }
    
    uint32_t FindMemoryType(uint32_t typeFilter, MemoryPropertyFlag properties) const override {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
        
        // 转换 MemoryPropertyFlag 到 VkMemoryPropertyFlags
        VkMemoryPropertyFlags vkProperties = 0;
        if ((properties & MemoryPropertyFlag::DeviceLocal) != MemoryPropertyFlag::None) {
            vkProperties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        }
        if ((properties & MemoryPropertyFlag::HostVisible) != MemoryPropertyFlag::None) {
            vkProperties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        }
        if ((properties & MemoryPropertyFlag::HostCoherent) != MemoryPropertyFlag::None) {
            vkProperties |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }
        if ((properties & MemoryPropertyFlag::HostCached) != MemoryPropertyFlag::None) {
            vkProperties |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        }
        
        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && 
                (memProperties.memoryTypes[i].propertyFlags & vkProperties) == vkProperties) {
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

// 工厂函数（在头文件中声明，在 .cpp 中实现）
IRenderContext* CreateVulkanRenderContext(VkDevice device, 
                                          VkPhysicalDevice physicalDevice,
                                          VkCommandPool commandPool,
                                          VkQueue graphicsQueue,
                                          VkRenderPass renderPass,
                                          VkExtent2D swapchainExtent) {
    return new VulkanRenderContext(device, physicalDevice, commandPool, 
                                   graphicsQueue, renderPass, swapchainExtent);
}


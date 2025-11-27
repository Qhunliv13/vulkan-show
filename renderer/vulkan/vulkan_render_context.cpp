#include "renderer/vulkan/vulkan_render_context.h"  // 1. 对应头文件

#include "core/types/render_types.h"  // 4. 项目头文件（类型）

#include <vulkan/vulkan.h>  // 3. 第三方库头文件（用于VkPhysicalDeviceMemoryProperties等）

/**
 * VulkanRenderContext 实现（在 .cpp 文件中实现 FindMemoryType 方法）
 * 此设计将 Vulkan 依赖隔离在实现文件中，保持接口头文件的平台无关性
 */
uint32_t VulkanRenderContext::FindMemoryType(uint32_t typeFilter, MemoryPropertyFlag properties) const {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    
    // 转换抽象内存属性标志到 Vulkan 具体标志
    // 此转换层允许接口层使用平台无关的枚举，而实现层使用 Vulkan 特定类型
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


#include "renderer/vulkan/vulkan_render_context_factory.h"  // 1. 对应头文件

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.h>  // 3. 第三方库头文件

#include "core/types/render_types.h"  // 4. 项目头文件（类型）
#include "renderer/vulkan/vulkan_render_context.h"  // 4. 项目头文件（实现）

/**
 * Vulkan 渲染上下文工厂实现
 * 将抽象类型转换为 Vulkan 类型，创建 VulkanRenderContext 实例
 */
std::unique_ptr<IRenderContext> VulkanRenderContextFactory::CreateRenderContext(
    DeviceHandle device,
    PhysicalDeviceHandle physicalDevice,
    CommandPoolHandle commandPool,
    QueueHandle graphicsQueue,
    RenderPassHandle renderPass,
    Extent2D swapchainExtent) {
    // 将抽象类型转换为 Vulkan 类型（工厂函数内部进行转换，隐藏实现细节）
    return std::make_unique<VulkanRenderContext>(
        static_cast<VkDevice>(device),
        static_cast<VkPhysicalDevice>(physicalDevice),
        static_cast<VkCommandPool>(commandPool),
        static_cast<VkQueue>(graphicsQueue),
        static_cast<VkRenderPass>(renderPass),
        VkExtent2D{ swapchainExtent.width, swapchainExtent.height }
    );
}

/**
 * 向后兼容：独立的工厂函数实现
 * 内部使用 VulkanRenderContextFactory 实例
 */
std::unique_ptr<IRenderContext> CreateVulkanRenderContext(
    DeviceHandle device,
    PhysicalDeviceHandle physicalDevice,
    CommandPoolHandle commandPool,
    QueueHandle graphicsQueue,
    RenderPassHandle renderPass,
    Extent2D swapchainExtent) {
    static VulkanRenderContextFactory factory;
    return factory.CreateRenderContext(
        device, physicalDevice, commandPool, graphicsQueue, renderPass, swapchainExtent
    );
}


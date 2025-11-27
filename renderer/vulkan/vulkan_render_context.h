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
    /**
     * 构造函数
     * 创建Vulkan渲染上下文，将Vulkan类型转换为抽象句柄类型
     * 
     * @param device Vulkan设备句柄
     * @param physicalDevice Vulkan物理设备句柄
     * @param commandPool Vulkan命令池句柄
     * @param graphicsQueue Vulkan图形队列句柄
     * @param renderPass Vulkan渲染通道句柄
     * @param swapchainExtent Vulkan交换链尺寸
     */
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
    
    /**
     * 获取设备句柄
     * 将Vulkan设备句柄转换为抽象类型
     * 
     * @return 设备句柄（抽象类型）
     */
    DeviceHandle GetDevice() const override { return static_cast<DeviceHandle>(m_device); }
    
    /**
     * 获取物理设备句柄
     * 将Vulkan物理设备句柄转换为抽象类型
     * 
     * @return 物理设备句柄（抽象类型）
     */
    PhysicalDeviceHandle GetPhysicalDevice() const override { return static_cast<PhysicalDeviceHandle>(m_physicalDevice); }
    
    /**
     * 获取命令池句柄
     * 将Vulkan命令池句柄转换为抽象类型
     * 
     * @return 命令池句柄（抽象类型）
     */
    CommandPoolHandle GetCommandPool() const override { return static_cast<CommandPoolHandle>(m_commandPool); }
    
    /**
     * 获取图形队列句柄
     * 将Vulkan图形队列句柄转换为抽象类型
     * 
     * @return 图形队列句柄（抽象类型）
     */
    QueueHandle GetGraphicsQueue() const override { return static_cast<QueueHandle>(m_graphicsQueue); }
    
    /**
     * 获取渲染通道句柄
     * 将Vulkan渲染通道句柄转换为抽象类型
     * 
     * @return 渲染通道句柄（抽象类型）
     */
    RenderPassHandle GetRenderPass() const override { return static_cast<RenderPassHandle>(m_renderPass); }
    
    /**
     * 获取交换链尺寸
     * 将Vulkan交换链尺寸转换为抽象类型
     * 
     * @return 交换链尺寸（Extent2D）
     */
    Extent2D GetSwapchainExtent() const override { 
        return Extent2D(m_swapchainExtent.width, m_swapchainExtent.height); 
    }
    
    /**
     * 查找内存类型
     * 根据类型过滤器和内存属性标志查找合适的内存类型
     * 
     * @param typeFilter 内存类型过滤器（位掩码）
     * @param properties 内存属性标志（DeviceLocal/HostVisible/HostCoherent/HostCached）
     * @return 内存类型索引，未找到返回 UINT32_MAX
     */
    uint32_t FindMemoryType(uint32_t typeFilter, MemoryPropertyFlag properties) const override;
    
private:
    VkDevice m_device;
    VkPhysicalDevice m_physicalDevice;
    VkCommandPool m_commandPool;
    VkQueue m_graphicsQueue;
    VkRenderPass m_renderPass;
    VkExtent2D m_swapchainExtent;
};


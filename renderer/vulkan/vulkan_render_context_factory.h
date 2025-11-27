#pragma once

#include <memory>  // 2. 系统头文件
#include "core/interfaces/irender_context_factory.h"  // 4. 项目头文件（接口）
#include "core/types/render_types.h"  // 4. 项目头文件（类型）

// 前向声明
class VulkanRenderContext;

/**
 * Vulkan 渲染上下文工厂实现
 * 
 * 实现 IRenderContextFactory 接口，创建 VulkanRenderContext 实例
 * 符合依赖倒置原则，允许通过接口替换不同实现
 */
class VulkanRenderContextFactory : public IRenderContextFactory {
public:
    virtual ~VulkanRenderContextFactory() = default;
    
    /**
     * 创建 Vulkan 渲染上下文实例
     * 
     * @param device 设备句柄（抽象类型）
     * @param physicalDevice 物理设备句柄（抽象类型）
     * @param commandPool 命令池句柄（抽象类型）
     * @param graphicsQueue 图形队列句柄（抽象类型）
     * @param renderPass 渲染通道句柄（抽象类型）
     * @param swapchainExtent 交换链尺寸（抽象类型）
     * @return std::unique_ptr<IRenderContext> 渲染上下文接口指针，调用者获得所有权
     */
    std::unique_ptr<IRenderContext> CreateRenderContext(
        DeviceHandle device,
        PhysicalDeviceHandle physicalDevice,
        CommandPoolHandle commandPool,
        QueueHandle graphicsQueue,
        RenderPassHandle renderPass,
        Extent2D swapchainExtent) override;
};

/**
 * 向后兼容：独立的工厂函数
 * 现有代码可以使用此函数，内部使用 VulkanRenderContextFactory 实现
 * 建议新代码直接使用 VulkanRenderContextFactory 类，支持依赖注入
 * 
 * @param device 设备句柄（抽象类型）
 * @param physicalDevice 物理设备句柄（抽象类型）
 * @param commandPool 命令池句柄（抽象类型）
 * @param graphicsQueue 图形队列句柄（抽象类型）
 * @param renderPass 渲染通道句柄（抽象类型）
 * @param swapchainExtent 交换链尺寸（抽象类型）
 * @return std::unique_ptr<IRenderContext> 渲染上下文接口指针，调用者获得所有权
 */
std::unique_ptr<IRenderContext> CreateVulkanRenderContext(
    DeviceHandle device,
    PhysicalDeviceHandle physicalDevice,
    CommandPoolHandle commandPool,
    QueueHandle graphicsQueue,
    RenderPassHandle renderPass,
    Extent2D swapchainExtent);

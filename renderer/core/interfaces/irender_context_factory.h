#pragma once

#include <memory>  // 2. 系统头文件
#include "core/interfaces/irender_context.h"  // 4. 项目头文件（接口）
#include "core/types/render_types.h"  // 4. 项目头文件（类型）

/**
 * 渲染上下文工厂接口
 * 
 * 提供统一的工厂接口，允许通过依赖注入替换不同的渲染后端实现
 * 符合依赖倒置原则，解耦调用方与具体实现
 */
class IRenderContextFactory {
public:
    virtual ~IRenderContextFactory() = default;
    
    /**
     * 创建渲染上下文实例
     * 
     * @param device 设备句柄（抽象类型）
     * @param physicalDevice 物理设备句柄（抽象类型）
     * @param commandPool 命令池句柄（抽象类型）
     * @param graphicsQueue 图形队列句柄（抽象类型）
     * @param renderPass 渲染通道句柄（抽象类型）
     * @param swapchainExtent 交换链尺寸（抽象类型）
     * @return std::unique_ptr<IRenderContext> 渲染上下文接口指针，调用者获得所有权
     */
    virtual std::unique_ptr<IRenderContext> CreateRenderContext(
        DeviceHandle device,
        PhysicalDeviceHandle physicalDevice,
        CommandPoolHandle commandPool,
        QueueHandle graphicsQueue,
        RenderPassHandle renderPass,
        Extent2D swapchainExtent) = 0;
};


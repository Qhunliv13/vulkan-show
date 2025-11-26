#include "core/interfaces/irenderer.h"  // 1. 对应头文件

#include "core/interfaces/ipipeline_manager.h"  // 4. 项目头文件（接口）
#include "core/interfaces/icamera_controller.h"  // 4. 项目头文件（接口）
#include "core/interfaces/irender_device.h"  // 4. 项目头文件（接口）

/**
 * 便捷方法的默认实现（通过组合访问子接口）
 * 
 * 这些方法使用组合模式，通过 GetRenderDevice() 等方法访问子接口
 * 提供向后兼容的便捷访问方式，避免直接访问子接口
 */
Extent2D IRenderer::GetSwapchainExtent() const {
    IRenderDevice* device = const_cast<IRenderer*>(this)->GetRenderDevice();
    return device ? device->GetSwapchainExtent() : Extent2D{0, 0};
}

DeviceHandle IRenderer::GetDevice() const {
    IRenderDevice* device = const_cast<IRenderer*>(this)->GetRenderDevice();
    return device ? device->GetDevice() : nullptr;
}

PhysicalDeviceHandle IRenderer::GetPhysicalDevice() const {
    IRenderDevice* device = const_cast<IRenderer*>(this)->GetRenderDevice();
    return device ? device->GetPhysicalDevice() : nullptr;
}

CommandPoolHandle IRenderer::GetCommandPool() const {
    IRenderDevice* device = const_cast<IRenderer*>(this)->GetRenderDevice();
    return device ? device->GetCommandPool() : nullptr;
}

QueueHandle IRenderer::GetGraphicsQueue() const {
    IRenderDevice* device = const_cast<IRenderer*>(this)->GetRenderDevice();
    return device ? device->GetGraphicsQueue() : nullptr;
}

RenderPassHandle IRenderer::GetRenderPass() const {
    IRenderDevice* device = const_cast<IRenderer*>(this)->GetRenderDevice();
    return device ? device->GetRenderPass() : nullptr;
}

ImageFormat IRenderer::GetSwapchainFormat() const {
    IRenderDevice* device = const_cast<IRenderer*>(this)->GetRenderDevice();
    return device ? device->GetSwapchainFormat() : static_cast<ImageFormat>(0);
}

uint32_t IRenderer::GetSwapchainImageCount() const {
    IRenderDevice* device = const_cast<IRenderer*>(this)->GetRenderDevice();
    return device ? device->GetSwapchainImageCount() : 0;
}


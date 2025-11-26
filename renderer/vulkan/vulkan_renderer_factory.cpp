#include "vulkan/vulkan_renderer_factory.h"  // 1. 对应头文件

#include <memory>  // 2. 系统头文件

#include "vulkan/vulkan_renderer.h"  // 4. 项目头文件（实现）

VulkanRendererFactory::VulkanRendererFactory() {
}

VulkanRendererFactory::~VulkanRendererFactory() {
}

std::unique_ptr<IRenderer> VulkanRendererFactory::CreateRenderer() {
    return std::make_unique<VulkanRenderer>();
}


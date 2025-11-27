#include "vulkan/vulkan_renderer_factory.h"  // 1. 对应头文件

#include <memory>  // 2. 系统头文件

#include "vulkan/vulkan_renderer.h"  // 4. 项目头文件（实现）

// 使用默认构造函数，无需特殊初始化
VulkanRendererFactory::VulkanRendererFactory() {
}

// 使用默认析构函数，无需特殊清理
VulkanRendererFactory::~VulkanRendererFactory() {
}

std::unique_ptr<IRenderer> VulkanRendererFactory::CreateRenderer() {
    return std::make_unique<VulkanRenderer>();
}


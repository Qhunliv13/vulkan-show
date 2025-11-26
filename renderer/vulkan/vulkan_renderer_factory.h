#pragma once

#include "core/interfaces/irenderer_factory.h"

// Vulkan 渲染器工厂实现
class VulkanRendererFactory : public IRendererFactory {
public:
    VulkanRendererFactory();
    ~VulkanRendererFactory();
    
    // IRendererFactory 接口实现
    // 所有权：[TRANSFER] 调用方获得所有权，通过 std::unique_ptr 自动管理生命周期
    std::unique_ptr<IRenderer> CreateRenderer() override;
};


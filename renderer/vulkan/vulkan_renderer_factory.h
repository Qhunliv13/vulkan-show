#pragma once

#include "core/irenderer_factory.h"

// Vulkan 渲染器工厂实现
class VulkanRendererFactory : public IRendererFactory {
public:
    VulkanRendererFactory();
    ~VulkanRendererFactory();
    
    // IRendererFactory 接口实现
    IRenderer* CreateRenderer() override;
    void DestroyRenderer(IRenderer* renderer) override;
};


#include "vulkan/vulkan_renderer_factory.h"
#include "vulkan/vulkan_renderer.h"

VulkanRendererFactory::VulkanRendererFactory() {
}

VulkanRendererFactory::~VulkanRendererFactory() {
}

IRenderer* VulkanRendererFactory::CreateRenderer() {
    return new VulkanRenderer();
}

void VulkanRendererFactory::DestroyRenderer(IRenderer* renderer) {
    if (renderer) {
        renderer->Cleanup();
        delete renderer;
    }
}


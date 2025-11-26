#pragma once

#include "core/interfaces/irenderer_factory.h"  // 4. 项目头文件（接口）

// Vulkan渲染器工厂 - 创建VulkanRenderer实例，实现IRendererFactory接口
// 通过依赖注入提供渲染器实例，支持测试和替换实现
class VulkanRendererFactory : public IRendererFactory {
public:
    VulkanRendererFactory();
    ~VulkanRendererFactory();
    
    // IRendererFactory 接口实现
    // 返回值：调用方获得所有权，通过 std::unique_ptr 自动管理生命周期
    std::unique_ptr<IRenderer> CreateRenderer() override;
};


#pragma once

#include <memory>  // 2. 系统头文件
#include "core/interfaces/irenderer.h"  // 4. 项目头文件（接口）

/**
 * 渲染器工厂接口 - 用于创建渲染器实例，实现依赖倒置
 * 
 * 职责：提供渲染器创建接口，支持可替换的渲染器实现
 * 设计：通过工厂模式实现依赖倒置，解耦创建逻辑与使用逻辑
 * 
 * 使用方式：
 * 1. 实现此接口创建具体的渲染器（如 VulkanRendererFactory）
 * 2. 通过依赖注入传入工厂实例
 * 3. 使用 CreateRenderer() 创建渲染器实例
 */
class IRendererFactory {
public:
    virtual ~IRendererFactory() = default;
    
    /**
     * 创建渲染器实例
     * 
     * 所有权：[TRANSFER] 调用方获得所有权，通过 std::unique_ptr 自动管理生命周期
     * @return std::unique_ptr<IRenderer> 渲染器实例，调用方获得所有权
     */
    [[ownership("transfer")]]
    virtual std::unique_ptr<IRenderer> CreateRenderer() = 0;
};


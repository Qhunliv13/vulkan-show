#pragma once

#include <memory>
#include "core/interfaces/irenderer.h"

// 渲染器工厂接口 - 用于创建渲染器实例，实现依赖倒置
class IRendererFactory {
public:
    virtual ~IRendererFactory() = default;
    
    // 创建渲染器实例
    // 所有权：[TRANSFER] 调用方获得所有权，通过 std::unique_ptr 自动管理生命周期
    virtual std::unique_ptr<IRenderer> CreateRenderer() = 0;
};


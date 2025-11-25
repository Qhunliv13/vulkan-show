#pragma once

#include "core/interfaces/irenderer.h"

// 渲染器工厂接口 - 用于创建渲染器实例，实现依赖倒置
class IRendererFactory {
public:
    virtual ~IRendererFactory() = default;
    
    // 创建渲染器实例
    virtual IRenderer* CreateRenderer() = 0;
    
    // 销毁渲染器实例
    virtual void DestroyRenderer(IRenderer* renderer) = 0;
};


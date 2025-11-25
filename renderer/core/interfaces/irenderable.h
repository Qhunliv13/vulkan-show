#pragma once

// 可渲染对象接口 - 用于统一渲染接口
class IRenderable {
public:
    virtual ~IRenderable() = default;
    
    // 渲染对象
    virtual void Render(float time, float deltaTime, float fps) = 0;
    
    // 检查是否应该渲染
    virtual bool ShouldRender() const = 0;
};


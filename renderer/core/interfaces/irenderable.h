#pragma once

/**
 * 可渲染对象接口 - 用于统一渲染接口
 * 
 * 职责：提供统一的渲染接口，支持多种可渲染对象
 * 设计：通过接口抽象，支持场景对象、UI对象等多种可渲染实体
 * 
 * 使用方式：
 * 1. 实现此接口创建可渲染对象
 * 2. 使用 Render() 方法渲染对象
 * 3. 使用 ShouldRender() 检查是否应该渲染
 */
class IRenderable {
public:
    virtual ~IRenderable() = default;
    
    // 渲染对象
    virtual void Render(float time, float deltaTime, float fps) = 0;
    
    // 检查是否应该渲染
    virtual bool ShouldRender() const = 0;
};


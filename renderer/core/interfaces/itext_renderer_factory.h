#pragma once

// 前向声明
class ITextRenderer;

/**
 * 文字渲染器工厂接口 - 用于依赖注入，支持可替换的文字渲染实现
 * 
 * 职责：提供文字渲染器创建接口，支持可替换的文字渲染实现
 * 设计：通过工厂模式实现依赖倒置，解耦创建逻辑与使用逻辑
 * 
 * 使用方式：
 * 1. 实现此接口创建具体的文字渲染器（如 TextRendererFactory）
 * 2. 通过依赖注入传入工厂实例
 * 3. 使用 CreateTextRenderer() 创建文字渲染器实例
 * 4. 注意：当前使用裸指针返回，未来考虑改用 std::unique_ptr 或句柄模式
 */
class ITextRendererFactory {
public:
    virtual ~ITextRendererFactory() = default;
    
    // 创建文字渲染器实例
    // 所有权：[TRANSFER] 调用方获得所有权，负责通过 DestroyTextRenderer() 销毁
    // 建议：未来考虑改用 std::unique_ptr 或句柄模式，避免裸指针
    [[ownership("transfer")]]
    virtual ITextRenderer* CreateTextRenderer() = 0;
    
    // 销毁文字渲染器实例
    virtual void DestroyTextRenderer(ITextRenderer* renderer) = 0;
};


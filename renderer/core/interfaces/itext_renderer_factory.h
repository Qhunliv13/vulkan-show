#pragma once

// 前向声明
class ITextRenderer;

// 文字渲染器工厂接口 - 用于依赖注入，支持可替换的文字渲染实现
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


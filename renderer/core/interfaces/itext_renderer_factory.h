#pragma once

// 前向声明
class ITextRenderer;

// 文字渲染器工厂接口 - 用于依赖注入，支持可替换的文字渲染实现
class ITextRendererFactory {
public:
    virtual ~ITextRendererFactory() = default;
    
    // 创建文字渲染器实例
    virtual ITextRenderer* CreateTextRenderer() = 0;
    
    // 销毁文字渲染器实例
    virtual void DestroyTextRenderer(ITextRenderer* renderer) = 0;
};


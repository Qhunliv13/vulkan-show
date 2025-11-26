#pragma once

#include "core/interfaces/itext_renderer_factory.h"

// 默认文字渲染器工厂实现
class TextRendererFactory : public ITextRendererFactory {
public:
    TextRendererFactory() = default;
    ~TextRendererFactory() = default;
    
    ITextRenderer* CreateTextRenderer() override;
    void DestroyTextRenderer(ITextRenderer* renderer) override;
};


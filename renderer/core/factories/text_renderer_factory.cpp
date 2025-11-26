#include "core/factories/text_renderer_factory.h"
#include "text/text_renderer.h"

ITextRenderer* TextRendererFactory::CreateTextRenderer() {
    return new TextRenderer();
}

void TextRendererFactory::DestroyTextRenderer(ITextRenderer* renderer) {
    if (renderer) {
        renderer->Cleanup();
        delete renderer;
    }
}


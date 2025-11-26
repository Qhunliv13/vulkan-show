#include "core/factories/text_renderer_factory.h"  // 1. 对应头文件

#include "text/text_renderer.h"  // 4. 项目头文件

/**
 * 创建文字渲染器实例
 * 
 * 使用 new 创建 TextRenderer 对象，调用方负责通过 DestroyTextRenderer() 销毁
 * 所有权：[TRANSFER] 调用方获得所有权
 * 
 * @return ITextRenderer* 文字渲染器接口指针，调用方获得所有权
 */
ITextRenderer* TextRendererFactory::CreateTextRenderer() {
    return new TextRenderer();
}

/**
 * 销毁文字渲染器实例
 * 
 * 先调用 Cleanup() 方法清理资源，然后释放内存
 * 必须与 CreateTextRenderer() 配对使用，确保资源正确释放
 * 
 * @param renderer 文字渲染器指针（拥有所有权，将被销毁）
 */
void TextRendererFactory::DestroyTextRenderer(ITextRenderer* renderer) {
    if (renderer) {
        renderer->Cleanup();
        delete renderer;
    }
}


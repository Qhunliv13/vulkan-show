#include "core/factories/text_renderer_factory.h"  // 1. 对应头文件

#include "text/text_renderer.h"  // 4. 项目头文件

/**
 * 创建文字渲染器实例
 * 
 * 使用 std::make_unique 创建 TextRenderer 对象，确保异常安全
 * 返回的 unique_ptr 自动管理文字渲染器生命周期
 * 
 * @return std::unique_ptr<ITextRenderer> 文字渲染器实例，调用方获得所有权
 */
std::unique_ptr<ITextRenderer> TextRendererFactory::CreateTextRenderer() {
    return std::make_unique<TextRenderer>();
}


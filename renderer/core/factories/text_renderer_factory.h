#pragma once

#include <memory>  // 2. 系统头文件

#include "core/interfaces/itext_renderer_factory.h"  // 4. 项目头文件（接口）

/**
 * 默认文字渲染器工厂实现
 * 
 * 实现 ITextRendererFactory 接口，提供文字渲染器创建功能
 * 使用 std::unique_ptr 管理文字渲染器生命周期，确保自动内存管理
 */
class TextRendererFactory : public ITextRendererFactory {
public:
    TextRendererFactory() = default;
    ~TextRendererFactory() = default;
    
    /**
     * 创建文字渲染器实例
     * 
     * 所有权：[TRANSFER] 调用方通过 std::unique_ptr 获得所有权，自动管理内存
     * @return std::unique_ptr<ITextRenderer> 文字渲染器实例，调用方获得所有权
     */
    std::unique_ptr<ITextRenderer> CreateTextRenderer() override;
};


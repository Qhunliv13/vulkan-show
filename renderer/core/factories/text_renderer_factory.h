#pragma once

#include "core/interfaces/itext_renderer_factory.h"  // 4. 项目头文件（接口）

/**
 * 默认文字渲染器工厂实现
 * 
 * 实现 ITextRendererFactory 接口，提供文字渲染器创建功能
 * 注意：当前使用裸指针返回，未来考虑改用 std::unique_ptr 或句柄模式
 */
class TextRendererFactory : public ITextRendererFactory {
public:
    TextRendererFactory() = default;
    ~TextRendererFactory() = default;
    
    /**
     * 创建文字渲染器实例
     * 
     * 所有权：[TRANSFER] 调用方获得所有权，必须通过 DestroyTextRenderer() 销毁
     * 建议：使用 std::unique_ptr 包装返回值以确保自动内存管理
     * 
     * @return ITextRenderer* 文字渲染器接口指针，调用方获得所有权
     */
    ITextRenderer* CreateTextRenderer() override;
    
    /**
     * 销毁文字渲染器实例
     * 
     * 调用清理方法并释放内存，必须与 CreateTextRenderer() 配对使用
     * 
     * @param renderer 文字渲染器指针（拥有所有权，将被销毁）
     */
    void DestroyTextRenderer(ITextRenderer* renderer) override;
};


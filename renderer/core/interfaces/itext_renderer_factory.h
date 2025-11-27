#pragma once

#include <memory>  // 2. 系统头文件

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
 * 4. 通过 std::unique_ptr 自动管理内存，无需手动销毁
 */
class ITextRendererFactory {
public:
    virtual ~ITextRendererFactory() = default;
    
    /**
     * 创建文字渲染器实例
     * 
     * 所有权：[TRANSFER] 调用方通过 std::unique_ptr 获得所有权，自动管理内存
     * @return std::unique_ptr<ITextRenderer> 文字渲染器实例，调用方获得所有权
     */
    virtual std::unique_ptr<ITextRenderer> CreateTextRenderer() = 0;
};


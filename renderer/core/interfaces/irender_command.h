#pragma once

#include <memory>  // 2. 系统头文件
#include "core/types/render_types.h"  // 4. 项目头文件（类型）

/**
 * 渲染命令接口 - 抽象渲染操作，支持命令缓冲和延迟执行
 * 
 * 职责：提供统一的渲染命令接口，支持命令缓冲和延迟执行
 * 设计：通过命令模式，将渲染操作封装为命令对象，支持批处理和优化
 * 
 * 使用方式：
 * 1. 实现此接口创建具体的渲染命令
 * 2. 通过 IRenderCommandBuffer::AddCommand() 添加命令
 * 3. 通过 IRenderCommandBuffer::ExecuteAll() 统一执行所有命令
 */
class IRenderCommand {
public:
    virtual ~IRenderCommand() = default;
    
    // 执行渲染命令（使用抽象命令缓冲区句柄）
    virtual void Execute(CommandBufferHandle commandBuffer) = 0;
    
    // 获取命令类型（用于优化和批处理）
    virtual uint32_t GetCommandType() const = 0;
};

/**
 * 渲染命令类型枚举
 * 
 * 定义不同类型的渲染命令，用于优化和批处理
 */
enum class RenderCommandType {
    DrawPrimitive = 1,
    DrawUI = 2,
    DrawBackground = 3,
    DrawText = 4,
    Custom = 100
};

/**
 * 渲染命令缓冲区接口 - 管理渲染命令的缓冲和执行
 * 
 * 职责：管理渲染命令的缓冲、批处理和统一执行
 * 设计：通过命令缓冲区模式，支持延迟执行和命令优化
 * 
 * 使用方式：
 * 1. 通过 IRenderer::GetCommandBuffer() 获取接口指针
 * 2. 使用 AddCommand() 添加渲染命令
 * 3. 使用 ExecuteAll() 统一执行所有命令
 */
class IRenderCommandBuffer {
public:
    virtual ~IRenderCommandBuffer() = default;
    
    // 添加渲染命令
    virtual void AddCommand(std::shared_ptr<IRenderCommand> command) = 0;
    
    // 清空命令缓冲区
    virtual void Clear() = 0;
    
    // 执行所有命令（立即执行）
    virtual void ExecuteAll(CommandBufferHandle commandBuffer) = 0;
    
    // 获取命令数量
    virtual size_t GetCommandCount() const = 0;
    
    // 检查是否为空
    virtual bool IsEmpty() const = 0;
};


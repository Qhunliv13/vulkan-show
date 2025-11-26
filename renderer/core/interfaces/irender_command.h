#pragma once

#include <memory>
#include "core/types/render_types.h"

// 渲染命令接口 - 抽象渲染操作，支持命令缓冲和延迟执行
class IRenderCommand {
public:
    virtual ~IRenderCommand() = default;
    
    // 执行渲染命令（使用抽象命令缓冲区句柄）
    virtual void Execute(CommandBufferHandle commandBuffer) = 0;
    
    // 获取命令类型（用于优化和批处理）
    virtual uint32_t GetCommandType() const = 0;
};

// 渲染命令类型枚举
enum class RenderCommandType {
    DrawPrimitive = 1,
    DrawUI = 2,
    DrawBackground = 3,
    DrawText = 4,
    Custom = 100
};

// 渲染命令缓冲区接口 - 管理渲染命令的缓冲和执行
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


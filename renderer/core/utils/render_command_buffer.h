#pragma once

#include <memory>        // 2. 系统头文件
#include <vector>        // 2. 系统头文件

#include "core/interfaces/irender_command.h"  // 4. 项目头文件

/**
 * 渲染命令缓冲区 - 实现 IRenderCommandBuffer 接口，管理渲染命令的添加和执行
 * 
 * 职责：管理渲染命令的缓冲、批处理和统一执行，支持延迟渲染
 * 设计：通过命令模式，将渲染操作封装为命令对象，支持批处理和优化
 * 
 * 使用方式：
 * 1. 通过依赖注入获取接口指针
 * 2. 使用 AddCommand() 添加渲染命令
 * 3. 使用 ExecuteAll() 统一执行所有命令
 * 
 * 注意：vulkan.h 在 .cpp 文件中包含，避免头文件暴露 Vulkan 类型
 */
class RenderCommandBuffer : public IRenderCommandBuffer {
public:
    RenderCommandBuffer();
    ~RenderCommandBuffer();
    
    /**
     * 初始化命令缓冲区
     * 初始化内部状态，准备使用
     */
    void Initialize();
    
    /**
     * 清理资源
     * 清除所有命令，准备销毁
     */
    void Cleanup();
    
    // IRenderCommandBuffer 接口实现
    void AddCommand(std::shared_ptr<IRenderCommand> command) override;
    void Clear() override;
    void ExecuteAll(CommandBufferHandle commandBuffer) override;
    size_t GetCommandCount() const override;
    bool IsEmpty() const override;

private:
    std::vector<std::shared_ptr<IRenderCommand>> m_commands;
    bool m_initialized = false;
};


#pragma once

#include "core/interfaces/irender_command.h"
#include <vector>
#include <memory>
// 注意：vulkan.h 在 .cpp 文件中包含，避免头文件暴露 Vulkan 类型

// 渲染命令缓冲区的简单实现
class RenderCommandBuffer : public IRenderCommandBuffer {
public:
    RenderCommandBuffer();
    ~RenderCommandBuffer();
    
    // IRenderCommandBuffer 接口实现
    void AddCommand(std::shared_ptr<IRenderCommand> command) override;
    void Clear() override;
    void ExecuteAll(CommandBufferHandle commandBuffer) override;
    size_t GetCommandCount() const override;
    bool IsEmpty() const override;

private:
    std::vector<std::shared_ptr<IRenderCommand>> m_commands;
};


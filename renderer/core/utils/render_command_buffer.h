#pragma once

#include <memory>        // 2. 系统头文件
#include <vector>        // 2. 系统头文件

#include "core/interfaces/irender_command.h"  // 4. 项目头文件

// 注意：vulkan.h 在 .cpp 文件中包含，避免头文件暴露 Vulkan 类型

// 渲染命令缓冲区 - 实现 IRenderCommandBuffer 接口，管理渲染命令的添加和执行
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


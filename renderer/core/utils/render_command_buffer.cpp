#include "core/utils/render_command_buffer.h"  // 1. 对应头文件

#include <algorithm>       // 2. 系统头文件

#include "core/interfaces/irender_command.h"   // 4. 项目头文件

RenderCommandBuffer::RenderCommandBuffer() {
}

RenderCommandBuffer::~RenderCommandBuffer() {
    Clear();
}

void RenderCommandBuffer::AddCommand(std::shared_ptr<IRenderCommand> command) {
    if (command) {
        m_commands.push_back(command);
    }
}

void RenderCommandBuffer::Clear() {
    m_commands.clear();
}

void RenderCommandBuffer::ExecuteAll(CommandBufferHandle commandBuffer) {
    // 将抽象句柄传递给命令，命令内部会处理类型转换
    for (auto& command : m_commands) {
        if (command) {
            command->Execute(commandBuffer);
        }
    }
}

size_t RenderCommandBuffer::GetCommandCount() const {
    return m_commands.size();
}

bool RenderCommandBuffer::IsEmpty() const {
    return m_commands.empty();
}


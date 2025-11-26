#pragma once

#include <cstdint>

// 渲染抽象类型 - 用于解耦接口层与具体渲染后端实现
// 这些类型是平台无关的，可以映射到 Vulkan、OpenGL、DirectX 等

// 2D 尺寸结构（替代 VkExtent2D）
struct Extent2D {
    uint32_t width = 0;
    uint32_t height = 0;
    
    Extent2D() = default;
    Extent2D(uint32_t w, uint32_t h) : width(w), height(h) {}
};

// 设备句柄（不透明指针，替代 VkDevice）
using DeviceHandle = void*;

// 物理设备句柄（不透明指针，替代 VkPhysicalDevice）
using PhysicalDeviceHandle = void*;

// 命令池句柄（不透明指针，替代 VkCommandPool）
using CommandPoolHandle = void*;

// 队列句柄（不透明指针，替代 VkQueue）
using QueueHandle = void*;

// 渲染通道句柄（不透明指针，替代 VkRenderPass）
using RenderPassHandle = void*;

// 命令缓冲区句柄（不透明指针，替代 VkCommandBuffer）
using CommandBufferHandle = void*;

// 格式枚举（替代 VkFormat）
enum class ImageFormat {
    Undefined = 0,
    R8G8B8A8Unorm = 37,
    B8G8R8A8Unorm = 44,
    // 可以根据需要添加更多格式
};

// 内存属性标志（替代 VkMemoryPropertyFlags）
enum class MemoryPropertyFlag : uint32_t {
    None = 0,
    DeviceLocal = 1 << 0,
    HostVisible = 1 << 1,
    HostCoherent = 1 << 2,
    HostCached = 1 << 3,
};

inline MemoryPropertyFlag operator|(MemoryPropertyFlag a, MemoryPropertyFlag b) {
    return static_cast<MemoryPropertyFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline MemoryPropertyFlag operator&(MemoryPropertyFlag a, MemoryPropertyFlag b) {
    return static_cast<MemoryPropertyFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}


#pragma once

#include <cstdint>  // 2. 系统头文件

/**
 * 渲染抽象类型 - 用于解耦接口层与具体渲染后端实现
 * 
 * 职责：提供平台无关的渲染类型定义，隐藏具体渲染后端实现细节
 * 设计：这些类型是平台无关的，可以映射到 Vulkan、OpenGL、DirectX 等
 * 
 * 使用方式：
 * 1. 在接口层使用这些抽象类型
 * 2. 在实现层将抽象类型转换为具体后端类型
 */

/**
 * 2D 尺寸结构（替代 VkExtent2D）
 * 
 * 用于表示二维尺寸，支持多种渲染后端
 */
struct Extent2D {
    uint32_t width = 0;
    uint32_t height = 0;
    
    Extent2D() = default;
    Extent2D(uint32_t w, uint32_t h) : width(w), height(h) {}
};

/**
 * 设备句柄（不透明指针，替代 VkDevice）
 * 
 * 使用不透明指针隐藏具体设备类型，支持多种渲染后端
 */
using DeviceHandle = void*;

/**
 * 物理设备句柄（不透明指针，替代 VkPhysicalDevice）
 */
using PhysicalDeviceHandle = void*;

/**
 * 命令池句柄（不透明指针，替代 VkCommandPool）
 */
using CommandPoolHandle = void*;

/**
 * 队列句柄（不透明指针，替代 VkQueue）
 */
using QueueHandle = void*;

/**
 * 渲染通道句柄（不透明指针，替代 VkRenderPass）
 */
using RenderPassHandle = void*;

/**
 * 命令缓冲区句柄（不透明指针，替代 VkCommandBuffer）
 */
using CommandBufferHandle = void*;

/**
 * 图像格式枚举（替代 VkFormat）
 * 
 * 定义支持的图像格式，可以映射到不同渲染后端的格式
 */
enum class ImageFormat {
    Undefined = 0,
    R8G8B8A8Unorm = 37,
    B8G8R8A8Unorm = 44,
    // 可以根据需要添加更多格式
};

/**
 * 内存属性标志（替代 VkMemoryPropertyFlags）
 * 
 * 定义内存属性标志，用于缓冲区分配时的内存类型选择
 */
enum class MemoryPropertyFlag : uint32_t {
    None = 0,
    DeviceLocal = 1 << 0,
    HostVisible = 1 << 1,
    HostCoherent = 1 << 2,
    HostCached = 1 << 3,
};

/**
 * 内存属性标志位或运算符
 * 
 * 用于组合多个内存属性标志
 */
inline MemoryPropertyFlag operator|(MemoryPropertyFlag a, MemoryPropertyFlag b) {
    return static_cast<MemoryPropertyFlag>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/**
 * 内存属性标志位与运算符
 * 
 * 用于检查内存属性标志是否包含特定标志
 */
inline MemoryPropertyFlag operator&(MemoryPropertyFlag a, MemoryPropertyFlag b) {
    return static_cast<MemoryPropertyFlag>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}


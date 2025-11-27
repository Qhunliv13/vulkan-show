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

/**
 * 着色器模块句柄（不透明指针，替代 VkShaderModule）
 * 
 * 用于表示编译后的着色器模块，支持多种渲染后端
 */
using ShaderModuleHandle = void*;

/**
 * 着色器阶段枚举（替代 VkShaderStageFlagBits）
 * 
 * 定义着色器阶段类型，用于编译和创建着色器模块
 */
enum class ShaderStage : uint32_t {
    Vertex = 0x00000001,              // 顶点着色器
    TessellationControl = 0x00000002, // 曲面细分控制着色器
    TessellationEvaluation = 0x00000004, // 曲面细分评估着色器
    Geometry = 0x00000008,            // 几何着色器
    Fragment = 0x00000010,            // 片段着色器
    Compute = 0x00000020,             // 计算着色器
};

/**
 * 缓冲区句柄（不透明指针，替代 VkBuffer）
 * 
 * 用于表示GPU缓冲区，支持顶点缓冲区、索引缓冲区等
 */
using BufferHandle = void*;

/**
 * 设备内存句柄（不透明指针，替代 VkDeviceMemory）
 * 
 * 用于表示GPU设备内存，用于存储缓冲区和纹理数据
 */
using DeviceMemoryHandle = void*;

/**
 * 管线句柄（不透明指针，替代 VkPipeline）
 * 
 * 用于表示图形或计算管线，包含着色器、状态等信息
 */
using PipelineHandle = void*;

/**
 * 管线布局句柄（不透明指针，替代 VkPipelineLayout）
 * 
 * 用于表示管线的布局，包含描述符集布局和推送常量范围
 */
using PipelineLayoutHandle = void*;

/**
 * 描述符集布局句柄（不透明指针，替代 VkDescriptorSetLayout）
 * 
 * 用于表示描述符集的布局，定义着色器中使用的资源绑定
 */
using DescriptorSetLayoutHandle = void*;


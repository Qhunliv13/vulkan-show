#pragma once

#include "core/config/render_context.h"  // 4. 项目头文件

// Vulkan 相关前向声明（避免在抽象接口头文件中暴露）
// 使用前向声明而非完整包含 vulkan.h，减少编译依赖和编译时间
struct VkDevice_T;
struct VkPhysicalDevice_T;
struct VkCommandPool_T;
struct VkQueue_T;
struct VkRenderPass_T;
struct VkExtent2D;

typedef VkDevice_T* VkDevice;
typedef VkPhysicalDevice_T* VkPhysicalDevice;
typedef VkCommandPool_T* VkCommandPool;
typedef VkQueue_T* VkQueue;
typedef VkRenderPass_T* VkRenderPass;

/**
 * Vulkan 渲染上下文工厂函数
 * 
 * 此函数在独立的头文件中声明，避免在抽象接口 render_context.h 中暴露 Vulkan 类型
 * 实现了更好的解耦：抽象接口层不依赖具体渲染后端实现
 * 
 * 所有权：[TRANSFER] 调用方获得所有权，负责使用 delete 或 std::unique_ptr 管理内存
 * 建议：使用 std::unique_ptr<IRenderContext> 包装返回值以确保自动内存管理
 * 
 * @param device Vulkan 设备句柄
 * @param physicalDevice Vulkan 物理设备句柄
 * @param commandPool Vulkan 命令池句柄
 * @param graphicsQueue Vulkan 图形队列句柄
 * @param renderPass Vulkan 渲染通道句柄
 * @param swapchainExtent 交换链尺寸
 * @return IRenderContext* 渲染上下文接口指针，调用者获得所有权
 */
IRenderContext* CreateVulkanRenderContext(VkDevice device, 
                                          VkPhysicalDevice physicalDevice,
                                          VkCommandPool commandPool,
                                          VkQueue graphicsQueue,
                                          VkRenderPass renderPass,
                                          VkExtent2D swapchainExtent);


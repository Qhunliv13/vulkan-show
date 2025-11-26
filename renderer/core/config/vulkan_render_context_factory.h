#pragma once

#include "core/config/render_context.h"  // 4. 项目头文件
#include "core/types/render_types.h"      // 4. 项目头文件（抽象类型）

/**
 * Vulkan 渲染上下文工厂函数
 * 
 * 此函数在独立的头文件中声明，避免在抽象接口 render_context.h 中暴露 Vulkan 类型
 * 实现了更好的解耦：抽象接口层不依赖具体渲染后端实现
 * 
 * 所有权：[TRANSFER] 调用方获得所有权，负责使用 delete 或 std::unique_ptr 管理内存
 * 建议：使用 std::unique_ptr<IRenderContext> 包装返回值以确保自动内存管理
 * 
 * @param device 设备句柄（抽象类型）
 * @param physicalDevice 物理设备句柄（抽象类型）
 * @param commandPool 命令池句柄（抽象类型）
 * @param graphicsQueue 图形队列句柄（抽象类型）
 * @param renderPass 渲染通道句柄（抽象类型）
 * @param swapchainExtent 交换链尺寸（抽象类型）
 * @return IRenderContext* 渲染上下文接口指针，调用者获得所有权
 */
IRenderContext* CreateVulkanRenderContext(DeviceHandle device, 
                                          PhysicalDeviceHandle physicalDevice,
                                          CommandPoolHandle commandPool,
                                          QueueHandle graphicsQueue,
                                          RenderPassHandle renderPass,
                                          Extent2D swapchainExtent);


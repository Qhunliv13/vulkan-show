#pragma once

#include <string>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件

#include "core/types/render_types.h"  // 4. 项目头文件（抽象类型）

// 加载动画类 - 将CSS动画转换为Vulkan渲染
// 职责：实现加载动画效果，将CSS动画转换为Vulkan渲染管线
// 设计：使用9个方块（3x3网格）实现动画效果，每个方块独立移动和着色
// 使用抽象类型以支持多种渲染后端，在实现层进行类型转换
class LoadingAnimation {
public:
    LoadingAnimation();
    ~LoadingAnimation();
    
    // 初始化（需要渲染设备和命令缓冲区）
    // 使用抽象类型以支持多种渲染后端
    bool Initialize(DeviceHandle device, PhysicalDeviceHandle physicalDevice, CommandPoolHandle commandPool, 
                    QueueHandle graphicsQueue, RenderPassHandle renderPass, Extent2D swapchainExtent);
    
    // 清理资源
    void Cleanup();
    
    // 更新动画（传入时间）
    void Update(float time);
    
    // 渲染到命令缓冲区
    void Render(CommandBufferHandle commandBuffer, Extent2D extent);
    
    // 设置位置和大小
    void SetPosition(float x, float y) { m_posX = x; m_posY = y; }
    void SetSize(float width, float height) { m_width = width; m_height = height; }
    
    // 设置方块颜色（RGBA，0.0-1.0）- 设置所有方块
    void SetBoxColor(float r, float g, float b, float a = 1.0f);
    
    // 设置单个方块颜色（boxIndex: 0-8，RGBA，0.0-1.0）
    void SetBoxColor(int boxIndex, float r, float g, float b, float a = 1.0f);
    
private:
    // 方块加载器（banter-loader）
    struct BoxAnimation {
        float x, y;  // 当前位置
        float baseX, baseY;  // 基础位置（网格位置）
        int boxIndex;  // 方块索引（0-8）
    };
    
    // 初始化方块动画
    void InitializeBoxAnimation();
    
    // 更新方块位置（根据CSS动画）
    void UpdateBoxPosition(BoxAnimation& box, float time);
    
    // 创建渲染资源
    bool CreateBuffers();
    bool CreatePipeline(RenderPassHandle renderPass);
    
    // 渲染设备对象（使用抽象类型，在实现层转换为具体类型）
    DeviceHandle m_device = nullptr;
    PhysicalDeviceHandle m_physicalDevice = nullptr;
    CommandPoolHandle m_commandPool = nullptr;
    QueueHandle m_graphicsQueue = nullptr;
    RenderPassHandle m_renderPass = nullptr;
    Extent2D m_swapchainExtent = {};
    
    // 查找内存类型
    // 注意：在实现层需要将MemoryPropertyFlag转换为VkMemoryPropertyFlags
    uint32_t FindMemoryType(uint32_t typeFilter, MemoryPropertyFlag properties);
    
    // 方块动画数据
    std::vector<BoxAnimation> m_boxes;
    static constexpr int BOX_COUNT = 9;
    static constexpr float BOX_SIZE = 20.0f;
    static constexpr float BOX_SPACING = 6.0f;
    static constexpr float GRID_SIZE = 3;  // 3x3网格
    
    // 位置和大小（banter-loader: 72x72px）
    float m_posX = 0.0f;
    float m_posY = 0.0f;
    float m_width = 72.0f;
    float m_height = 72.0f;
    
    // 方块颜色（每个方块单独的颜色）
    struct BoxColor {
        float r = 1.0f;
        float g = 1.0f;
        float b = 1.0f;
        float a = 1.0f;
    };
    std::vector<BoxColor> m_boxColors;  // 9个方块的颜色
    
    // 更新顶点缓冲区颜色
    void UpdateBoxColorBuffer();
    void UpdateBoxColorBuffer(int boxIndex);
    
    // 渲染资源（使用抽象类型，在实现层转换为具体类型）
    std::vector<BufferHandle> m_vertexBuffers;  // 每个方块的顶点缓冲区
    std::vector<DeviceMemoryHandle> m_vertexBufferMemories;  // 每个方块的顶点缓冲区内存
    PipelineHandle m_graphicsPipeline = nullptr;
    PipelineLayoutHandle m_pipelineLayout = nullptr;
    
    bool m_initialized = false;
};


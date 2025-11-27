#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>                  // 2. 系统头文件

#include <vulkan/vulkan.h>            // 3. 第三方库头文件

#include <memory>                     // 2. 系统头文件
#include <string>                     // 2. 系统头文件
#include <vector>                     // 2. 系统头文件
#include "core/config/constants.h"    // 4. 项目头文件（配置）
#include "core/config/stretch_params.h"  // 4. 项目头文件（配置）
#include "core/interfaces/irenderer.h"   // 4. 项目头文件（接口）
#include "core/interfaces/ipipeline_manager.h"  // 4. 项目头文件（接口）
#include "core/interfaces/icamera_controller.h"  // 4. 项目头文件（接口）
#include "core/interfaces/irender_device.h"  // 4. 项目头文件（接口）

// 前向声明
class LoadingAnimation;
class Button;
class ITextRenderer;
class TextRenderer;
class Slider;
class IRenderCommandBuffer;

/**
 * Vulkan渲染器实现 - 实现IRenderer接口，通过组合模式提供IPipelineManager、ICameraController、IRenderDevice子功能
 * 使用pimpl模式隐藏实现细节，减少头文件依赖
 */
class VulkanRenderer : public IRenderer, public IPipelineManager, public ICameraController, public IRenderDevice {
public:
    VulkanRenderer();
    ~VulkanRenderer();
    
    // IRenderer 接口实现
    /**
     * 初始化渲染器
     * 创建Vulkan实例、设备、交换链等所有必要的Vulkan对象
     * 
     * @param hwnd 窗口句柄
     * @param hInstance 应用程序实例句柄
     * @return 成功返回 true，失败返回 false
     */
    bool Initialize(HWND hwnd, HINSTANCE hInstance) override;
    
    /**
     * 清理渲染器资源
     * 按正确顺序销毁所有Vulkan对象，支持多次调用（通过m_initialized标志检查）
     */
    void Cleanup() override;
    
    /**
     * 绘制一帧
     * 执行完整的渲染流程：获取交换链图像、记录命令缓冲区、提交渲染命令、呈现图像
     * 
     * @param time 当前时间（秒）
     * @param useLoadingCubes 是否使用loading_cubes shader渲染
     * @param textRenderer 文本渲染器指针（可选，用于渲染FPS等文本）
     * @param fps 当前帧率（可选，用于显示FPS文本）
     * @return 成功返回 true，失败返回 false
     */
    bool DrawFrame(float time, bool useLoadingCubes = false, 
                  ITextRenderer* textRenderer = nullptr, float fps = 0.0f) override;
    
    /**
     * 绘制带加载动画的帧
     * 用于加载场景，渲染加载动画和UI元素（按钮、滑块等）
     * 
     * @param params 绘制参数，包含加载动画、按钮、滑块、文本渲染器等
     * @return 成功返回 true，失败返回 false
     */
    bool DrawFrameWithLoading(const DrawFrameWithLoadingParams& params) override;
    
    /**
     * 设置拉伸模式
     * 控制shader渲染时的视口和宽高比处理方式
     * 
     * @param mode 拉伸模式（Disabled/Scaled/Fit）
     */
    void SetStretchMode(StretchMode mode) override { m_stretchMode = mode; }
    
    /**
     * 设置背景拉伸模式
     * 控制背景纹理的显示方式（Fit保持比例，Scaled覆盖窗口）
     * 
     * @param mode 背景拉伸模式
     */
    void SetBackgroundStretchMode(BackgroundStretchMode mode) override { m_backgroundStretchMode = mode; }
    
    /**
     * 获取UI基准尺寸
     * 返回用于UI坐标计算的基准尺寸，优先使用背景纹理原始尺寸
     * 
     * @return UI基准尺寸（Extent2D）
     */
    Extent2D GetUIBaseSize() const override;
    
    /**
     * 获取拉伸参数
     * 返回当前拉伸模式的参数（用于UI坐标转换）
     * 
     * @return 拉伸参数引用（const）
     */
    const StretchParams& GetStretchParams() const override { return m_stretchParams; }
    
    /**
     * 加载背景纹理
     * 从文件加载背景纹理，使用Button类实现背景渲染
     * 
     * @param filepath 纹理文件路径
     * @return 成功返回 true，失败返回 false
     */
    bool LoadBackgroundTexture(const std::string& filepath) override;
    
    // IRenderer 接口实现 - 获取子功能接口（组合模式）
    /**
     * 获取管线管理器接口
     * 通过组合模式返回自身（实现IPipelineManager接口）
     * 
     * @return IPipelineManager接口指针（this）
     */
    IPipelineManager* GetPipelineManager() override { return this; }
    
    /**
     * 获取相机控制器接口
     * 通过组合模式返回自身（实现ICameraController接口）
     * 
     * @return ICameraController接口指针（this）
     */
    ICameraController* GetCameraController() override { return this; }
    
    /**
     * 获取渲染设备接口
     * 通过组合模式返回自身（实现IRenderDevice接口）
     * 
     * @return IRenderDevice接口指针（this）
     */
    IRenderDevice* GetRenderDevice() override { return this; }
    
    /**
     * 获取渲染命令缓冲区
     * 返回用于记录渲染命令的缓冲区接口指针
     * 
     * @return IRenderCommandBuffer接口指针（不拥有所有权，由VulkanRenderer管理生命周期）
     */
    IRenderCommandBuffer* GetCommandBuffer() override;
    
    // IRenderDevice 接口实现
    /**
     * 获取交换链尺寸
     * 
     * @return 交换链尺寸（Extent2D）
     */
    Extent2D GetSwapchainExtent() const override { 
        return Extent2D(m_swapchainExtent.width, m_swapchainExtent.height); 
    }
    
    /**
     * 获取设备句柄
     * 
     * @return 设备句柄（抽象类型）
     */
    DeviceHandle GetDevice() const override { return static_cast<DeviceHandle>(m_device); }
    
    /**
     * 获取物理设备句柄
     * 
     * @return 物理设备句柄（抽象类型）
     */
    PhysicalDeviceHandle GetPhysicalDevice() const override { return static_cast<PhysicalDeviceHandle>(m_physicalDevice); }
    
    /**
     * 获取命令池句柄
     * 
     * @return 命令池句柄（抽象类型）
     */
    CommandPoolHandle GetCommandPool() const override { return static_cast<CommandPoolHandle>(m_commandPool); }
    
    /**
     * 获取图形队列句柄
     * 
     * @return 图形队列句柄（抽象类型）
     */
    QueueHandle GetGraphicsQueue() const override { return static_cast<QueueHandle>(m_graphicsQueue); }
    
    /**
     * 获取渲染通道句柄
     * 
     * @return 渲染通道句柄（抽象类型）
     */
    RenderPassHandle GetRenderPass() const override { return static_cast<RenderPassHandle>(m_renderPass); }
    
    /**
     * 获取交换链图像格式
     * 将Vulkan格式转换为抽象格式类型
     * 
     * @return 图像格式（ImageFormat）
     */
    ImageFormat GetSwapchainFormat() const override { 
        // 将 VkFormat 转换为 ImageFormat（简化映射）
        return static_cast<ImageFormat>(m_swapchainImageFormat); 
    }
    
    /**
     * 获取交换链图像数量
     * 
     * @return 交换链图像数量
     */
    uint32_t GetSwapchainImageCount() const override { return m_swapchainImageCount; }
    
    // ICameraController 接口实现
    /**
     * 设置鼠标输入
     * 累积鼠标移动增量，用于相机旋转控制
     * 
     * @param deltaX 鼠标X方向移动增量
     * @param deltaY 鼠标Y方向移动增量
     * @param buttonDown 鼠标左键是否按下
     */
    void SetMouseInput(float deltaX, float deltaY, bool buttonDown) override;
    
    /**
     * 设置键盘输入
     * 设置WASD键的按下状态，用于相机移动控制
     * 
     * @param w W键是否按下（前进）
     * @param a A键是否按下（左移）
     * @param s S键是否按下（后退）
     * @param d D键是否按下（右移）
     */
    void SetKeyInput(bool w, bool a, bool s, bool d) override;
    
    /**
     * 更新相机状态
     * 根据输入状态更新相机位置和旋转角度
     * 
     * @param deltaTime 时间步长（秒）
     */
    void UpdateCamera(float deltaTime) override;
    
    /**
     * 重置相机状态
     * 将相机位置、旋转角度和输入状态重置为初始值
     */
    void ResetCamera() override;
    
    // IPipelineManager 接口实现
    /**
     * 创建图形管线
     * 从shader文件创建Vulkan图形管线，支持SPIR-V和GLSL格式
     * 
     * @param vertShaderPath 顶点shader文件路径
     * @param fragShaderPath 片段shader文件路径
     * @return 成功返回 true，失败返回 false
     */
    bool CreateGraphicsPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath) override;
    
    /**
     * 创建loading_cubes管线
     * 创建用于加载动画场景的专用图形管线
     * 
     * @param vertShaderPath 顶点shader文件路径
     * @param fragShaderPath 片段shader文件路径
     * @return 成功返回 true，失败返回 false
     */
    bool CreateLoadingCubesPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath) override;
    
    /**
     * 检查是否支持光线追踪
     * 检查硬件和驱动是否支持Vulkan光线追踪扩展
     * 
     * @return 支持返回 true，不支持返回 false
     */
    bool IsRayTracingSupported() const override { return m_rayTracingSupported; }
    
    /**
     * 创建光线追踪管线
     * 创建硬件光线追踪管线（当前为框架实现，实际使用软件fallback）
     * 
     * @return 成功返回 true，失败返回 false（当前实现返回false，使用软件fallback）
     */
    bool CreateRayTracingPipeline() override;
    
    // VulkanRenderer 特有的方法（不在接口中）
    /**
     * 记录命令缓冲区
     * 将渲染命令记录到Vulkan命令缓冲区中
     * 
     * @param commandBuffer Vulkan命令缓冲区句柄
     * @param imageIndex 交换链图像索引
     * @param time 当前时间（秒）
     * @param useLoadingCubes 是否使用loading_cubes shader
     * @param textRenderer 文本渲染器指针（可选）
     * @param fps 当前帧率（可选，用于显示FPS文本）
     */
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, float time, bool useLoadingCubes = false, ITextRenderer* textRenderer = nullptr, float fps = 0.0f);
    
    /**
     * 清理背景纹理
     * 释放背景纹理资源
     */
    void CleanupBackgroundTexture();
    
    /**
     * 检查是否有背景纹理
     * 
     * @return 有背景纹理返回 true，否则返回 false
     */
    bool HasBackgroundTexture() const;
    
private:
    // 绘制背景纹理（保持宽高比居中填充窗口）
    void RenderBackgroundTexture(VkCommandBuffer commandBuffer, VkExtent2D extent);
    bool CreateInstance();
    bool CreateSurface(HWND hwnd, HINSTANCE hInstance);
    bool SelectPhysicalDevice();
    bool CreateLogicalDevice();
    bool CreateSwapchain();
    bool CreateImageViews();
    bool CreateRenderPass();
    bool CreateFramebuffers();
    bool CreateCommandPool();
    bool CreateCommandBuffers();
    bool CreateSyncObjects();
    
    void CleanupSwapchain();
    void RecreateSwapchain();
    
    // Vulkan对象
    VkInstance m_instance = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    std::vector<VkFramebuffer> m_swapchainFramebuffers;
    VkFormat m_swapchainImageFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D m_swapchainExtent = {};
    uint32_t m_swapchainImageCount = 0;
    
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_loadingCubesPipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_loadingCubesPipeline = VK_NULL_HANDLE;
    
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;
    
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    
    uint32_t m_graphicsQueueFamily = UINT32_MAX;
    uint32_t m_presentQueueFamily = UINT32_MAX;
    uint32_t m_currentFrame = 0;
    
    bool m_initialized = false;  // 初始化状态标志，防止重复初始化
    
    HWND m_hwnd = nullptr;
    StretchMode m_stretchMode = StretchMode::Scaled;
    BackgroundStretchMode m_backgroundStretchMode = BackgroundStretchMode::Fit;  // 背景拉伸模式
    
    // 背景纹理原始尺寸（用于计算宽高比）
    uint32_t m_backgroundTextureWidth = 0;
    uint32_t m_backgroundTextureHeight = 0;
    
    // Scaled 模式的拉伸参数（已废弃，Scaled模式不使用此结构）
    StretchParams m_stretchParams;
    
    // 背景纹理（使用Button类实现）
    std::unique_ptr<Button> m_backgroundButton;
    
    // 相机状态（初始值，实际计算在GPU上完成）
    float m_cameraYaw = 0.0f;    // 水平旋转角度（弧度）
    float m_cameraPitch = 0.0f;   // 垂直旋转角度（弧度）
    float m_cameraPosX = 0.0f;   // 相机X位置
    float m_cameraPosY = 0.0f;   // 相机Y位置
    float m_cameraPosZ = 2.2f;   // 相机Z位置（默认稍微拉近）
    
    // 输入状态（传递给GPU）
    float m_mouseDeltaX = 0.0f;
    float m_mouseDeltaY = 0.0f;
    bool m_mouseButtonDown = false;
    bool m_keyW = false;
    bool m_keyA = false;
    bool m_keyS = false;
    bool m_keyD = false;
    
    // 光线追踪相关
    bool m_rayTracingSupported = false;
    VkPipeline m_rayTracingPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_rayTracingPipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_rayTracingDescriptorSetLayout = VK_NULL_HANDLE;
    
    // 检查光线追踪扩展支持
    bool CheckRayTracingSupport();
    
    // 渲染命令缓冲区（使用接口指针，避免头文件直接依赖实现）
    // 在 .cpp 文件中使用 std::unique_ptr<RenderCommandBuffer> 管理实际对象
    IRenderCommandBuffer* m_commandBuffer = nullptr;
    
    // 内部实现细节（pimpl模式，避免头文件暴露完整类型定义）
    struct Impl;
    std::unique_ptr<Impl> m_pImpl;
};


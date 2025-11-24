#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include "core/constants.h"

// Canvas Items 模式的拉伸参数
struct StretchParams {
    float stretchScaleX = 1.0f;      // X轴拉伸比例
    float stretchScaleY = 1.0f;      // Y轴拉伸比例
    float logicalWidth = 800.0f;     // 逻辑宽度（viewportWidth）
    float logicalHeight = 800.0f;    // 逻辑高度（viewportHeight）
    float screenWidth = 800.0f;      // 屏幕宽度
    float screenHeight = 800.0f;     // 屏幕高度
    float marginX = 0.0f;            // X轴边距（offsetX）
    float marginY = 0.0f;            // Y轴边距（offsetY）
};

class VulkanRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();
    
    bool Initialize(HWND hwnd, HINSTANCE hInstance);
    void Cleanup();
    
    bool CreateGraphicsPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath);
    bool CreateLoadingCubesPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath);
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, float time, bool useLoadingCubes = false, class TextRenderer* textRenderer = nullptr, float fps = 0.0f);
    bool DrawFrame(float time, bool useLoadingCubes = false, class TextRenderer* textRenderer = nullptr, float fps = 0.0f);
    void SetAspectRatioMode(AspectRatioMode mode) { m_aspectRatioMode = mode; }
    void SetStretchMode(StretchMode mode) { m_stretchMode = mode; }
    void SetBackgroundStretchMode(BackgroundStretchMode mode) { m_backgroundStretchMode = mode; }
    
    // 获取UI基准尺寸（基于背景纹理大小，如果没有背景则使用800x800）
    VkExtent2D GetUIBaseSize() const;
    bool DrawFrameWithLoading(float time, class LoadingAnimation* loadingAnim, 
                              class Button* button = nullptr,
                              class TextRenderer* textRenderer = nullptr,
                              class Button* colorButton = nullptr,
                              class Button* leftButton = nullptr,
                              const std::vector<class Button*>* additionalButtons = nullptr,
                              class Slider* slider = nullptr,
                              const std::vector<class Slider*>* additionalSliders = nullptr,
                              float fps = 0.0f);
    
    VkExtent2D GetSwapchainExtent() const { return m_swapchainExtent; }
    VkFormat GetSwapchainFormat() const { return m_swapchainImageFormat; }
    uint32_t GetSwapchainImageCount() const { return m_swapchainImageCount; }
    
    // 获取Vulkan对象（用于加载动画等）
    VkDevice GetDevice() const { return m_device; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    VkCommandPool GetCommandPool() const { return m_commandPool; }
    VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    VkRenderPass GetRenderPass() const { return m_renderPass; }
    
    // 获取Scaled模式的拉伸参数（已废弃，Scaled模式不使用此结构）
    const StretchParams& GetStretchParams() const { return m_stretchParams; }
    
    // 背景纹理管理
    bool LoadBackgroundTexture(const std::string& filepath);
    void CleanupBackgroundTexture();
    bool HasBackgroundTexture() const;
    
    // 相机控制（用于loading_cubes shader）
    void SetMouseInput(float deltaX, float deltaY, bool buttonDown);  // 设置鼠标输入
    void SetKeyInput(bool w, bool a, bool s, bool d);  // 设置键盘输入
    void UpdateCamera(float deltaTime);  // 更新相机状态（基于累积的输入）
    void ResetCamera();  // 重置相机状态
    
    // 光线追踪支持
    bool IsRayTracingSupported() const { return m_rayTracingSupported; }
    bool CreateRayTracingPipeline();  // 创建光线追踪管线
    
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
    
    HWND m_hwnd = nullptr;
    AspectRatioMode m_aspectRatioMode = AspectRatioMode::Keep;
    StretchMode m_stretchMode = StretchMode::Scaled;
    BackgroundStretchMode m_backgroundStretchMode = BackgroundStretchMode::Fit;  // 背景拉伸模式
    
    // 背景纹理原始尺寸（用于计算宽高比）
    uint32_t m_backgroundTextureWidth = 0;
    uint32_t m_backgroundTextureHeight = 0;
    
    // Scaled 模式的拉伸参数（已废弃，Scaled模式不使用此结构）
    StretchParams m_stretchParams;
    
    // 背景纹理（使用Button类实现，不需要额外的成员变量）
    
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
};


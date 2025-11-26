#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <memory>
#include "core/config/constants.h"
#include "core/config/stretch_params.h"
#include "core/interfaces/irenderer.h"

// 前向声明
class LoadingAnimation;
class Button;
class ITextRenderer;
class TextRenderer;  // 用于内部实现
class Slider;

class VulkanRenderer : public IRenderer {
public:
    VulkanRenderer();
    ~VulkanRenderer();
    
    // IRenderer 接口实现
    bool Initialize(HWND hwnd, HINSTANCE hInstance) override;
    void Cleanup() override;
    
    bool DrawFrame(float time, bool useLoadingCubes = false, 
                  ITextRenderer* textRenderer = nullptr, float fps = 0.0f) override;
    bool DrawFrameWithLoading(const DrawFrameWithLoadingParams& params) override;
    
    void SetAspectRatioMode(AspectRatioMode mode) override { m_aspectRatioMode = mode; }
    void SetStretchMode(StretchMode mode) override { m_stretchMode = mode; }
    void SetBackgroundStretchMode(BackgroundStretchMode mode) override { m_backgroundStretchMode = mode; }
    
    VkExtent2D GetUIBaseSize() const override;
    VkExtent2D GetSwapchainExtent() const override { return m_swapchainExtent; }
    
    VkDevice GetDevice() const override { return m_device; }
    VkPhysicalDevice GetPhysicalDevice() const override { return m_physicalDevice; }
    VkCommandPool GetCommandPool() const override { return m_commandPool; }
    VkQueue GetGraphicsQueue() const override { return m_graphicsQueue; }
    VkRenderPass GetRenderPass() const override { return m_renderPass; }
    
    const StretchParams& GetStretchParams() const override { return m_stretchParams; }
    
    bool LoadBackgroundTexture(const std::string& filepath) override;
    
    void SetMouseInput(float deltaX, float deltaY, bool buttonDown) override;
    void SetKeyInput(bool w, bool a, bool s, bool d) override;
    void UpdateCamera(float deltaTime) override;
    
    bool CreateGraphicsPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath) override;
    bool CreateLoadingCubesPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath) override;
    
    bool IsRayTracingSupported() const override { return m_rayTracingSupported; }
    bool CreateRayTracingPipeline() override;
    
    // VulkanRenderer 特有的方法（不在接口中）
    void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, float time, bool useLoadingCubes = false, ITextRenderer* textRenderer = nullptr, float fps = 0.0f);
    void CleanupBackgroundTexture();
    bool HasBackgroundTexture() const;
    void ResetCamera();  // 重置相机状态
    
    VkFormat GetSwapchainFormat() const { return m_swapchainImageFormat; }
    uint32_t GetSwapchainImageCount() const { return m_swapchainImageCount; }
    
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


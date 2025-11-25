#pragma once

#include <windows.h>
#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include "core/config/constants.h"
#include "core/config/stretch_params.h"

// 前向声明
class ITextRenderer;
class Button;
class Slider;
class LoadingAnimation;

// DrawFrameWithLoading 函数的参数结构体
struct DrawFrameWithLoadingParams {
    float time = 0.0f;
    LoadingAnimation* loadingAnim = nullptr;
    Button* enterButton = nullptr;
    ITextRenderer* textRenderer = nullptr;
    Button* colorButton = nullptr;
    Button* leftButton = nullptr;
    const std::vector<Button*>* additionalButtons = nullptr;
    Slider* slider = nullptr;
    const std::vector<Slider*>* additionalSliders = nullptr;
    float fps = 0.0f;
};

// 渲染器接口 - 抽象层，用于解耦组件与具体渲染实现
class IRenderer {
public:
    virtual ~IRenderer() = default;
    
    // 初始化和清理
    virtual bool Initialize(HWND hwnd, HINSTANCE hInstance) = 0;
    virtual void Cleanup() = 0;
    
    // 渲染相关
    virtual bool DrawFrame(float time, bool useLoadingCubes = false, 
                          ITextRenderer* textRenderer = nullptr, float fps = 0.0f) = 0;
    virtual bool DrawFrameWithLoading(const DrawFrameWithLoadingParams& params) = 0;
    
    // 配置设置
    virtual void SetAspectRatioMode(AspectRatioMode mode) = 0;
    virtual void SetStretchMode(StretchMode mode) = 0;
    virtual void SetBackgroundStretchMode(BackgroundStretchMode mode) = 0;
    
    // 获取尺寸信息
    virtual VkExtent2D GetUIBaseSize() const = 0;
    virtual VkExtent2D GetSwapchainExtent() const = 0;
    
    // 获取Vulkan对象（用于UI组件初始化）
    virtual VkDevice GetDevice() const = 0;
    virtual VkPhysicalDevice GetPhysicalDevice() const = 0;
    virtual VkCommandPool GetCommandPool() const = 0;
    virtual VkQueue GetGraphicsQueue() const = 0;
    virtual VkRenderPass GetRenderPass() const = 0;
    
    // 获取拉伸参数
    virtual const StretchParams& GetStretchParams() const = 0;
    
    // 背景纹理管理
    virtual bool LoadBackgroundTexture(const std::string& filepath) = 0;
    
    // 相机控制（用于loading_cubes shader）
    virtual void SetMouseInput(float deltaX, float deltaY, bool buttonDown) = 0;
    virtual void SetKeyInput(bool w, bool a, bool s, bool d) = 0;
    virtual void UpdateCamera(float deltaTime) = 0;
    
    // 管线创建
    virtual bool CreateGraphicsPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath) = 0;
    virtual bool CreateLoadingCubesPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath) = 0;
    
    // 光线追踪支持
    virtual bool IsRayTracingSupported() const = 0;
    virtual bool CreateRayTracingPipeline() = 0;
};


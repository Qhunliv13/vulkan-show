#pragma once

#include <windows.h>  // 2. 系统头文件
#include <string>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件
#include "core/config/constants.h"  // 4. 项目头文件（配置）
#include "core/config/stretch_params.h"  // 4. 项目头文件（配置）
#include "core/types/render_types.h"  // 4. 项目头文件（类型）
#include "core/interfaces/irender_command.h"  // 4. 项目头文件（接口）

// 前向声明（遵循接口隔离原则，不直接继承其他接口）
class ITextRenderer;
class Button;
class Slider;
class LoadingAnimation;
class IPipelineManager;
class ICameraController;
class IRenderDevice;

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

/**
 * 渲染器接口 - 抽象层，用于解耦组件与具体渲染实现
 * 
 * 职责：专注于渲染帧和配置管理，不暴露具体渲染后端细节
 * 设计：使用组合替代多重继承，遵循接口隔离原则
 * 子功能访问：通过 GetPipelineManager()、GetCameraController()、GetRenderDevice() 访问子功能
 * 
 * 使用方式：
 * 1. 通过工厂接口创建实现（如 VulkanRendererFactory）
 * 2. 使用接口指针操作，无需了解具体实现
 * 3. 通过组合模式访问子功能接口
 */
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
    virtual void SetStretchMode(StretchMode mode) = 0;
    virtual void SetBackgroundStretchMode(BackgroundStretchMode mode) = 0;
    
    // 获取尺寸信息
    virtual Extent2D GetUIBaseSize() const = 0;
    
    // 获取拉伸参数
    virtual const StretchParams& GetStretchParams() const = 0;
    
    // 背景纹理管理
    virtual bool LoadBackgroundTexture(const std::string& filepath) = 0;
    
    /**
     * 获取渲染命令缓冲区（用于延迟执行）
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由渲染器管理生命周期
     * 
     * @return IRenderCommandBuffer* 渲染命令缓冲区指针，可能为 nullptr
     */
    virtual IRenderCommandBuffer* GetCommandBuffer() = 0;
    
    /**
     * 获取管线管理器接口（组合模式，遵循接口隔离原则）
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由渲染器管理生命周期
     * 
     * @return IPipelineManager* 管线管理器接口指针，可能为 nullptr（如果实现类不支持）
     */
    virtual IPipelineManager* GetPipelineManager() = 0;
    
    /**
     * 获取相机控制器接口（组合模式，遵循接口隔离原则）
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由渲染器管理生命周期
     * 
     * @return ICameraController* 相机控制器接口指针，可能为 nullptr（如果实现类不支持）
     */
    virtual ICameraController* GetCameraController() = 0;
    
    /**
     * 获取渲染设备接口（组合模式，遵循接口隔离原则）
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由渲染器管理生命周期
     * 
     * @return IRenderDevice* 渲染设备接口指针，可能为 nullptr（如果实现类不支持）
     */
    virtual IRenderDevice* GetRenderDevice() = 0;
    
    // 便捷方法：为了向后兼容，提供直接访问常用功能的方法
    // 这些方法内部调用 GetRenderDevice() 等方法，使用组合模式
    // 注意：这些方法不是虚函数，因为它们只是包装器，不需要被覆盖
    Extent2D GetSwapchainExtent() const;
    DeviceHandle GetDevice() const;
    PhysicalDeviceHandle GetPhysicalDevice() const;
    CommandPoolHandle GetCommandPool() const;
    QueueHandle GetGraphicsQueue() const;
    RenderPassHandle GetRenderPass() const;
    ImageFormat GetSwapchainFormat() const;
    uint32_t GetSwapchainImageCount() const;
};


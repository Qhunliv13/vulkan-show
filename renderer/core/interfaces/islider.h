#pragma once

#include <functional>  // 2. 系统头文件
#include <string>       // 2. 系统头文件

#include "core/types/render_types.h"  // 4. 项目头文件（类型）

// 前向声明
class IRenderContext;

// 滑块配置结构体（与Slider类共享，定义在slider.h中）
struct SliderConfig;

/**
 * 滑块组件接口 - 用于解耦滑块组件与UI管理，不依赖具体实现
 * 
 * 职责：提供平台无关的滑块组件接口，支持渲染、交互、值管理
 * 设计：使用抽象类型，支持多种渲染后端（Vulkan、OpenGL、DirectX等）
 * 
 * 使用方式：
 * 1. 通过依赖注入接收IRenderContext
 * 2. 使用接口指针操作，无需了解具体实现
 * 3. 支持拖动、值变化回调、相对位置等功能
 */
class ISlider {
public:
    virtual ~ISlider() = default;
    
    /**
     * 初始化滑块
     * 
     * @param renderContext 渲染上下文接口（不拥有所有权）
     * @param config 滑块配置（位置、大小、颜色、值范围等）
     * @param usePureShader 是否使用纯shader渲染模式（默认false）
     * @return 初始化成功返回true，失败返回false
     */
    virtual bool Initialize(IRenderContext* renderContext,
                           const SliderConfig& config,
                           bool usePureShader = false) = 0;
    
    /**
     * 清理资源
     */
    virtual void Cleanup() = 0;
    
    /**
     * 设置滑块位置和大小（窗口坐标，Y向下，原点在左上角）
     */
    virtual void SetPosition(float x, float y) = 0;
    virtual void SetSize(float width, float height) = 0;
    
    /**
     * 设置滑块值（会自动限制在minValue和maxValue之间）
     */
    virtual void SetValue(float value) = 0;
    
    /**
     * 获取当前值
     */
    virtual float GetValue() const = 0;
    
    /**
     * 获取归一化值（0.0-1.0）
     */
    virtual float GetNormalizedValue() const = 0;
    
    /**
     * 设置值范围
     */
    virtual void SetRange(float minValue, float maxValue) = 0;
    
    /**
     * 设置滑块轨道颜色
     */
    virtual void SetTrackColor(float r, float g, float b, float a = 1.0f) = 0;
    
    /**
     * 设置滑块填充颜色
     */
    virtual void SetFillColor(float r, float g, float b, float a = 1.0f) = 0;
    
    /**
     * 设置拖动点颜色
     */
    virtual void SetThumbColor(float r, float g, float b, float a = 1.0f) = 0;
    
    /**
     * 设置拖动点纹理
     */
    virtual void SetThumbTexture(const std::string& texturePath) = 0;
    
    /**
     * 设置滑块在屏幕上的相对位置（0.0-1.0，相对于屏幕宽高）
     */
    virtual void SetRelativePosition(float relX, float relY, float screenWidth = 0.0f, float screenHeight = 0.0f) = 0;
    
    /**
     * 更新窗口大小（用于相对位置计算）
     */
    virtual void UpdateScreenSize(float screenWidth, float screenHeight) = 0;
    
    /**
     * 获取滑块位置和大小
     */
    virtual float GetX() const = 0;
    virtual float GetY() const = 0;
    virtual float GetWidth() const = 0;
    virtual float GetHeight() const = 0;
    
    /**
     * 设置和获取渲染层级（z-index）
     */
    virtual void SetZIndex(int zIndex) = 0;
    virtual int GetZIndex() const = 0;
    
    /**
     * 设置和获取可见性
     */
    virtual void SetVisible(bool visible) = 0;
    virtual bool IsVisible() const = 0;
    
    /**
     * 检测点是否在滑块轨道内（窗口坐标）
     */
    virtual bool IsPointInsideTrack(float px, float py) const = 0;
    
    /**
     * 检测点是否在拖动点内（窗口坐标）
     */
    virtual bool IsPointInsideThumb(float px, float py) const = 0;
    
    /**
     * 根据鼠标位置设置滑块值（用于拖动）
     */
    virtual void SetValueFromPosition(float px, float py) = 0;
    
    /**
     * 渲染滑块到命令缓冲区
     */
    virtual void Render(CommandBufferHandle commandBuffer, Extent2D extent) = 0;
    
    /**
     * 设置值变化回调函数
     */
    virtual void SetOnValueChangedCallback(std::function<void(float)> callback) = 0;
    
    /**
     * 处理鼠标按下
     * 
     * @return 是否点击了滑块
     */
    virtual bool HandleMouseDown(float clickX, float clickY) = 0;
    
    /**
     * 处理鼠标移动（用于拖动）
     * 
     * @return 是否正在拖动滑块
     */
    virtual bool HandleMouseMove(float mouseX, float mouseY) = 0;
    
    /**
     * 处理鼠标释放
     */
    virtual void HandleMouseUp() = 0;
    
    /**
     * 更新滑块位置以适应窗口大小变化（保持相对位置）
     */
    virtual void UpdateForWindowResize(float newWidth, float newHeight) = 0;
    
    /**
     * 设置Scaled模式的拉伸参数（已废弃）
     */
    virtual void SetStretchParams(const struct StretchParams& params) = 0;
};


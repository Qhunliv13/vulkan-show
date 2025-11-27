#pragma once

#include <functional>  // 2. 系统头文件
#include <string>       // 2. 系统头文件

#include "core/types/render_types.h"  // 4. 项目头文件（类型）

// 前向声明
class IRenderContext;
class ITextRenderer;

// 按钮配置结构体（与Button类共享，定义在button.h中）
struct ButtonConfig;

/**
 * 按钮组件接口 - 用于解耦按钮组件与UI管理，不依赖具体实现
 * 
 * 职责：提供平台无关的按钮组件接口，支持渲染、交互、状态管理
 * 设计：使用抽象类型，支持多种渲染后端（Vulkan、OpenGL、DirectX等）
 * 
 * 使用方式：
 * 1. 通过依赖注入接收IRenderContext和ITextRenderer
 * 2. 使用接口指针操作，无需了解具体实现
 * 3. 支持点击检测、悬停效果、文本渲染等功能
 */
class IButton {
public:
    virtual ~IButton() = default;
    
    /**
     * 初始化按钮
     * 
     * @param renderContext 渲染上下文接口（不拥有所有权）
     * @param config 按钮配置（位置、大小、颜色、纹理等）
     * @param textRenderer 文字渲染器接口（可选，用于渲染按钮文本）
     * @param usePureShader 是否使用纯shader渲染模式（默认false）
     * @return 初始化成功返回true，失败返回false
     */
    virtual bool Initialize(IRenderContext* renderContext,
                           const ButtonConfig& config,
                           ITextRenderer* textRenderer = nullptr,
                           bool usePureShader = false) = 0;
    
    /**
     * 清理资源
     */
    virtual void Cleanup() = 0;
    
    /**
     * 设置按钮位置（窗口坐标，Y向下，原点在左上角）
     */
    virtual void SetPosition(float x, float y) = 0;
    
    /**
     * 设置按钮大小
     */
    virtual void SetSize(float width, float height) = 0;
    
    /**
     * 设置按钮边界（位置和大小）
     */
    virtual void SetBounds(float x, float y, float width, float height) = 0;
    
    /**
     * 设置按钮颜色（RGBA，0.0-1.0）
     */
    virtual void SetColor(float r, float g, float b, float a = 1.0f) = 0;
    
    /**
     * 设置纹理路径
     */
    virtual void SetTexture(const std::string& texturePath) = 0;
    
    /**
     * 设置按钮文本
     */
    virtual void SetText(const std::string& text) = 0;
    
    /**
     * 设置文本颜色（RGBA，0.0-1.0）
     */
    virtual void SetTextColor(float r, float g, float b, float a = 1.0f) = 0;
    
    /**
     * 设置文字渲染器
     */
    virtual void SetTextRenderer(ITextRenderer* textRenderer) = 0;
    
    /**
     * 设置按钮在屏幕上的相对位置（0.0-1.0，相对于屏幕宽高）
     */
    virtual void SetRelativePosition(float relX, float relY, float screenWidth = 0.0f, float screenHeight = 0.0f) = 0;
    
    /**
     * 更新窗口大小（用于相对位置计算）
     */
    virtual void UpdateScreenSize(float screenWidth, float screenHeight) = 0;
    
    /**
     * 设置固定screenSize模式（用于FIT模式）
     */
    virtual void SetFixedScreenSize(bool fixed) = 0;
    
    /**
     * 获取按钮位置和大小
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
     * 设置按钮形状类型（0=矩形，1=圆形）
     */
    virtual void SetShapeType(int shapeType) = 0;
    virtual int GetShapeType() const = 0;
    
    /**
     * 设置悬停效果
     */
    virtual void SetHoverEffect(bool enable, int effectType = 0, float strength = 0.2f) = 0;
    
    /**
     * 检查是否有纹理
     */
    virtual bool HasTexture() const = 0;
    
    /**
     * 检测点是否在按钮内（窗口坐标）
     */
    virtual bool IsPointInside(float px, float py) const = 0;
    
    /**
     * 渲染按钮到命令缓冲区
     */
    virtual void Render(CommandBufferHandle commandBuffer, Extent2D extent) = 0;
    
    /**
     * 渲染按钮文本（单独调用，确保在所有其他元素之后渲染）
     */
    virtual void RenderText(CommandBufferHandle commandBuffer, Extent2D extent, 
                            const void* viewport = nullptr, const void* scissor = nullptr) = 0;
    
    /**
     * 设置点击回调函数
     */
    virtual void SetOnClickCallback(std::function<void()> callback) = 0;
    
    /**
     * 处理鼠标点击
     * 
     * @return true表示点击了按钮，false表示未点击按钮
     */
    virtual bool HandleClick(float clickX, float clickY) = 0;
    
    /**
     * 处理鼠标移动（用于检测悬停状态）
     * 
     * @return true表示鼠标在按钮上，false表示鼠标不在按钮上
     */
    virtual bool HandleMouseMove(float mouseX, float mouseY) = 0;
    
    /**
     * 更新按钮位置以适应窗口大小变化（保持相对位置）
     */
    virtual void UpdateForWindowResize(float newWidth, float newHeight) = 0;
    
    /**
     * 设置Scaled模式的拉伸参数（已废弃）
     */
    virtual void SetStretchParams(const struct StretchParams& params) = 0;
};


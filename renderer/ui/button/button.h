#pragma once

#include <functional>    // 2. 系统头文件
#include <memory>         // 2. 系统头文件
#include <string>         // 2. 系统头文件

#include "core/types/render_types.h"  // 4. 项目头文件（抽象类型）

// 前向声明
class IRenderContext;
class TextRenderer;
namespace renderer { namespace texture { class Texture; } }

// Scaled 模式的拉伸参数（前向声明，实际定义在 core/stretch_params.h，已废弃）
struct StretchParams;

// 按钮配置结构体 - 所有参数由使用方传入
struct ButtonConfig {
    // 位置和大小（窗口坐标，Y向下，原点在左上角）
    float x = 0.0f;
    float y = 0.0f;
    float width = 200.0f;
    float height = 50.0f;
    
    // 颜色（RGBA，0.0-1.0）- 当不使用纹理时使用
    float colorR = 1.0f;
    float colorG = 0.0f;
    float colorB = 0.0f;
    float colorA = 1.0f;
    
    // 纹理路径（可选，如果为空则使用颜色）
    std::string texturePath = "";
    
    // 是否使用相对位置（0.0-1.0，相对于屏幕宽高）
    bool useRelativePosition = false;
    float relativeX = 0.5f;  // 0.5 = 屏幕中央
    float relativeY = 0.75f; // 0.75 = 屏幕下方75%位置
    
    // 文本相关（可选）
    bool enableText = false;        // 是否启用文本
    std::string text = "";           // 按钮文本
    float textColorR = 1.0f;        // 文本颜色R
    float textColorG = 1.0f;        // 文本颜色G
    float textColorB = 1.0f;        // 文本颜色B
    float textColorA = 1.0f;        // 文本颜色A
    
    // 渲染层级（z-index，数值越大越在上层，默认0）
    int zIndex = 0;
    
    // 按钮形状类型（0=矩形，1=圆形，默认0）
    int shapeType = 0;  // 0: 矩形, 1: 圆形
    
    // 悬停效果相关配置
    bool enableHoverEffect = false;  // 是否启用悬停效果
    int hoverEffectType = 0;         // 悬停效果类型：0=变暗, 1=变淡
    float hoverEffectStrength = 0.2f; // 悬停效果强度（0.0-1.0），默认0.2（20%）
    
    // 构造函数 - 方便快速创建配置
    ButtonConfig() = default;
    
    // 使用绝对位置（带颜色）
    ButtonConfig(float x, float y, float width, float height, 
                 float r, float g, float b, float a = 1.0f)
        : x(x), y(y), width(width), height(height),
          colorR(r), colorG(g), colorB(b), colorA(a),
          useRelativePosition(false), enableText(false) {}
    
    // 使用绝对位置（默认红色）
    ButtonConfig(float x, float y, float width, float height)
        : x(x), y(y), width(width), height(height),
          useRelativePosition(false), enableText(false) {}
    
    // 使用相对位置（带颜色）- 需要明确指定所有颜色参数以避免歧义
    static ButtonConfig CreateRelative(float relX, float relY, float width, float height,
                                       float r, float g, float b, float a = 1.0f) {
        ButtonConfig config;
        config.width = width;
        config.height = height;
        config.colorR = r;
        config.colorG = g;
        config.colorB = b;
        config.colorA = a;
        config.useRelativePosition = true;
        config.relativeX = relX;
        config.relativeY = relY;
        config.enableText = false;
        return config;
    }
    
    // 使用相对位置（默认红色）
    static ButtonConfig CreateRelative(float relX, float relY, float width, float height) {
        ButtonConfig config;
        config.width = width;
        config.height = height;
        config.useRelativePosition = true;
        config.relativeX = relX;
        config.relativeY = relY;
        config.enableText = false;
        return config;
    }
    
    // 使用绝对位置和纹理
    ButtonConfig(float x, float y, float width, float height, const std::string& texturePath)
        : x(x), y(y), width(width), height(height),
          texturePath(texturePath), useRelativePosition(false), enableText(false) {}
    
    // 使用相对位置和纹理
    static ButtonConfig CreateRelativeWithTexture(float relX, float relY, float width, float height, 
                                                   const std::string& texturePath) {
        ButtonConfig config;
        config.width = width;
        config.height = height;
        config.texturePath = texturePath;
        config.useRelativePosition = true;
        config.relativeX = relX;
        config.relativeY = relY;
        config.enableText = false;
        return config;
    }
    
    // 使用相对位置、颜色和文本
    static ButtonConfig CreateRelativeWithText(float relX, float relY, float width, float height,
                                               float r, float g, float b, float a,
                                               const std::string& text,
                                               float textR = 1.0f, float textG = 1.0f, 
                                               float textB = 1.0f, float textA = 1.0f) {
        ButtonConfig config;
        config.width = width;
        config.height = height;
        config.colorR = r;
        config.colorG = g;
        config.colorB = b;
        config.colorA = a;
        config.useRelativePosition = true;
        config.relativeX = relX;
        config.relativeY = relY;
        config.enableText = true;
        config.text = text;
        config.textColorR = textR;
        config.textColorG = textG;
        config.textColorB = textB;
        config.textColorA = textA;
        return config;
    }
};

/**
 * 独立的按钮组件 - 可快速插拔使用
 * 
 * 提供完整的按钮功能，包括：
 * - 位置和大小管理（支持绝对位置和相对位置）
 * - 颜色和纹理渲染（支持纯颜色或纹理贴图）
 * - 文本渲染（可选，需要提供 TextRenderer）
 * - 点击检测和回调处理
 * - 悬停效果（可选）
 * - 多种渲染模式（传统方式和纯shader方式）
 * 
 * 设计意图：
 * - 通过依赖注入接收 IRenderContext，避免直接依赖 Vulkan 实现
 * - 支持快速创建和配置，一行代码即可初始化
 * - 支持多种拉伸模式（Fit、Scaled、Disabled等）
 * - 资源管理使用智能指针，自动清理
 */
class Button {
public:
    Button();
    ~Button();
    
    /**
     * 初始化按钮（一行代码即可使用）
     * 
     * 通过依赖注入接收渲染上下文，根据配置创建按钮资源
     * 支持两种渲染模式：传统方式（使用顶点缓冲区）和纯shader方式（全屏四边形）
     * 
     * @param renderContext 渲染上下文接口（不拥有所有权，由外部管理生命周期）
     * @param config 按钮配置（位置、大小、颜色、纹理等）
     * @param textRenderer 文字渲染器（可选，用于渲染按钮文本）
     * @param usePureShader 是否使用纯shader渲染模式（默认false，使用传统方式）
     * @return 初始化成功返回true，失败返回false
     */
    bool Initialize(IRenderContext* renderContext,
                    const ButtonConfig& config,
                    TextRenderer* textRenderer = nullptr,
                    bool usePureShader = false);
    
    /**
     * 便捷初始化方法 - 使用默认配置
     * 
     * 使用默认的 ButtonConfig 快速创建按钮
     * 
     * @param renderContext 渲染上下文接口
     * @param textRenderer 文字渲染器（可选）
     * @return 初始化成功返回true，失败返回false
     */
    bool Initialize(IRenderContext* renderContext,
                    TextRenderer* textRenderer = nullptr) {
        ButtonConfig defaultConfig;
        return Initialize(renderContext, defaultConfig, textRenderer);
    }
    
    /**
     * 清理资源
     * 
     * 释放所有渲染资源（缓冲区、管线、描述符等）
     * 应在按钮不再使用时调用，防止资源泄漏
     */
    void Cleanup();
    
    /**
     * 设置按钮位置（窗口坐标，Y向下，原点在左上角）
     * 
     * @param x X坐标（像素）
     * @param y Y坐标（像素）
     */
    void SetPosition(float x, float y) { 
        m_x = x; m_y = y; 
        m_useRelativePosition = false;
    }
    
    /**
     * 设置按钮大小
     * 
     * @param width 宽度（像素）
     * @param height 高度（像素）
     */
    void SetSize(float width, float height);
    
    /**
     * 设置按钮边界（位置和大小）
     * 
     * @param x X坐标（像素）
     * @param y Y坐标（像素）
     * @param width 宽度（像素）
     * @param height 高度（像素）
     */
    void SetBounds(float x, float y, float width, float height) {
        m_x = x; m_y = y; m_width = width; m_height = height;
        m_useRelativePosition = false;
    }
    
    /**
     * 设置按钮颜色（RGBA，0.0-1.0）
     * 
     * 注意：仅在不使用纹理时有效，如果已设置纹理则颜色会被忽略
     * 
     * @param r 红色分量（0.0-1.0）
     * @param g 绿色分量（0.0-1.0）
     * @param b 蓝色分量（0.0-1.0）
     * @param a 透明度分量（0.0-1.0，默认1.0）
     */
    void SetColor(float r, float g, float b, float a = 1.0f);
    
    /**
     * 设置纹理路径（如果设置纹理，将使用纹理而不是颜色）
     * 
     * 设置纹理后，按钮将使用纹理渲染，颜色设置会被忽略
     * 
     * @param texturePath 纹理文件路径
     */
    void SetTexture(const std::string& texturePath);
    
    /**
     * 设置按钮文本（需要先设置TextRenderer）
     * 
     * @param text 要显示的文本内容
     */
    void SetText(const std::string& text) {
        m_text = text;
        m_enableText = !text.empty();
    }
    
    /**
     * 设置文本颜色（RGBA，0.0-1.0）
     * 
     * @param r 红色分量（0.0-1.0）
     * @param g 绿色分量（0.0-1.0）
     * @param b 蓝色分量（0.0-1.0）
     * @param a 透明度分量（0.0-1.0，默认1.0）
     */
    void SetTextColor(float r, float g, float b, float a = 1.0f) {
        m_textColorR = r; m_textColorG = g; m_textColorB = b; m_textColorA = a;
    }
    
    /**
     * 启用/禁用文本显示
     * 
     * @param enabled true启用文本，false禁用文本
     */
    void SetTextEnabled(bool enabled) {
        m_enableText = enabled && !m_text.empty();
    }
    
    /**
     * 设置文字渲染器（用于渲染按钮文本）
     * 
     * @param textRenderer 文字渲染器指针（不拥有所有权，由外部管理生命周期）
     */
    void SetTextRenderer(TextRenderer* textRenderer) {
        m_textRenderer = textRenderer;
    }
    
    /**
     * 设置按钮在屏幕上的相对位置（0.0-1.0，相对于屏幕宽高）
     * 
     * 使用相对位置可以让按钮自动适应窗口大小变化
     * 例如：SetRelativePosition(0.5f, 0.75f) 表示屏幕中央下方75%位置
     * 
     * @param relX 相对X坐标（0.0-1.0，0.0=左边缘，1.0=右边缘，0.5=中央）
     * @param relY 相对Y坐标（0.0-1.0，0.0=上边缘，1.0=下边缘，0.5=中央）
     * @param screenWidth 屏幕宽度（可选，如果提供则立即更新位置）
     * @param screenHeight 屏幕高度（可选，如果提供则立即更新位置）
     */
    void SetRelativePosition(float relX, float relY, float screenWidth = 0.0f, float screenHeight = 0.0f) {
        m_relativeX = relX;
        m_relativeY = relY;
        m_useRelativePosition = true;
        if (screenWidth > 0.0f && screenHeight > 0.0f) {
            m_screenWidth = screenWidth;
            m_screenHeight = screenHeight;
            UpdateRelativePosition();
        }
    }
    
    /**
     * 更新窗口大小（用于相对位置计算）
     * 
     * 当窗口大小改变时调用此方法，按钮会根据相对位置自动调整
     * 注意：如果fixedScreenSize为true，则不更新（用于FIT模式）
     * 如果m_stretchParams不为空，则不更新（用于Scaled模式，通过SetStretchParams更新）
     * 
     * @param screenWidth 新的屏幕宽度
     * @param screenHeight 新的屏幕高度
     */
    void UpdateScreenSize(float screenWidth, float screenHeight);
    
    /**
     * 设置固定screenSize模式（用于FIT模式，UI不响应窗口变化）
     * 
     * 当设置为true时，按钮位置不会随窗口大小变化而更新
     * 
     * @param fixed true表示固定大小，false表示响应窗口变化
     */
    void SetFixedScreenSize(bool fixed) {
        m_fixedScreenSize = fixed;
    }
    
    /**
     * 获取按钮X坐标
     * @return X坐标（像素）
     */
    float GetX() const { return m_x; }
    
    /**
     * 获取按钮Y坐标
     * @return Y坐标（像素）
     */
    float GetY() const { return m_y; }
    
    /**
     * 获取按钮宽度
     * @return 宽度（像素）
     */
    float GetWidth() const { return m_width; }
    
    /**
     * 获取按钮高度
     * @return 高度（像素）
     */
    float GetHeight() const { return m_height; }
    
    /**
     * 设置渲染层级（z-index，数值越大越在上层）
     * 
     * @param zIndex 层级值，数值越大越在上层
     */
    void SetZIndex(int zIndex) { m_zIndex = zIndex; }
    
    /**
     * 获取渲染层级
     * @return 层级值
     */
    int GetZIndex() const { return m_zIndex; }
    
    /**
     * 设置可见性（是否渲染）
     * 
     * @param visible true表示可见（渲染），false表示不可见（不渲染）
     */
    void SetVisible(bool visible) { m_visible = visible; }
    
    /**
     * 获取可见性
     * @return true表示可见，false表示不可见
     */
    bool IsVisible() const { return m_visible; }
    
    /**
     * 设置按钮形状类型
     * 
     * @param shapeType 形状类型：0=矩形，1=圆形
     */
    void SetShapeType(int shapeType) { m_shapeType = shapeType; }
    
    /**
     * 获取按钮形状类型
     * @return 形状类型：0=矩形，1=圆形
     */
    int GetShapeType() const { return m_shapeType; }
    
    /**
     * 设置悬停效果（启用/禁用、效果类型、效果强度）
     * 
     * @param enable 是否启用悬停效果
     * @param effectType 效果类型：0=变暗，1=变淡
     * @param strength 效果强度（0.0-1.0），默认0.2（20%）
     */
    void SetHoverEffect(bool enable, int effectType = 0, float strength = 0.2f) {
        m_enableHoverEffect = enable;
        m_hoverEffectType = effectType;
        m_hoverEffectStrength = strength;
        if (!enable) {
            m_isHovering = false;
        }
        UpdateButtonBuffer(); // 更新缓冲区
    }
    
    /**
     * 检查是否有纹理
     * 
     * @return true表示已加载纹理，false表示未加载纹理
     */
    bool HasTexture() const;
    
    /**
     * 检测点是否在按钮内（窗口坐标）
     * 
     * 如果使用纹理，会根据纹理的alpha通道判断点击是否有效
     * 只有alpha值大于阈值的像素才被认为是有效的点击区域
     * 
     * @param px 点的X坐标（窗口坐标）
     * @param py 点的Y坐标（窗口坐标）
     * @return true表示点在按钮内，false表示点在按钮外
     */
    bool IsPointInside(float px, float py) const;
    
    /**
     * 渲染按钮到命令缓冲区（使用传统方式或纯shader方式，取决于初始化时的选择）
     * 
     * 根据初始化时选择的渲染模式，调用相应的渲染方法
     * 
     * @param commandBuffer 命令缓冲区句柄
     * @param extent 渲染区域大小
     */
    void Render(CommandBufferHandle commandBuffer, Extent2D extent);
    
    /**
     * 纯shader方式渲染按钮（在片段着色器中判断像素是否在按钮区域内）
     * 
     * 使用全屏四边形，在片段着色器中计算每个像素是否在按钮区域内
     * 这种方式避免了顶点缓冲区的创建和管理
     * 
     * @param commandBuffer 命令缓冲区句柄
     * @param extent 渲染区域大小
     */
    void RenderPureShader(CommandBufferHandle commandBuffer, Extent2D extent);
    
    /**
     * 渲染按钮文本（单独调用，确保在所有其他元素之后渲染）
     * 
     * 文本应在所有其他UI元素之后渲染，以确保文本显示在最上层
     * 
     * @param commandBuffer 命令缓冲区句柄
     * @param extent 渲染区域大小
     * @param viewport 视口参数（可选）
     * @param scissor 裁剪区域参数（可选）
     */
    void RenderText(CommandBufferHandle commandBuffer, Extent2D extent, 
                    const void* viewport = nullptr, const void* scissor = nullptr);
    
    /**
     * 设置点击回调函数
     * 
     * 当按钮被点击时，会调用此回调函数
     * 
     * @param callback 回调函数，无参数无返回值
     */
    void SetOnClickCallback(std::function<void()> callback) {
        m_onClickCallback = callback;
    }
    
    /**
     * 处理鼠标点击（在窗口消息循环中调用）
     * 
     * 检测点击位置是否在按钮内，如果是则调用回调函数
     * 
     * @param clickX 点击的X坐标（窗口坐标）
     * @param clickY 点击的Y坐标（窗口坐标）
     * @return true表示点击了按钮，false表示未点击按钮
     */
    bool HandleClick(float clickX, float clickY) {
        if (IsPointInside(clickX, clickY)) {
            if (m_onClickCallback) {
                m_onClickCallback();
            }
            return true;
        }
        return false;
    }
    
    /**
     * 处理鼠标移动（在窗口消息循环中调用，用于检测悬停状态）
     * 
     * 如果启用了悬停效果，会检测鼠标是否在按钮上并更新悬停状态
     * 
     * @param mouseX 鼠标的X坐标（窗口坐标）
     * @param mouseY 鼠标的Y坐标（窗口坐标）
     * @return true表示鼠标在按钮上，false表示鼠标不在按钮上
     */
    bool HandleMouseMove(float mouseX, float mouseY) {
        if (m_enableHoverEffect) {
            bool wasHovering = m_isHovering;
            m_isHovering = IsPointInside(mouseX, mouseY);
            // 如果悬停状态改变，需要更新渲染
            if (wasHovering != m_isHovering) {
                UpdateButtonBuffer(); // 更新缓冲区以应用悬停效果
            }
            return m_isHovering;
        }
        return false;
    }
    
    /**
     * 更新按钮位置以适应窗口大小变化（保持相对位置）
     * 
     * 当窗口大小改变时调用此方法，按钮会根据相对位置自动调整
     * 
     * @param newWidth 新的窗口宽度
     * @param newHeight 新的窗口高度
     */
    void UpdateForWindowResize(float newWidth, float newHeight) {
        UpdateScreenSize(newWidth, newHeight);
    }
    
    /**
     * 设置Scaled模式的拉伸参数（已废弃）
     * 
     * @deprecated Scaled模式已废弃，建议使用其他拉伸模式
     * @param params 拉伸参数
     */
    void SetStretchParams(const struct StretchParams& params);

private:
    // 更新相对位置
    void UpdateRelativePosition();
    
    // 加载纹理（如果使用纹理）
    bool LoadTexture(const std::string& texturePath);
    
    // 清理纹理资源
    void CleanupTexture();
    // 创建按钮顶点缓冲区
    bool CreateButtonBuffer();
    
    // 更新按钮缓冲区（当颜色改变时）
    void UpdateButtonBuffer();
    
    // 查找内存类型
    uint32_t FindMemoryType(uint32_t typeFilter, MemoryPropertyFlag properties);
    
    // 创建图形管线（传统方式）
    bool CreatePipeline(RenderPassHandle renderPass);
    
    // 创建纯shader图形管线
    bool CreatePureShaderPipeline(RenderPassHandle renderPass);
    
    // 创建描述符集布局（用于纹理绑定）
    bool CreateDescriptorSetLayout();
    
    // 创建描述符池和描述符集
    bool CreateDescriptorSet();
    
    // 创建全屏四边形顶点缓冲区（用于纯shader渲染）
    bool CreateFullscreenQuadBuffer();
    
    /**
     * 渲染上下文（新接口）
     * 
     * 通过依赖注入接收，不拥有所有权，由外部管理生命周期
     * 提供渲染所需的设备、命令池、队列等资源
     */
    IRenderContext* m_renderContext = nullptr;
    
    /**
     * 渲染设备句柄（通过渲染上下文获取，使用抽象类型）
     * 
     * 注意：在 .cpp 文件中转换为 Vulkan 类型使用
     * 使用抽象类型避免头文件直接依赖 Vulkan 实现
     */
    DeviceHandle m_device = nullptr;
    PhysicalDeviceHandle m_physicalDevice = nullptr;
    CommandPoolHandle m_commandPool = nullptr;
    QueueHandle m_graphicsQueue = nullptr;
    RenderPassHandle m_renderPass = nullptr;
    Extent2D m_swapchainExtent = {};
    
    // 按钮属性
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 200.0f;
    float m_height = 50.0f;
    float m_colorR = 1.0f;
    float m_colorG = 0.0f;
    float m_colorB = 0.0f;
    float m_colorA = 1.0f;
    
    // 相对位置相关
    bool m_useRelativePosition = false;
    float m_relativeX = 0.5f;
    float m_relativeY = 0.75f;
    float m_screenWidth = 0.0f;
    float m_screenHeight = 0.0f;
    bool m_fixedScreenSize = false;  // FIT模式：固定screenSize，不响应窗口变化
    
    // Scaled 模式的拉伸参数（已废弃）
    std::unique_ptr<StretchParams> m_stretchParams;  // 如果非空，使用Scaled模式（已废弃）
    
    /**
     * 纹理相关
     * 
     * 纹理用于按钮的视觉渲染，当设置了纹理时，颜色设置会被忽略
     * 纹理数据同时用于点击判定，实现不规则形状按钮的精确检测
     */
    std::string m_texturePath = "";  // 纹理文件路径
    bool m_useTexture = false;       // 是否使用纹理渲染（传统渲染方式需要）
    std::unique_ptr<renderer::texture::Texture> m_texture;  // Vulkan纹理对象（拥有所有权）
    
    /**
     * 描述符相关资源（用于纹理绑定）
     * 
     * 注意：以下成员变量在 .cpp 文件中使用 Vulkan 类型，头文件中使用不透明指针
     * 使用不透明指针避免头文件直接依赖 Vulkan 实现
     */
    void* m_descriptorSet = nullptr;   // 描述符集（用于绑定纹理到shader）
    void* m_descriptorPool = nullptr;  // 描述符池（用于分配描述符集）
    
    /**
     * 纹理图像数据（用于点击判定）
     * 
     * 存储纹理的原始像素数据（RGBA格式），用于精确的点击检测
     * 当使用纹理时，只有alpha值大于阈值的像素才被认为是有效的点击区域
     * 这样可以实现不规则形状按钮的精确点击检测
     */
    struct TextureData {
        std::vector<uint8_t> pixels;  // RGBA格式的像素数据，每个像素4字节
        uint32_t width = 0;           // 纹理宽度（像素）
        uint32_t height = 0;          // 纹理高度（像素）
        
        /**
         * 获取指定位置的alpha值
         * 
         * @param x X坐标（像素）
         * @param y Y坐标（像素）
         * @return alpha值（0-255），如果坐标无效则返回0
         */
        uint8_t GetAlpha(uint32_t x, uint32_t y) const {
            if (x >= width || y >= height || pixels.empty()) return 0;
            uint32_t index = (y * width + x) * 4;
            if (index + 3 < pixels.size()) {
                return pixels[index + 3];
            }
            return 0;
        }
        
        /**
         * 检查指定位置是否不透明
         * 
         * @param x X坐标（像素）
         * @param y Y坐标（像素）
         * @param threshold 透明度阈值（默认128），alpha值大于此值则认为不透明
         * @return true表示不透明，false表示透明或无效
         */
        bool IsOpaque(uint32_t x, uint32_t y, uint8_t threshold = 128) const {
            return GetAlpha(x, y) > threshold;
        }
    };
    TextureData m_textureData;  // 纹理像素数据，用于点击判定
    bool m_useTextureHitTest = false;  // 是否使用纹理进行点击判定（当加载了纹理且需要精确检测时启用）
    
    // 文本相关
    bool m_enableText = false;
    std::string m_text = "";
    float m_textColorR = 1.0f;
    float m_textColorG = 1.0f;
    float m_textColorB = 1.0f;
    float m_textColorA = 1.0f;
    TextRenderer* m_textRenderer = nullptr;
    
    // 渲染层级（z-index，数值越大越在上层）
    int m_zIndex = 0;
    
    // 可见性（是否渲染）
    bool m_visible = true;
    
    // 按钮形状类型（0=矩形，1=圆形）
    int m_shapeType = 0;
    
    /**
     * 渲染资源（传统方式）
     * 
     * 传统渲染方式使用顶点缓冲区存储按钮的顶点数据
     * 注意：以下成员变量在 .cpp 文件中使用 Vulkan 类型，头文件中使用不透明指针
     * 使用不透明指针避免头文件直接依赖 Vulkan 实现
     */
    void* m_vertexBuffer = nullptr;          // 顶点缓冲区（存储按钮顶点数据）
    void* m_vertexBufferMemory = nullptr;   // 顶点缓冲区内存（拥有所有权）
    void* m_graphicsPipeline = nullptr;      // 图形管线（定义渲染状态和shader）
    void* m_pipelineLayout = nullptr;         // 管线布局（定义描述符集布局）
    void* m_descriptorSetLayout = nullptr;   // 描述符集布局（定义纹理绑定方式）
    
    /**
     * 纯shader渲染资源
     * 
     * 纯shader方式使用全屏四边形，在片段着色器中计算每个像素是否在按钮区域内
     * 这种方式避免了顶点缓冲区的创建和管理，但需要更多的shader计算
     */
    bool m_usePureShader = false;                    // 是否使用纯shader渲染模式
    void* m_fullscreenQuadBuffer = nullptr;          // 全屏四边形顶点缓冲区
    void* m_fullscreenQuadBufferMemory = nullptr;    // 全屏四边形缓冲区内存（拥有所有权）
    void* m_pureShaderPipeline = nullptr;            // 纯shader图形管线
    void* m_pureShaderPipelineLayout = nullptr;      // 纯shader管线布局
    
    // 点击回调
    std::function<void()> m_onClickCallback;
    
    // 悬停效果相关
    bool m_enableHoverEffect = false;  // 是否启用悬停效果
    int m_hoverEffectType = 0;         // 悬停效果类型：0=变暗, 1=变淡
    float m_hoverEffectStrength = 0.2f; // 悬停效果强度（0.0-1.0）
    bool m_isHovering = false;          // 当前是否悬停
    
    bool m_initialized = false;
};


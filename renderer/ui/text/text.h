#pragma once

#include <string>         // 2. 系统头文件

#include "core/interfaces/itext_renderer.h"  // 4. 项目头文件（接口）
#include "core/types/render_types.h"  // 4. 项目头文件（类型）

// 前向声明
class IRenderContext;

// 文本UI组件配置结构体
struct TextConfig {
    // 位置（窗口坐标，Y向下，原点在左上角）
    float x = 0.0f;
    float y = 0.0f;
    
    // 文本内容
    std::string text = "";
    
    // 文本颜色（RGBA，0.0-1.0）
    float colorR = 1.0f;
    float colorG = 1.0f;
    float colorB = 1.0f;
    float colorA = 1.0f;
    
    // 是否使用相对位置（0.0-1.0，相对于屏幕宽高）
    bool useRelativePosition = false;
    float relativeX = 0.5f;  // 0.5 = 屏幕中央
    float relativeY = 0.5f;  // 0.5 = 屏幕中央
    
    // 是否使用中心坐标（如果为true，x和y表示文本中心位置；如果为false，表示左上角位置）
    bool useCenterPosition = false;
    
    // 构造函数
    TextConfig() = default;
    
    // 使用绝对位置（左上角坐标）
    TextConfig(float x, float y, const std::string& text,
               float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
        : x(x), y(y), text(text),
          colorR(r), colorG(g), colorB(b), colorA(a),
          useRelativePosition(false), useCenterPosition(false) {}
    
    // 使用相对位置（左上角坐标）
    static TextConfig CreateRelative(float relX, float relY, const std::string& text,
                                     float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) {
        TextConfig config;
        config.relativeX = relX;
        config.relativeY = relY;
        config.text = text;
        config.colorR = r;
        config.colorG = g;
        config.colorB = b;
        config.colorA = a;
        config.useRelativePosition = true;
        config.useCenterPosition = false;
        return config;
    }
    
    // 使用绝对位置（中心坐标）
    static TextConfig CreateCentered(float centerX, float centerY, const std::string& text,
                                     float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) {
        TextConfig config;
        config.x = centerX;
        config.y = centerY;
        config.text = text;
        config.colorR = r;
        config.colorG = g;
        config.colorB = b;
        config.colorA = a;
        config.useRelativePosition = false;
        config.useCenterPosition = true;
        return config;
    }
    
    // 使用相对位置（中心坐标）
    static TextConfig CreateRelativeCentered(float relX, float relY, const std::string& text,
                                             float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f) {
        TextConfig config;
        config.relativeX = relX;
        config.relativeY = relY;
        config.text = text;
        config.colorR = r;
        config.colorG = g;
        config.colorB = b;
        config.colorA = a;
        config.useRelativePosition = true;
        config.useCenterPosition = true;
        return config;
    }
};

/**
 * 独立的文本UI组件 - 可快速插拔使用
 * 
 * 提供文本渲染功能，支持绝对位置和相对位置，支持中心坐标和左上角坐标
 * 通过依赖注入接收 ITextRenderer 接口，避免直接依赖具体实现类
 */
class Text {
public:
    Text();
    ~Text();
    
    /**
     * 初始化文本组件
     * 
     * 通过依赖注入接收渲染上下文和文本渲染器接口
     * 
     * @param renderContext 渲染上下文接口（不拥有所有权，由外部管理生命周期）
     * @param config 文本配置（位置、内容、颜色等）
     * @param textRenderer 文字渲染器接口（不拥有所有权，由外部管理生命周期）
     * @return 初始化成功返回true，失败返回false
     */
    bool Initialize(IRenderContext* renderContext,
                    const TextConfig& config,
                    ITextRenderer* textRenderer);
    
    // 清理资源
    void Cleanup();
    
    // 设置文本位置（窗口坐标，Y向下，原点在左上角）
    void SetPosition(float x, float y) { 
        m_x = x; m_y = y; 
        m_useRelativePosition = false;
        m_useCenterPosition = false;
    }
    
    // 设置文本内容
    void SetText(const std::string& text) {
        m_text = text;
    }
    
    // 设置文本颜色
    void SetColor(float r, float g, float b, float a = 1.0f) {
        m_colorR = r; m_colorG = g; m_colorB = b; m_colorA = a;
    }
    
    // 设置文本在屏幕上的相对位置（0.0-1.0，相对于屏幕宽高）
    void SetRelativePosition(float relX, float relY, float screenWidth = 0.0f, float screenHeight = 0.0f);
    
    // 设置是否使用中心坐标
    void SetUseCenterPosition(bool useCenter) {
        m_useCenterPosition = useCenter;
    }
    
    // 更新窗口大小（用于相对位置计算）
    void UpdateScreenSize(float screenWidth, float screenHeight);
    
    // 获取文本位置
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    
    // 获取文本内容
    const std::string& GetText() const { return m_text; }
    
    /**
     * 渲染文本到命令缓冲区
     * 
     * @param commandBuffer 命令缓冲区句柄
     * @param extent 渲染区域大小
     */
    void Render(CommandBufferHandle commandBuffer, Extent2D extent);
    
    // 更新文本位置以适应窗口大小变化（保持相对位置）
    void UpdateForWindowResize(float newWidth, float newHeight) {
        UpdateScreenSize(newWidth, newHeight);
    }

private:
    // 更新相对位置
    void UpdateRelativePosition();
    
    /**
     * 渲染上下文（新接口）
     * 
     * 通过依赖注入接收，不拥有所有权，由外部管理生命周期
     * 提供渲染所需的设备、命令池、队列等资源
     */
    IRenderContext* m_renderContext = nullptr;
    
    /**
     * 渲染设备对象（使用抽象类型，在实现层转换为Vulkan类型）
     * 
     * 注意：在 .cpp 文件中转换为 Vulkan 类型使用
     * 使用抽象类型避免头文件直接依赖 Vulkan 实现
     */
    Extent2D m_swapchainExtent = {};
    
    // 文本属性
    float m_x = 0.0f;
    float m_y = 0.0f;
    std::string m_text = "";
    float m_colorR = 1.0f;
    float m_colorG = 1.0f;
    float m_colorB = 1.0f;
    float m_colorA = 1.0f;
    
    // 相对位置相关
    bool m_useRelativePosition = false;
    float m_relativeX = 0.5f;
    float m_relativeY = 0.5f;
    float m_screenWidth = 0.0f;
    float m_screenHeight = 0.0f;
    
    // 是否使用中心坐标
    bool m_useCenterPosition = false;
    
    /**
     * 文字渲染器接口
     * 
     * 通过依赖注入接收，不拥有所有权，由外部管理生命周期
     * 提供文本渲染功能
     */
    ITextRenderer* m_textRenderer = nullptr;
    
    bool m_initialized = false;
};


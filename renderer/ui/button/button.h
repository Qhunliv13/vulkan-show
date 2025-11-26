#pragma once

#include <functional>    // 2. 系统头文件
#include <memory>        // 2. 系统头文件
#include <string>        // 2. 系统头文件

#include <vulkan/vulkan.h>  // 3. 第三方库头文件

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

// 独立的按钮组件 - 可快速插拔使用
class Button {
public:
    Button();
    ~Button();
    
    // 初始化按钮（一行代码即可使用）
    // 参数：渲染上下文、按钮配置、文字渲染器（可选）、是否使用纯shader渲染（默认false）
    // 返回：是否初始化成功
    bool Initialize(IRenderContext* renderContext,
                    const ButtonConfig& config,
                    TextRenderer* textRenderer = nullptr,
                    bool usePureShader = false);
    
    // 便捷初始化方法 - 使用默认配置
    bool Initialize(IRenderContext* renderContext,
                    TextRenderer* textRenderer = nullptr) {
        ButtonConfig defaultConfig;
        return Initialize(renderContext, defaultConfig, textRenderer);
    }
    
    // 兼容旧接口的初始化方法（已废弃，建议使用新接口）
    [[deprecated("Use Initialize(IRenderContext*, ...) instead")]]
    bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice, 
                    VkCommandPool commandPool, VkQueue graphicsQueue, 
                    VkRenderPass renderPass, VkExtent2D swapchainExtent,
                    const ButtonConfig& config,
                    TextRenderer* textRenderer = nullptr,
                    bool usePureShader = false);
    
    // 清理资源
    void Cleanup();
    
    // 设置按钮位置和大小（窗口坐标，Y向下，原点在左上角）
    void SetPosition(float x, float y) { 
        m_x = x; m_y = y; 
        m_useRelativePosition = false;
    }
    void SetSize(float width, float height);
    void SetBounds(float x, float y, float width, float height) {
        m_x = x; m_y = y; m_width = width; m_height = height;
        m_useRelativePosition = false;
    }
    
    // 设置按钮颜色（RGBA，0.0-1.0）- 仅在不使用纹理时有效
    void SetColor(float r, float g, float b, float a = 1.0f);
    
    // 设置纹理路径（如果设置纹理，将使用纹理而不是颜色）
    void SetTexture(const std::string& texturePath);
    
    // 设置按钮文本（需要先设置TextRenderer）
    void SetText(const std::string& text) {
        m_text = text;
        m_enableText = !text.empty();
    }
    
    // 设置文本颜色
    void SetTextColor(float r, float g, float b, float a = 1.0f) {
        m_textColorR = r; m_textColorG = g; m_textColorB = b; m_textColorA = a;
    }
    
    // 启用/禁用文本
    void SetTextEnabled(bool enabled) {
        m_enableText = enabled && !m_text.empty();
    }
    
    // 设置文字渲染器（用于渲染按钮文本）
    void SetTextRenderer(TextRenderer* textRenderer) {
        m_textRenderer = textRenderer;
    }
    
    // 设置按钮在屏幕上的相对位置（0.0-1.0，相对于屏幕宽高）
    // 例如：SetRelativePosition(0.5f, 0.75f) 表示屏幕中央下方75%位置
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
    
    // 更新窗口大小（用于相对位置计算）
    // 如果fixedScreenSize为true，则不更新（用于FIT模式）
    // 如果m_stretchParams不为空，则不更新（用于Scaled模式，通过SetStretchParams更新）
    void UpdateScreenSize(float screenWidth, float screenHeight);
    
    // 设置固定screenSize模式（用于FIT模式，UI不响应窗口变化）
    void SetFixedScreenSize(bool fixed) {
        m_fixedScreenSize = fixed;
    }
    
    // 获取按钮位置和大小
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetWidth() const { return m_width; }
    float GetHeight() const { return m_height; }
    
    // 设置和获取渲染层级（z-index，数值越大越在上层）
    void SetZIndex(int zIndex) { m_zIndex = zIndex; }
    int GetZIndex() const { return m_zIndex; }
    
    // 设置和获取可见性（是否渲染）
    void SetVisible(bool visible) { m_visible = visible; }
    bool IsVisible() const { return m_visible; }
    
    // 设置和获取按钮形状类型（0=矩形，1=圆形）
    void SetShapeType(int shapeType) { m_shapeType = shapeType; }
    int GetShapeType() const { return m_shapeType; }
    
    // 设置悬停效果（启用/禁用、效果类型、效果强度）
    void SetHoverEffect(bool enable, int effectType = 0, float strength = 0.2f) {
        m_enableHoverEffect = enable;
        m_hoverEffectType = effectType;
        m_hoverEffectStrength = strength;
        if (!enable) {
            m_isHovering = false;
        }
        UpdateButtonBuffer(); // 更新缓冲区
    }
    
    // 检查是否有纹理
    bool HasTexture() const;
    
    // 检测点是否在按钮内（窗口坐标）
    // 如果使用纹理，会根据纹理的alpha通道判断点击是否有效
    bool IsPointInside(float px, float py) const;
    
    // 渲染按钮到命令缓冲区（使用传统方式或纯shader方式，取决于初始化时的选择）
    void Render(VkCommandBuffer commandBuffer, VkExtent2D extent);
    
    // 纯shader方式渲染按钮（在片段着色器中判断像素是否在按钮区域内）
    void RenderPureShader(VkCommandBuffer commandBuffer, VkExtent2D extent);
    
    // 渲染按钮文本（单独调用，确保在所有其他元素之后渲染）
    void RenderText(VkCommandBuffer commandBuffer, VkExtent2D extent, 
                    const VkViewport* viewport = nullptr, const VkRect2D* scissor = nullptr);
    
    // 设置点击回调函数
    void SetOnClickCallback(std::function<void()> callback) {
        m_onClickCallback = callback;
    }
    
    // 处理鼠标点击（在窗口消息循环中调用）
    // 返回：是否点击了按钮
    bool HandleClick(float clickX, float clickY) {
        if (IsPointInside(clickX, clickY)) {
            if (m_onClickCallback) {
                m_onClickCallback();
            }
            return true;
        }
        return false;
    }
    
    // 处理鼠标移动（在窗口消息循环中调用，用于检测悬停状态）
    // 返回：鼠标是否在按钮上
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
    
    // 更新按钮位置以适应窗口大小变化（保持相对位置）
    void UpdateForWindowResize(float newWidth, float newHeight) {
        UpdateScreenSize(newWidth, newHeight);
    }
    
    // 设置Scaled模式的拉伸参数（已废弃）
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
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    // 创建图形管线（传统方式）
    bool CreatePipeline(VkRenderPass renderPass);
    
    // 创建纯shader图形管线
    bool CreatePureShaderPipeline(VkRenderPass renderPass);
    
    // 创建描述符集布局（用于纹理绑定）
    bool CreateDescriptorSetLayout();
    
    // 创建描述符池和描述符集
    bool CreateDescriptorSet();
    
    // 创建全屏四边形顶点缓冲区（用于纯shader渲染）
    bool CreateFullscreenQuadBuffer();
    
    // 渲染上下文（新接口）
    IRenderContext* m_renderContext = nullptr;
    
    // Vulkan对象（通过渲染上下文获取，保留用于向后兼容）
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkExtent2D m_swapchainExtent = {};
    
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
    
    // 纹理相关
    std::string m_texturePath = "";
    bool m_useTexture = false;
    std::unique_ptr<renderer::texture::Texture> m_texture;  // Vulkan纹理对象
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;  // 描述符集（用于绑定纹理）
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;  // 描述符池
    
    // 纹理图像数据（用于点击判定）
    struct TextureData {
        std::vector<uint8_t> pixels;  // RGBA格式
        uint32_t width = 0;
        uint32_t height = 0;
        
        // 获取指定位置的alpha值
        uint8_t GetAlpha(uint32_t x, uint32_t y) const {
            if (x >= width || y >= height || pixels.empty()) return 0;
            uint32_t index = (y * width + x) * 4;
            if (index + 3 < pixels.size()) {
                return pixels[index + 3];
            }
            return 0;
        }
        
        // 检查是否不透明
        bool IsOpaque(uint32_t x, uint32_t y, uint8_t threshold = 128) const {
            return GetAlpha(x, y) > threshold;
        }
    };
    TextureData m_textureData;
    bool m_useTextureHitTest = false;  // 是否使用纹理进行点击判定
    
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
    
    // 渲染资源（传统方式）
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    
    // 纯shader渲染资源
    bool m_usePureShader = false;
    VkBuffer m_fullscreenQuadBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_fullscreenQuadBufferMemory = VK_NULL_HANDLE;
    VkPipeline m_pureShaderPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pureShaderPipelineLayout = VK_NULL_HANDLE;
    
    // 点击回调
    std::function<void()> m_onClickCallback;
    
    // 悬停效果相关
    bool m_enableHoverEffect = false;  // 是否启用悬停效果
    int m_hoverEffectType = 0;         // 悬停效果类型：0=变暗, 1=变淡
    float m_hoverEffectStrength = 0.2f; // 悬停效果强度（0.0-1.0）
    bool m_isHovering = false;          // 当前是否悬停
    
    bool m_initialized = false;
};


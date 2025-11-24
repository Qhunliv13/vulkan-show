#pragma once

#include <vulkan/vulkan.h>
#include <string>

// 前向声明
class TextRenderer;

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

// 独立的文本UI组件 - 可快速插拔使用
class Text {
public:
    Text();
    ~Text();
    
    // 初始化文本组件
    // 参数：Vulkan设备、物理设备、命令池、图形队列、渲染通道、交换链范围、文本配置、文字渲染器
    // 返回：是否初始化成功
    bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice, 
                    VkCommandPool commandPool, VkQueue graphicsQueue, 
                    VkRenderPass renderPass, VkExtent2D swapchainExtent,
                    const TextConfig& config,
                    TextRenderer* textRenderer);
    
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
    
    // 渲染文本到命令缓冲区
    void Render(VkCommandBuffer commandBuffer, VkExtent2D extent);
    
    // 更新文本位置以适应窗口大小变化（保持相对位置）
    void UpdateForWindowResize(float newWidth, float newHeight) {
        UpdateScreenSize(newWidth, newHeight);
    }

private:
    // 更新相对位置
    void UpdateRelativePosition();
    
    // Vulkan对象（用于存储屏幕尺寸等信息）
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    VkExtent2D m_swapchainExtent = {};
    
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
    
    // 文字渲染器
    TextRenderer* m_textRenderer = nullptr;
    
    bool m_initialized = false;
};


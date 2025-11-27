#pragma once

#include <functional>    // 2. 系统头文件
#include <memory>         // 2. 系统头文件
#include <string>         // 2. 系统头文件

#include <vulkan/vulkan.h>  // 3. 第三方库头文件

// 前向声明
class Button;
class IRenderContext;

// 滑块配置结构体
struct SliderConfig {
    // 位置和大小（窗口坐标，Y向下，原点在左上角）
    float x = 0.0f;
    float y = 0.0f;
    float width = 300.0f;
    float height = 20.0f;
    
    // 滑块轨道颜色（RGBA，0.0-1.0）
    float trackColorR = 0.3f;
    float trackColorG = 0.3f;
    float trackColorB = 0.3f;
    float trackColorA = 1.0f;
    
    // 滑块填充颜色（已选择部分的颜色，RGBA，0.0-1.0）
    float fillColorR = 0.5f;
    float fillColorG = 0.5f;
    float fillColorB = 1.0f;
    float fillColorA = 1.0f;
    
    // 拖动点（thumb）大小
    float thumbWidth = 20.0f;
    float thumbHeight = 20.0f;
    
    // 拖动点颜色（RGBA，0.0-1.0）- 如果使用按钮，这个颜色可能被按钮覆盖
    float thumbColorR = 1.0f;
    float thumbColorG = 1.0f;
    float thumbColorB = 1.0f;
    float thumbColorA = 1.0f;
    
    // 拖动点纹理路径（可选，如果为空则使用颜色）
    std::string thumbTexturePath = "";
    
    // 是否使用相对位置（0.0-1.0，相对于屏幕宽高）
    bool useRelativePosition = false;
    float relativeX = 0.5f;  // 0.5 = 屏幕中央
    float relativeY = 0.5f;  // 0.5 = 屏幕中央
    
    // 滑块值范围
    float minValue = 0.0f;
    float maxValue = 100.0f;
    float defaultValue = 50.0f;
    
    // 渲染层级（z-index，数值越大越在上层，默认0）
    int zIndex = 0;
    
    // 构造函数
    SliderConfig() = default;
    
    // 使用绝对位置
    SliderConfig(float x, float y, float width, float height,
                 float minVal = 0.0f, float maxVal = 100.0f, float defaultVal = 50.0f)
        : x(x), y(y), width(width), height(height),
          minValue(minVal), maxValue(maxVal), defaultValue(defaultVal),
          useRelativePosition(false) {}
    
    // 使用相对位置
    static SliderConfig CreateRelative(float relX, float relY, float width, float height,
                                       float minVal = 0.0f, float maxVal = 100.0f, float defaultVal = 50.0f) {
        SliderConfig config;
        config.width = width;
        config.height = height;
        config.minValue = minVal;
        config.maxValue = maxVal;
        config.defaultValue = defaultVal;
        config.useRelativePosition = true;
        config.relativeX = relX;
        config.relativeY = relY;
        return config;
    }
};

// 独立的滑块组件
class Slider {
public:
    Slider();
    ~Slider();
    
    // 初始化滑块
    // 参数：渲染上下文、滑块配置、是否使用纯shader渲染（默认false）
    // 返回：是否初始化成功
    bool Initialize(IRenderContext* renderContext,
                    const SliderConfig& config,
                    bool usePureShader = false);
    
    // 兼容旧接口的初始化方法（已废弃，建议使用新接口）
    [[deprecated("Use Initialize(IRenderContext*, ...) instead")]]
    bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice, 
                    VkCommandPool commandPool, VkQueue graphicsQueue, 
                    VkRenderPass renderPass, VkExtent2D swapchainExtent,
                    const SliderConfig& config,
                    bool usePureShader = false);
    
    // 清理资源
    void Cleanup();
    
    // 设置滑块位置和大小（窗口坐标，Y向下，原点在左上角）
    void SetPosition(float x, float y);
    void SetSize(float width, float height);
    
    // 设置滑块值（会自动限制在minValue和maxValue之间）
    void SetValue(float value);
    
    // 获取当前值
    float GetValue() const { return m_value; }
    
    // 获取归一化值（0.0-1.0）
    float GetNormalizedValue() const {
        if (m_maxValue == m_minValue) return 0.0f;
        return (m_value - m_minValue) / (m_maxValue - m_minValue);
    }
    
    // 设置值范围
    void SetRange(float minValue, float maxValue) {
        m_minValue = minValue;
        m_maxValue = maxValue;
        if (m_value < m_minValue) m_value = m_minValue;
        if (m_value > m_maxValue) m_value = m_maxValue;
        UpdateThumbPosition();
    }
    
    // 设置滑块轨道颜色
    void SetTrackColor(float r, float g, float b, float a = 1.0f) {
        m_trackColorR = r; m_trackColorG = g; m_trackColorB = b; m_trackColorA = a;
        UpdateTrackBuffer();
    }
    
    // 设置滑块填充颜色
    void SetFillColor(float r, float g, float b, float a = 1.0f) {
        m_fillColorR = r; m_fillColorG = g; m_fillColorB = b; m_fillColorA = a;
        UpdateFillBuffer();
    }
    
    // 设置拖动点颜色（如果使用按钮，这个颜色可能被按钮覆盖）
    void SetThumbColor(float r, float g, float b, float a = 1.0f);
    
    // 设置拖动点纹理
    void SetThumbTexture(const std::string& texturePath);
    
    // 设置滑块在屏幕上的相对位置（0.0-1.0，相对于屏幕宽高）
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
    void UpdateScreenSize(float screenWidth, float screenHeight) {
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;
        if (m_useRelativePosition) {
            UpdateRelativePosition();
        }
    }
    
    // 获取滑块位置和大小
    float GetX() const { return m_x; }
    float GetY() const { return m_y; }
    float GetWidth() const { return m_width; }
    float GetHeight() const { return m_height; }
    
    // 设置和获取渲染层级（z-index，数值越大越在上层）
    void SetZIndex(int zIndex);
    int GetZIndex() const { return m_zIndex; }
    
    // 设置和获取可见性（是否渲染）
    void SetVisible(bool visible);
    bool IsVisible() const { return m_visible; }
    
    // 检测点是否在滑块轨道内（窗口坐标）
    bool IsPointInsideTrack(float px, float py) const;
    
    // 检测点是否在拖动点内（窗口坐标）
    bool IsPointInsideThumb(float px, float py) const;
    
    // 根据鼠标位置设置滑块值（用于拖动）
    void SetValueFromPosition(float px, float py);
    
    // 渲染滑块到命令缓冲区
    void Render(VkCommandBuffer commandBuffer, VkExtent2D extent);
    
    // 设置值变化回调函数
    void SetOnValueChangedCallback(std::function<void(float)> callback) {
        m_onValueChangedCallback = callback;
    }
    
    // 处理鼠标按下（在窗口消息循环中调用）
    // 返回：是否点击了滑块
    bool HandleMouseDown(float clickX, float clickY);
    
    // 处理鼠标移动（在窗口消息循环中调用，用于拖动）
    // 返回：是否正在拖动滑块
    bool HandleMouseMove(float mouseX, float mouseY);
    
    // 处理鼠标释放（在窗口消息循环中调用）
    void HandleMouseUp();
    
    // 更新滑块位置以适应窗口大小变化（保持相对位置）
    void UpdateForWindowResize(float newWidth, float newHeight) {
        UpdateScreenSize(newWidth, newHeight);
    }
    
    // 设置Scaled模式的拉伸参数（已废弃）
    void SetStretchParams(const struct StretchParams& params);

private:
    // 更新相对位置
    void UpdateRelativePosition();
    
    // 更新拖动点位置（根据当前值）
    void UpdateThumbPosition();
    
    // 创建轨道顶点缓冲区
    bool CreateTrackBuffer();
    
    // 创建填充区域顶点缓冲区
    bool CreateFillBuffer();
    
    // 更新轨道缓冲区（当颜色改变时）
    void UpdateTrackBuffer();
    
    // 更新填充缓冲区（当颜色或值改变时）
    void UpdateFillBuffer();
    
    // 查找内存类型
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    // 创建图形管线
    bool CreatePipeline(VkRenderPass renderPass);
    
    // 创建纯shader图形管线
    bool CreatePureShaderPipeline(VkRenderPass renderPass);
    
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
    bool m_usePureShader = false;
    
    // 滑块属性
    float m_x = 0.0f;
    float m_y = 0.0f;
    float m_width = 300.0f;
    float m_height = 20.0f;
    float m_trackColorR = 0.3f;
    float m_trackColorG = 0.3f;
    float m_trackColorB = 0.3f;
    float m_trackColorA = 1.0f;
    float m_fillColorR = 0.5f;
    float m_fillColorG = 0.5f;
    float m_fillColorB = 1.0f;
    float m_fillColorA = 1.0f;
    
    // 拖动点相关
    float m_thumbWidth = 20.0f;
    float m_thumbHeight = 20.0f;
    float m_thumbX = 0.0f;  // 拖动点的X位置（相对于滑块轨道）
    float m_thumbY = 0.0f;  // 拖动点的Y位置（相对于滑块轨道）
    std::unique_ptr<Button> m_thumbButton;  // 拖动点按钮（复用Button组件）
    
    // 相对位置相关
    bool m_useRelativePosition = false;
    float m_relativeX = 0.5f;
    float m_relativeY = 0.5f;
    float m_screenWidth = 0.0f;
    float m_screenHeight = 0.0f;
    bool m_fixedScreenSize = false;
    
    // Scaled 模式的拉伸参数（已废弃）
    std::unique_ptr<StretchParams> m_stretchParams;
    
    // 滑块值
    float m_value = 50.0f;
    float m_minValue = 0.0f;
    float m_maxValue = 100.0f;
    
    // 渲染层级（z-index，数值越大越在上层）
    int m_zIndex = 0;
    
    // 可见性（是否渲染）
    bool m_visible = true;
    
    // 拖动状态
    bool m_isDragging = false;
    
    // 渲染资源（传统方式）
    VkBuffer m_trackVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_trackVertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_fillVertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_fillVertexBufferMemory = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    
    // 纯shader渲染资源
    VkBuffer m_fullscreenQuadBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_fullscreenQuadBufferMemory = VK_NULL_HANDLE;
    VkPipeline m_pureShaderPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pureShaderPipelineLayout = VK_NULL_HANDLE;
    
    // 值变化回调
    std::function<void(float)> m_onValueChangedCallback;
    
    bool m_initialized = false;
};


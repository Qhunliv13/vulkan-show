#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <unordered_map>
#include "core/interfaces/itext_renderer.h"

// 文字渲染器 - 参考 Godot 的实现
// 使用 Windows GDI 生成字体纹理图集，然后在 Vulkan 中渲染
class TextRenderer : public ITextRenderer {
public:
    struct Glyph {
        uint32_t charCode;           // 字符代码
        float x, y;                  // 纹理坐标（归一化）
        float width, height;          // 纹理尺寸（归一化）
        float advanceX;               // 水平前进距离
        float offsetX, offsetY;       // 字符偏移（相对于基线）
        int textureIndex;             // 纹理索引（如果使用多个纹理）
    };
    
    struct TextVertex {
        float x, y;                   // 屏幕坐标
        float u, v;                   // 纹理坐标
        float r, g, b, a;             // 颜色
    };
    
    TextRenderer();
    ~TextRenderer();
    
    // ITextRenderer 接口实现
    bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice, 
                    VkCommandPool commandPool, VkQueue graphicsQueue,
                    VkRenderPass renderPass) override;
    
    void Cleanup() override;
    bool LoadFont(const std::string& fontName, int fontSize) override;
    void BeginTextBatch() override;
    void EndTextBatch(VkCommandBuffer commandBuffer, float screenWidth, float screenHeight,
                     float viewportX = 0.0f, float viewportY = 0.0f,
                     float scaleX = 1.0f, float scaleY = 1.0f) override;
    void AddTextToBatch(const std::string& text, float x, float y,
                       float r = 1.0f, float g = 1.0f, 
                       float b = 1.0f, float a = 1.0f) override;
    void AddTextCenteredToBatch(const std::string& text, float centerX, float centerY,
                                float screenWidth, float screenHeight,
                                float r = 1.0f, float g = 1.0f,
                                float b = 1.0f, float a = 1.0f) override;
    void RenderText(VkCommandBuffer commandBuffer, const std::string& text, 
                    float x, float y, float screenWidth, float screenHeight,
                    float r = 1.0f, float g = 1.0f, 
                    float b = 1.0f, float a = 1.0f) override;
    void RenderTextCentered(VkCommandBuffer commandBuffer, const std::string& text,
                            float centerX, float centerY, float screenWidth, float screenHeight,
                            float r = 1.0f, float g = 1.0f,
                            float b = 1.0f, float a = 1.0f) override;
    void GetTextSize(const std::string& text, float& width, float& height) override;
    float GetTextCenterOffset(const std::string& text) override;
    void SetFontSize(int fontSize) override;
    int GetFontSize() const override { return m_fontSize; }
    
private:
    // 查找内存类型
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    
    // 创建字体纹理图集
    bool CreateFontAtlas();
    
    // 创建 Vulkan 纹理
    bool CreateVulkanTexture(const void* data, uint32_t width, uint32_t height);
    
    // 创建渲染管线
    bool CreatePipeline(VkRenderPass renderPass);
    
    // 获取或创建字符字形
    const Glyph& GetGlyph(uint32_t charCode);
    
    // 创建字符缓冲区
    bool CreateVertexBuffer();
    
    // 更新顶点缓冲区（追加模式，不清除现有顶点）
    void AppendVerticesToBuffer(const std::string& text, float x, float y, 
                                float r, float g, float b, float a);
    
    // 更新顶点缓冲区（覆盖模式，清除现有顶点）
    void UpdateVertexBuffer(const std::string& text, float x, float y, 
                           float r, float g, float b, float a);
    
    // 将累积的顶点数据上传到GPU并渲染
    // viewportX/viewportY: viewport的偏移（在Fit模式下需要设置，用于正确对齐文本）
    // scaleX/scaleY: 文本缩放比例（在Fit模式下需要根据viewport和UI基准大小的比例设置）
    void FlushBatch(VkCommandBuffer commandBuffer, float screenWidth, float screenHeight,
                   float viewportX = 0.0f, float viewportY = 0.0f,
                   float scaleX = 1.0f, float scaleY = 1.0f);
    
    // Vulkan 对象
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkRenderPass m_renderPass = VK_NULL_HANDLE;
    
    // 字体相关
    std::string m_fontName;
    int m_fontSize = 16;
    HFONT m_hFont = nullptr;
    HDC m_hDC = nullptr;
    HBITMAP m_hBitmap = nullptr;
    void* m_bitmapData = nullptr;
    
    // 纹理图集
    uint32_t m_atlasWidth = 512;
    uint32_t m_atlasHeight = 512;
    std::vector<uint8_t> m_atlasData;
    VkImage m_textureImage = VK_NULL_HANDLE;
    VkDeviceMemory m_textureImageMemory = VK_NULL_HANDLE;
    VkImageView m_textureImageView = VK_NULL_HANDLE;
    VkSampler m_textureSampler = VK_NULL_HANDLE;
    
    // 字形缓存
    std::unordered_map<uint32_t, Glyph> m_glyphs;
    float m_currentX = 0.0f;
    float m_currentY = 0.0f;
    float m_lineHeight = 0.0f;
    
    // 渲染资源
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    VkPipeline m_graphicsPipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    
    // 批量渲染相关的临时顶点数据
    std::vector<TextVertex> m_batchVertices;
    bool m_inBatchMode = false;  // 是否处于批量渲染模式
    
    // 文本块信息（用于Fit模式下的缩放）
    struct TextBlockInfo {
        size_t startIndex;      // 文本块在m_batchVertices中的起始索引
        size_t endIndex;        // 文本块在m_batchVertices中的结束索引
        float centerX;          // 文本块中心点X坐标（窗口坐标，已转换）
        float centerY;          // 文本块中心点Y坐标（窗口坐标，已转换）
    };
    std::vector<TextBlockInfo> m_textBlocks;  // 记录每个文本块的信息
    
    bool m_initialized = false;
};


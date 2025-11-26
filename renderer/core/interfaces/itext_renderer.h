#pragma once

#include <vulkan/vulkan.h>
#include <string>

// 文字渲染器接口 - 用于解耦文字渲染与UI管理
class ITextRenderer {
public:
    virtual ~ITextRenderer() = default;
    
    // 初始化（需要 Vulkan 设备）
    virtual bool Initialize(VkDevice device, VkPhysicalDevice physicalDevice, 
                           VkCommandPool commandPool, VkQueue graphicsQueue,
                           VkRenderPass renderPass) = 0;
    
    // 清理资源
    virtual void Cleanup() = 0;
    
    // 加载字体（使用系统字体）
    virtual bool LoadFont(const std::string& fontName, int fontSize) = 0;
    
    // 开始一个新的文本渲染批次（清除累积的顶点）
    virtual void BeginTextBatch() = 0;
    
    // 结束文本渲染批次并渲染所有累积的文本
    virtual void EndTextBatch(VkCommandBuffer commandBuffer, float screenWidth, float screenHeight,
                             float viewportX = 0.0f, float viewportY = 0.0f,
                             float scaleX = 1.0f, float scaleY = 1.0f) = 0;
    
    // 添加文字到当前批次（不立即渲染）
    virtual void AddTextToBatch(const std::string& text, float x, float y,
                               float r = 1.0f, float g = 1.0f, 
                               float b = 1.0f, float a = 1.0f) = 0;
    
    // 添加居中文字到当前批次（不立即渲染）
    virtual void AddTextCenteredToBatch(const std::string& text, float centerX, float centerY,
                                        float screenWidth, float screenHeight,
                                        float r = 1.0f, float g = 1.0f,
                                        float b = 1.0f, float a = 1.0f) = 0;
    
    // 渲染文字到命令缓冲区（x, y是文字左上角坐标）
    virtual void RenderText(VkCommandBuffer commandBuffer, const std::string& text, 
                           float x, float y, float screenWidth, float screenHeight,
                           float r = 1.0f, float g = 1.0f, 
                           float b = 1.0f, float a = 1.0f) = 0;
    
    // 渲染文字到命令缓冲区（centerX, centerY是文字中心坐标）
    virtual void RenderTextCentered(VkCommandBuffer commandBuffer, const std::string& text,
                                   float centerX, float centerY, float screenWidth, float screenHeight,
                                   float r = 1.0f, float g = 1.0f,
                                   float b = 1.0f, float a = 1.0f) = 0;
    
    // 获取文字尺寸
    virtual void GetTextSize(const std::string& text, float& width, float& height) = 0;
    
    // 获取文字中心相对于传入Y坐标的偏移（用于垂直居中）
    virtual float GetTextCenterOffset(const std::string& text) = 0;
    
    // 设置字体大小
    virtual void SetFontSize(int fontSize) = 0;
    
    // 获取字体大小
    virtual int GetFontSize() const = 0;
};


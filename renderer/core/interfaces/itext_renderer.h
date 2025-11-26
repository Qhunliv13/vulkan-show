#pragma once

#include <string>  // 2. 系统头文件
#include "core/types/render_types.h"  // 4. 项目头文件（类型）

/**
 * 文字渲染器接口 - 用于解耦文字渲染与UI管理，不依赖具体渲染后端
 * 
 * 职责：提供平台无关的文字渲染接口，支持批量渲染和文本测量
 * 设计：使用抽象设备句柄，支持多种渲染后端（Vulkan、OpenGL、DirectX等）
 * 
 * 使用方式：
 * 1. 通过工厂接口创建实现（如 TextRendererFactory）
 * 2. 使用接口指针操作，无需了解具体实现
 * 3. 支持批量渲染模式，提高渲染效率
 */
class ITextRenderer {
public:
    virtual ~ITextRenderer() = default;
    
    // 初始化（使用抽象设备句柄）
    virtual bool Initialize(DeviceHandle device, PhysicalDeviceHandle physicalDevice, 
                           CommandPoolHandle commandPool, QueueHandle graphicsQueue,
                           RenderPassHandle renderPass) = 0;
    
    // 清理资源
    virtual void Cleanup() = 0;
    
    // 加载字体（使用系统字体）
    virtual bool LoadFont(const std::string& fontName, int fontSize) = 0;
    
    // 开始一个新的文本渲染批次（清除累积的顶点）
    virtual void BeginTextBatch() = 0;
    
    // 结束文本渲染批次并渲染所有累积的文本
    virtual void EndTextBatch(CommandBufferHandle commandBuffer, float screenWidth, float screenHeight,
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
    virtual void RenderText(CommandBufferHandle commandBuffer, const std::string& text, 
                           float x, float y, float screenWidth, float screenHeight,
                           float r = 1.0f, float g = 1.0f, 
                           float b = 1.0f, float a = 1.0f) = 0;
    
    // 渲染文字到命令缓冲区（centerX, centerY是文字中心坐标）
    virtual void RenderTextCentered(CommandBufferHandle commandBuffer, const std::string& text,
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


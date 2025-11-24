#include "ui/text/text.h"
#include "text/text_renderer.h"
#include "window/window.h"
#include <algorithm>
#include <cmath>

Text::Text() {
}

Text::~Text() {
    Cleanup();
}

bool Text::Initialize(VkDevice device, VkPhysicalDevice physicalDevice, 
                     VkCommandPool commandPool, VkQueue graphicsQueue, 
                     VkRenderPass renderPass, VkExtent2D swapchainExtent,
                     const TextConfig& config,
                     TextRenderer* textRenderer) {
    if (!textRenderer) {
        Window::ShowError("TextRenderer is required for Text UI component!");
        return false;
    }
    
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_renderPass = renderPass;
    m_swapchainExtent = swapchainExtent;
    
    // 从配置中设置文本属性
    m_text = config.text;
    m_colorR = config.colorR;
    m_colorG = config.colorG;
    m_colorB = config.colorB;
    m_colorA = config.colorA;
    m_useRelativePosition = config.useRelativePosition;
    m_relativeX = config.relativeX;
    m_relativeY = config.relativeY;
    m_useCenterPosition = config.useCenterPosition;
    m_screenWidth = (float)swapchainExtent.width;
    m_screenHeight = (float)swapchainExtent.height;
    m_textRenderer = textRenderer;
    
    // 根据配置设置位置
    if (m_useRelativePosition) {
        UpdateRelativePosition();
    } else {
        m_x = config.x;
        m_y = config.y;
    }
    
    m_initialized = true;
    return true;
}

void Text::Cleanup() {
    if (!m_initialized) return;
    
    m_textRenderer = nullptr;
    m_initialized = false;
}

void Text::SetRelativePosition(float relX, float relY, float screenWidth, float screenHeight) {
    m_relativeX = relX;
    m_relativeY = relY;
    m_useRelativePosition = true;
    if (screenWidth > 0.0f && screenHeight > 0.0f) {
        m_screenWidth = screenWidth;
        m_screenHeight = screenHeight;
        UpdateRelativePosition();
    }
}

void Text::UpdateScreenSize(float screenWidth, float screenHeight) {
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    if (m_useRelativePosition) {
        UpdateRelativePosition();
    }
}

void Text::UpdateRelativePosition() {
    if (m_useRelativePosition && m_screenWidth > 0.0f && m_screenHeight > 0.0f) {
        if (m_useCenterPosition) {
            // 相对位置 + 中心坐标：直接使用相对位置作为中心
            m_x = m_relativeX * m_screenWidth;
            m_y = m_relativeY * m_screenHeight;
        } else {
            // 相对位置 + 左上角坐标：需要计算左上角位置
            // 这里假设文本宽度和高度，实际应该根据文本内容计算
            // 为了简化，暂时使用相对位置作为左上角
            m_x = m_relativeX * m_screenWidth;
            m_y = m_relativeY * m_screenHeight;
        }
    }
}

void Text::Render(VkCommandBuffer commandBuffer, VkExtent2D extent) {
    if (!m_initialized || !m_textRenderer || m_text.empty()) return;
    
    float screenWidth = (float)extent.width;
    float screenHeight = (float)extent.height;
    
    if (m_useCenterPosition) {
        // 使用中心坐标渲染
        m_textRenderer->RenderTextCentered(commandBuffer, m_text,
                                          m_x, m_y,
                                          screenWidth, screenHeight,
                                          m_colorR, m_colorG, m_colorB, m_colorA);
    } else {
        // 使用左上角坐标渲染
        m_textRenderer->RenderText(commandBuffer, m_text,
                                  m_x, m_y,
                                  screenWidth, screenHeight,
                                  m_colorR, m_colorG, m_colorB, m_colorA);
    }
}


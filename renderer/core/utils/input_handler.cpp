#include "core/utils/input_handler.h"
#include "core/types/render_types.h"
#include "window/window.h"
#include <vulkan/vulkan.h>  // 仅在实现文件中包含，避免头文件依赖
#include <algorithm>
#include <cmath>

InputHandler::InputHandler() {
}

InputHandler::~InputHandler() {
}

void InputHandler::Initialize(IRenderer* renderer, Window* window, StretchMode stretchMode) {
    m_renderer = renderer;
    m_window = window;
    m_stretchMode = stretchMode;
}

void InputHandler::ConvertWindowToUICoords(int windowX, int windowY, float& uiX, float& uiY) {
    if (!m_renderer) {
        uiX = -1.0f;
        uiY = -1.0f;
        return;
    }
    
    if (!m_window) {
        uiX = -1.0f;
        uiY = -1.0f;
        return;
    }
    
    RECT clientRect;
    GetClientRect(m_window->GetHandle(), &clientRect);
    float currentScreenWidth = (float)(clientRect.right - clientRect.left);
    float currentScreenHeight = (float)(clientRect.bottom - clientRect.top);
    
    float clickX = (float)windowX;
    float clickY = (float)windowY;
    
    if (m_stretchMode == StretchMode::Scaled) {
        // Scaled模式：将屏幕坐标转换为逻辑坐标
        StretchParams stretchParams = m_renderer->GetStretchParams();
        if (stretchParams.stretchScaleX > 0.0f && stretchParams.stretchScaleY > 0.0f) {
            clickX = (clickX - stretchParams.marginX) / stretchParams.stretchScaleX;
            clickY = (clickY - stretchParams.marginY) / stretchParams.stretchScaleY;
        }
    } else if (m_stretchMode == StretchMode::Fit) {
        // 获取UI基准尺寸（背景纹理大小，如果没有背景则使用800x800）
        Extent2D uiBaseSize = m_renderer->GetUIBaseSize();
        float uiBaseWidth = (float)uiBaseSize.width;
        float uiBaseHeight = (float)uiBaseSize.height;
        
        // 计算视口大小和偏移（与vulkan_renderer.cpp中的逻辑一致）
        const float targetAspect = uiBaseWidth / uiBaseHeight;
        const float currentAspect = currentScreenWidth / currentScreenHeight;
        
        float viewportWidth, viewportHeight;
        float offsetX = 0.0f, offsetY = 0.0f;
        
        if (currentAspect > targetAspect) {
            // 窗口更宽 - 添加左右黑边（pillarbox）
            viewportHeight = currentScreenHeight;
            viewportWidth = viewportHeight * targetAspect;
            offsetX = (currentScreenWidth - viewportWidth) * 0.5f;
        } else {
            // 窗口更高或相等 - 添加上下黑边（letterbox）
            viewportWidth = currentScreenWidth;
            viewportHeight = viewportWidth / targetAspect;
            offsetY = (currentScreenHeight - viewportHeight) * 0.5f;
        }
        
        // 窗口坐标 -> 视口坐标（减去视口偏移）
        float viewportX = clickX - offsetX;
        float viewportY = clickY - offsetY;
        
        // 检查点击是否在视口内
        if (viewportX >= 0.0f && viewportX <= viewportWidth &&
            viewportY >= 0.0f && viewportY <= viewportHeight) {
            // 视口坐标 -> UI坐标系坐标（使用实际的UI基准尺寸）
            float scaleX = uiBaseWidth / viewportWidth;
            float scaleY = uiBaseHeight / viewportHeight;
            clickX = viewportX * scaleX;
            clickY = viewportY * scaleY;
        } else {
            // 点击在视口外（黑边区域），忽略
            clickX = -1.0f;
            clickY = -1.0f;
        }
    }
    
    uiX = clickX;
    uiY = clickY;
}

bool InputHandler::IsKeyPressed(int keyCode) const {
    if (!m_window) {
        return false;
    }
    return m_window->IsKeyPressed(keyCode);
}

void InputHandler::GetWASDKeys(bool& w, bool& a, bool& s, bool& d) const {
    if (!m_window) {
        w = a = s = d = false;
        return;
    }
    
    // 检查WASD键和方向键
    w = m_window->IsKeyPressed('W') || m_window->IsKeyPressed(VK_UP);
    a = m_window->IsKeyPressed('A') || m_window->IsKeyPressed(VK_LEFT);
    s = m_window->IsKeyPressed('S') || m_window->IsKeyPressed(VK_DOWN);
    d = m_window->IsKeyPressed('D') || m_window->IsKeyPressed(VK_RIGHT);
}

bool InputHandler::IsEscapePressed() const {
    if (!m_window) {
        return false;
    }
    return m_window->IsKeyPressed(VK_ESCAPE);
}


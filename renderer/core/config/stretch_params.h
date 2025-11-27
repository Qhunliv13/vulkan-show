#pragma once

/**
 * Canvas Items 模式的拉伸参数
 * 
 * 用于控制 UI 元素在不同窗口尺寸下的拉伸和定位行为
 */
struct StretchParams {
    float m_stretchScaleX = 1.0f;      // X轴拉伸比例
    float m_stretchScaleY = 1.0f;      // Y轴拉伸比例
    float m_logicalWidth = 800.0f;     // 逻辑宽度（viewportWidth）
    float m_logicalHeight = 800.0f;     // 逻辑高度（viewportHeight）
    float m_screenWidth = 800.0f;      // 屏幕宽度
    float m_screenHeight = 800.0f;     // 屏幕高度
    float m_marginX = 0.0f;            // X轴边距（offsetX）
    float m_marginY = 0.0f;            // Y轴边距（offsetY）
    
    /**
     * 验证参数有效性
     * @return true 如果所有参数都有效，false 如果存在无效值
     */
    bool IsValid() const {
        return m_stretchScaleX > 0.0f && m_stretchScaleY > 0.0f &&
               m_logicalWidth > 0.0f && m_logicalHeight > 0.0f &&
               m_screenWidth > 0.0f && m_screenHeight > 0.0f;
    }
    
    /**
     * 获取逻辑宽高比
     * @return 逻辑宽度 / 逻辑高度
     */
    float GetLogicalAspectRatio() const {
        return (m_logicalHeight > 0.0f) ? (m_logicalWidth / m_logicalHeight) : 1.0f;
    }
    
    /**
     * 获取屏幕宽高比
     * @return 屏幕宽度 / 屏幕高度
     */
    float GetScreenAspectRatio() const {
        return (m_screenHeight > 0.0f) ? (m_screenWidth / m_screenHeight) : 1.0f;
    }
    
    /**
     * 重置为默认值
     */
    void Reset() {
        m_stretchScaleX = 1.0f;
        m_stretchScaleY = 1.0f;
        m_logicalWidth = 800.0f;
        m_logicalHeight = 800.0f;
        m_screenWidth = 800.0f;
        m_screenHeight = 800.0f;
        m_marginX = 0.0f;
        m_marginY = 0.0f;
    }
};


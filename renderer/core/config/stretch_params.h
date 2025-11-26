#pragma once

/**
 * Canvas Items 模式的拉伸参数
 * 
 * 用于控制 UI 元素在不同窗口尺寸下的拉伸和定位行为
 */
struct StretchParams {
    float stretchScaleX = 1.0f;      // X轴拉伸比例
    float stretchScaleY = 1.0f;      // Y轴拉伸比例
    float logicalWidth = 800.0f;     // 逻辑宽度（viewportWidth）
    float logicalHeight = 800.0f;    // 逻辑高度（viewportHeight）
    float screenWidth = 800.0f;      // 屏幕宽度
    float screenHeight = 800.0f;     // 屏幕高度
    float marginX = 0.0f;            // X轴边距（offsetX）
    float marginY = 0.0f;            // Y轴边距（offsetY）
    
    /**
     * 验证参数有效性
     * @return true 如果所有参数都有效，false 如果存在无效值
     */
    bool IsValid() const {
        return stretchScaleX > 0.0f && stretchScaleY > 0.0f &&
               logicalWidth > 0.0f && logicalHeight > 0.0f &&
               screenWidth > 0.0f && screenHeight > 0.0f;
    }
    
    /**
     * 获取逻辑宽高比
     * @return 逻辑宽度 / 逻辑高度
     */
    float GetLogicalAspectRatio() const {
        return (logicalHeight > 0.0f) ? (logicalWidth / logicalHeight) : 1.0f;
    }
    
    /**
     * 获取屏幕宽高比
     * @return 屏幕宽度 / 屏幕高度
     */
    float GetScreenAspectRatio() const {
        return (screenHeight > 0.0f) ? (screenWidth / screenHeight) : 1.0f;
    }
    
    /**
     * 重置为默认值
     */
    void Reset() {
        stretchScaleX = 1.0f;
        stretchScaleY = 1.0f;
        logicalWidth = 800.0f;
        logicalHeight = 800.0f;
        screenWidth = 800.0f;
        screenHeight = 800.0f;
        marginX = 0.0f;
        marginY = 0.0f;
    }
};


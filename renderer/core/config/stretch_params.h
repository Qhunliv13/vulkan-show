#pragma once

// Canvas Items 模式的拉伸参数
struct StretchParams {
    float stretchScaleX = 1.0f;      // X轴拉伸比例
    float stretchScaleY = 1.0f;      // Y轴拉伸比例
    float logicalWidth = 800.0f;     // 逻辑宽度（viewportWidth）
    float logicalHeight = 800.0f;    // 逻辑高度（viewportHeight）
    float screenWidth = 800.0f;      // 屏幕宽度
    float screenHeight = 800.0f;     // 屏幕高度
    float marginX = 0.0f;            // X轴边距（offsetX）
    float marginY = 0.0f;            // Y轴边距（offsetY）
};


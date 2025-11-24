#version 450

// 全屏四边形的顶点（覆盖整个屏幕）
layout(location = 0) in vec2 inPosition;

layout(location = 0) out vec2 fragScreenCoord;

void main() {
    // inPosition 是归一化坐标 (0,0) 到 (1,1)
    // 直接转换为NDC坐标 (-1,-1) 到 (1,1)
    vec2 ndcPos = inPosition * 2.0 - 1.0;
    // 翻转Y轴以匹配窗口坐标系
    ndcPos.y = -ndcPos.y;
    
    gl_Position = vec4(ndcPos, 0.0, 1.0);
    
    // 传递屏幕坐标给片段着色器（用于判断是否在按钮区域内）
    // 这里先传递归一化坐标，片段着色器会根据屏幕尺寸计算实际坐标
    fragScreenCoord = inPosition;
}


#version 450

layout(location = 0) in vec2 inPosition;  // 归一化坐标 (0-1)
layout(location = 1) in vec4 inColor;     // 颜色（如果不使用纹理）

layout(push_constant) uniform PushConstants {
    vec2 position;      // 按钮位置（窗口坐标，左上角）
    vec2 size;         // 按钮大小
    vec2 screenSize;   // 屏幕尺寸
    float useTexture;  // 1.0 = 使用纹理, 0.0 = 使用颜色（fragment shader使用）
    float shapeType;   // 按钮形状类型（0.0=矩形，1.0=圆形）
    float hoverEffect; // 悬停效果（0.0=无效果, >0.0=变暗, <0.0=变淡）
} pc;

layout(location = 0) out vec2 fragTexCoord;  // 纹理坐标
layout(location = 1) out vec4 fragColor;     // 颜色（用于不使用纹理时）

void main() {
    // 将归一化坐标(0-1)转换为窗口坐标
    vec2 windowPos = inPosition * pc.size + pc.position;
    
    // 转换为NDC坐标 (-1到1，Y轴翻转)
    vec2 ndcPos;
    ndcPos.x = (windowPos.x / pc.screenSize.x) * 2.0 - 1.0;
    ndcPos.y = 1.0 - (windowPos.y / pc.screenSize.y) * 2.0;  // 翻转Y轴
    
    gl_Position = vec4(ndcPos, 0.0, 1.0);
    
    // 纹理坐标（直接使用归一化坐标）
    fragTexCoord = inPosition;
    
    // 传递颜色
    fragColor = inColor;
}


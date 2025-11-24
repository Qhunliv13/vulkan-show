#version 450

layout(location = 0) in vec2 fragScreenCoord;
layout(location = 0) out vec4 outColor;

// Push constants: 按钮位置、大小、屏幕尺寸、颜色、形状类型
layout(push_constant) uniform PushConstants {
    vec2 position;      // 按钮位置（窗口坐标，左上角）
    vec2 size;         // 按钮大小
    vec2 screenSize;   // 屏幕尺寸
    vec4 color;        // 按钮颜色 (r, g, b, a)
    float shapeType;   // 按钮形状类型（0.0=矩形，1.0=圆形）
} pc;

void main() {
    // 将归一化坐标转换为窗口坐标
    vec2 windowCoord = fragScreenCoord * pc.screenSize;
    
    bool inside = false;
    
    // 根据形状类型判断是否在按钮区域内
    if (pc.shapeType < 0.5) {
        // 矩形按钮
    vec2 buttonMin = pc.position;
    vec2 buttonMax = pc.position + pc.size;
    
    bool insideX = windowCoord.x >= buttonMin.x && windowCoord.x <= buttonMax.x;
    bool insideY = windowCoord.y >= buttonMin.y && windowCoord.y <= buttonMax.y;
        inside = insideX && insideY;
    } else {
        // 圆形按钮
        vec2 center = pc.position + pc.size * 0.5;  // 按钮中心
        float radius = min(pc.size.x, pc.size.y) * 0.5;  // 使用较小的尺寸作为半径，确保圆形在按钮区域内
        
        float dist = distance(windowCoord, center);
        inside = dist <= radius;
    }
    
    if (inside) {
        // 在按钮区域内，输出按钮颜色
        outColor = pc.color;
    } else {
        // 不在按钮区域内，输出透明（discard也可以，但透明更灵活）
        discard;
    }
}


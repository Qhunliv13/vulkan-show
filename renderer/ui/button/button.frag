#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

// 纹理采样器（可选，如果使用纹理）
layout(set = 0, binding = 0) uniform sampler2D texSampler;

// Push constants（必须与vertex shader中的定义一致）
layout(push_constant) uniform PushConstants {
    vec2 position;      // 按钮位置
    vec2 size;         // 按钮大小
    vec2 screenSize;   // 屏幕尺寸
    float useTexture;  // 1.0 = 使用纹理, 0.0 = 使用颜色
    float shapeType;   // 按钮形状类型（0.0=矩形，1.0=圆形）
    float hoverEffect; // 悬停效果（0.0=无效果, >0.0=变暗, <0.0=变淡）
} pc;

void main() {
    // 根据形状类型判断是否在按钮区域内
    bool inside = false;
    
    if (pc.shapeType < 0.5) {
        // 矩形按钮：所有片段都在矩形区域内（因为顶点着色器已经裁剪了）
        inside = true;
    } else {
        // 圆形按钮：计算当前片段到按钮中心的距离
        // 考虑宽高比，确保在屏幕空间中是正圆形
        vec2 center = vec2(0.5, 0.5);  // 按钮中心（归一化坐标）
        vec2 aspect = vec2(pc.size.x / pc.size.y, 1.0);  // 宽高比修正
        vec2 coord = (fragTexCoord - center) * aspect;  // 修正后的坐标
        
        float radius = 0.5;  // 半径（归一化坐标）
        float dist = length(coord);
        inside = dist <= radius;
    }
    
    if (!inside) {
        discard;  // 不在圆形区域内，丢弃片段
    }
    
    if (pc.useTexture > 0.5) {
        // 使用纹理
        // 翻转Y轴（因为纹理坐标和窗口坐标Y轴方向相反）
        vec2 texCoord = fragTexCoord;
        texCoord.y = 1.0 - texCoord.y;  // 翻转Y轴
        
        // 直接使用翻转后的纹理坐标，不进行缩放
        outColor = texture(texSampler, texCoord);
        
        // 应用悬停效果（纹理按钮）
        if (pc.hoverEffect > 0.0) {
            // 变暗效果：将RGB值乘以(1 - hoverEffect)
            float darkenFactor = 1.0 - pc.hoverEffect;
            outColor.rgb *= darkenFactor;
        } else if (pc.hoverEffect < 0.0) {
            // 变淡效果：将Alpha值乘以(1 - abs(hoverEffect))
            float fadeFactor = 1.0 + pc.hoverEffect;  // hoverEffect是负数，所以这里是1.0 - abs(hoverEffect)
            outColor.a *= fadeFactor;
        }
    } else {
        // 使用颜色（颜色按钮的悬停效果已经在CPU端通过UpdateButtonBuffer应用）
        outColor = fragColor;
    }
}


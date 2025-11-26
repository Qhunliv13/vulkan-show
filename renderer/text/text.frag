#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D fontTexture;

void main() {
    // 采样字体纹理（alpha 通道）
    float alpha = texture(fontTexture, fragTexCoord).a;
    
    // 跳过空白像素
    if (alpha < 0.01) {
        discard;
    }
    
    // 使用 alpha 和颜色输出
    // 标准alpha混合：result = srcColor * srcAlpha + dstColor * (1 - srcAlpha)
    outColor = vec4(fragColor.rgb, fragColor.a * alpha);
}


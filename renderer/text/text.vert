#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec4 inColor;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
    vec2 screenSize;  // 屏幕大小
} pc;

void main() {
    // 将窗口坐标转换为 NDC
    // inPosition 是窗口坐标（像素）
    // Vulkan NDC: Y向上（-1在底部，1在顶部）
    // 窗口坐标: Y向下（0在顶部，height在底部）
    // 所以需要翻转Y轴
    vec2 normalized = inPosition / pc.screenSize;
    vec2 ndcPos;
    ndcPos.x = normalized.x * 2.0 - 1.0;
    ndcPos.y = 1.0 - normalized.y * 2.0;  // 翻转 Y 轴
    
    gl_Position = vec4(ndcPos, 0.0, 1.0);
    // 不翻转纹理坐标，保持原样
    fragTexCoord = inTexCoord;
    fragColor = inColor;
}


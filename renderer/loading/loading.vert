#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec4 inColor;

layout(location = 0) out vec4 fragColor;

layout(push_constant) uniform PushConstants {
    vec2 position;  // 方块位置
    vec2 size;      // 方块大小
    vec2 screenSize; // 屏幕大小
} pc;

void main() {
    // Normal viewport: NDC Y direction is opposite to window Y direction
    // Window: (0,0) top-left, (width,height) bottom-right, Y increases downward
    // NDC: (-1,-1) bottom-left, (1,1) top-right, Y increases upward
    
    // inPosition is a unit square: (0,0) top-left, (1,1) bottom-right
    // position is the top-left corner in window coordinates (Y down)
    // size is the width and height in pixels
    vec2 windowPos = pc.position + inPosition * pc.size;
    
    // Convert window coordinates to NDC
    // For X: window 0->width maps to NDC -1->1
    //   ndcX = (windowX / width) * 2.0 - 1.0
    //
    // For Y: window 0->height maps to NDC 1->-1 (opposite direction, flipped)
    //   window y=0 (top) -> NDC y=1 (top after viewport flip)
    //   window y=height (bottom) -> NDC y=-1 (bottom after viewport flip)
    //   ndcY = 1.0 - 2.0 * (windowY / height)
    
    vec2 normalized = windowPos / pc.screenSize;
    vec2 ndcPos;
    ndcPos.x = normalized.x * 2.0 - 1.0;
    ndcPos.y = 1.0 - normalized.y * 2.0;  // Flip Y axis: window Y down -> NDC Y up
    
    gl_Position = vec4(ndcPos, 0.0, 1.0);
    fragColor = inColor;
}


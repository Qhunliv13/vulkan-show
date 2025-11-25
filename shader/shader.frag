#version 450

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    float time;
    float aspect;
} pc;

#define S smoothstep
#define PI 3.141596

vec2 hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return -1.0 + 2.0 * fract(sin(p) * 43758.5453123);
}

float noise(vec2 p) {
    const float K1 = 0.366025404;
    const float K2 = 0.211324865;
    vec2 i = floor(p + (p.x + p.y) * K1);
    vec2 a = p - i + (i.x + i.y) * K2;
    vec2 o = (a.x > a.y) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);
    vec2 b = a - o + K2;
    vec2 c = a - 1.0 + 2.0 * K2;
    vec3 h = max(0.5 - vec3(dot(a,a), dot(b,b), dot(c,c)), 0.0);
    vec3 n = h * h * h * h * vec3(
        dot(a, hash(i + 0.0)),
        dot(b, hash(i + o)),
        dot(c, hash(i + 1.0))
    );
    return dot(n, vec3(70.0));
}

mat2 rotate(float a) {
    float c = cos(a);
    float s = sin(a);
    return mat2(c, -s, s, c);
}

float fbm(vec2 p) {
    float a = 0.5;
    float n = 0.0;
    for(float i = 0.0; i < 4.0; i++) {
        n += a * noise(p);
        p *= 2.0;
        a *= 0.5;
    }
    return n;
}

float sdf_gouyu(vec2 uv, float r) {
    float d = max(uv.y > 0.0 ?
        length(uv) - r :
        min(length(uv+vec2(r*0.5,0)) - r*0.5,
            length(uv-vec2(r,0))),
        -length(uv-vec2(r*0.5,0))+r*0.5);
    return d;
}

void main() {
    // 应用宽高比，保持正确的像素比例
    vec2 uv = fragCoord * vec2(pc.aspect, 1.0);
    
    // 初始背景色 - 灰色
    vec3 bg = vec3(0.2, 0.2, 0.2);
    vec3 col = bg;
    
    float radius = 0.5;
    float noiseTime = pc.time * 0.3;
    
    // 计算噪声
    float n = fbm(uv * rotate(noiseTime) * 4.0);
    
    // 第一个勾玉（黑色主体）
    vec2 p = uv * rotate(-pc.time * 0.5);
    float d = sdf_gouyu(p + n * 0.1, radius);
    
    // 空心洞
    float d2 = length(p - vec2(-0.25, 0)) - 0.05 - 0.05 * n;
    d = max(d, -d2);
    
    float s = S(0.01, 0.0, d);
    col = mix(col, vec3(0), s);
    
    float glow = pow(0.01 / max(d, 0.0001), 2.0);
    col = mix(col, vec3(1), glow);
    
    // 第二个勾玉（白色主体）
    p = uv * rotate(PI - pc.time * 0.5);
    d = sdf_gouyu(p + n * 0.1, radius);
    d2 = length(p - vec2(-0.25, 0)) - 0.05 - 0.05 * n;
    d = max(d, -d2);
    
    s = S(0.01, 0.0, d);
    col = mix(col, vec3(1), s);
    
    glow = pow(0.01 / max(d, 0.0001), 2.0);
    col = mix(col, vec3(0), glow);
    
    outColor = vec4(col, 1.0);
}


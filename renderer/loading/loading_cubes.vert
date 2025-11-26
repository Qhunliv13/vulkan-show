#version 450

layout(location = 0) out vec2 fragCoord;

vec2 positions[6] = vec2[](
    vec2(-1.0, -1.0),
    vec2( 1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0, -1.0),
    vec2( 1.0,  1.0),
    vec2(-1.0,  1.0)
);

void main() {
    vec2 pos = positions[gl_VertexIndex];
    gl_Position = vec4(pos, 0.0, 1.0);
    // 传递标准化坐标，保持宽高比
    fragCoord = pos;
}


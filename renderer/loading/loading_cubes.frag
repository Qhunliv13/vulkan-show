#version 450

// by SamuelYAN
// Converted from p5.js to Vulkan shader
// Original: https://twitter.com/SamuelAnn0924
// https://www.instagram.com/samuel_yan_1990/

layout(location = 0) in vec2 fragCoord;
layout(location = 0) out vec4 outColor;

layout(push_constant) uniform PushConstants {
    float time;
    float aspect;
    float cameraYaw;   // 相机水平旋转角度（弧度）
    float cameraPitch; // 相机垂直旋转角度（弧度）
    float cameraPosX;  // 相机X位置
    float cameraPosY;  // 相机Y位置
    float cameraPosZ;  // 相机Z位置
} pc;

#define PI 3.14159265359
#define DEG2RAD (PI / 180.0)

// 从yaw和pitch构建正确的旋转矩阵
// 正确的顺序：先绕X轴旋转（pitch），再绕Y轴旋转（yaw）
// 参考Godot的实现方式：R = R_y(yaw) * R_x(pitch)
mat3 buildCameraRotationMatrix(float yaw, float pitch) {
    float cosYaw = cos(yaw);
    float sinYaw = sin(yaw);
    float cosPitch = cos(pitch);
    float sinPitch = sin(pitch);
    
    // 构建旋转矩阵：R = R_y(yaw) * R_x(pitch)
    mat3 rotX = mat3(
        1.0, 0.0, 0.0,
        0.0, cosPitch, -sinPitch,
        0.0, sinPitch, cosPitch
    );
    
    mat3 rotY = mat3(
        cosYaw, 0.0, sinYaw,
        0.0, 1.0, 0.0,
        -sinYaw, 0.0, cosYaw
    );
    
    // 组合旋转：先X后Y（矩阵乘法从右到左）
    return rotY * rotX;
}

// 从旋转矩阵提取前、右、上向量
void getCameraBasis(mat3 rotation, out vec3 forward, out vec3 right, out vec3 up) {
    // 相机默认看向-Z方向
    forward = normalize(rotation * vec3(0.0, 0.0, -1.0));
    right = normalize(rotation * vec3(1.0, 0.0, 0.0));
    up = normalize(rotation * vec3(0.0, 1.0, 0.0));
}

// 构建射线方向（用于ray marching）
vec3 buildRayDirection(vec2 uv, float fov, float aspectRatio, mat3 cameraRotation) {
    vec3 forward, right, up;
    getCameraBasis(cameraRotation, forward, right, up);
    
    // 构建射线方向
    float tanHalfFov = tan(fov * 0.5);
    vec3 rayDir = normalize(forward + 
                           uv.x * tanHalfFov * aspectRatio * right + 
                           uv.y * tanHalfFov * up);
    
    return rayDir;
}

// 伪随机函数
float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

float hash(vec2 p) {
    p = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(p.x + p.y) * 43758.5453123);
}

vec3 hash3(vec2 p) {
    float h = hash(p);
    return vec3(hash(h), hash(h * 1.1), hash(h * 1.2));
}

// 3D旋转矩阵
mat3 rotateX(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
        1.0, 0.0, 0.0,
        0.0, c, -s,
        0.0, s, c
    );
}

mat3 rotateY(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
        c, 0.0, s,
        0.0, 1.0, 0.0,
        -s, 0.0, c
    );
}

mat3 rotateZ(float angle) {
    float c = cos(angle);
    float s = sin(angle);
    return mat3(
        c, -s, 0.0,
        s, c, 0.0,
        0.0, 0.0, 1.0
    );
}

// SDF 立方体
float sdBox(vec3 p, vec3 b) {
    vec3 q = abs(p) - b;
    return length(max(q, 0.0)) + min(max(q.x, max(q.y, q.z)), 0.0);
}

// 射线与旋转立方体的交点距离
float intersectCube(vec3 ro, vec3 rd, vec3 cubePos, vec3 cubeSize, mat3 rot) {
    // 将射线转换到立方体的局部空间
    vec3 localRo = rot * (ro - cubePos);
    vec3 localRd = rot * rd;
    
    // 计算与AABB的交点
    vec3 invRd = 1.0 / (localRd + 0.0001); // 避免除零
    vec3 t0 = (-cubeSize - localRo) * invRd;
    vec3 t1 = (cubeSize - localRo) * invRd;
    vec3 tmin = min(t0, t1);
    vec3 tmax = max(t0, t1);
    
    float tnear = max(max(tmin.x, tmin.y), tmin.z);
    float tfar = min(min(tmax.x, tmax.y), tmax.z);
    
    if (tnear > tfar || tfar < 0.0) {
        return -1.0;
    }
    
    return tnear > 0.0 ? tnear : tfar;
}

// 计算立方体表面的法线（简化版本）
vec3 getCubeNormal(vec3 p, vec3 cubePos, vec3 cubeSize, mat3 rot) {
    vec3 localP = rot * (p - cubePos);
    vec3 q = abs(localP) - cubeSize;
    
    vec3 n = vec3(0.0);
    if (q.x > q.y && q.x > q.z) {
        n = vec3(sign(localP.x), 0.0, 0.0);
    } else if (q.y > q.z) {
        n = vec3(0.0, sign(localP.y), 0.0);
    } else {
        n = vec3(0.0, 0.0, sign(localP.z));
    }
    
    return normalize(rot * n);
}

// 渲染单个像素的函数
vec3 renderPixel(vec2 uv, vec3 cameraPos, mat3 cameraRotation) {
    // 固定的种子值
    float seed = 123456.0;
    
    // 立方体网格数量
    float cubeNum = 8.0;
    float cubeW = 1.0 / cubeNum;
    float cubeH = 1.0 / cubeNum;
    
    // 构建射线方向
    float fov = 45.0 * DEG2RAD;
    vec3 rayDir = buildRayDirection(uv, fov, pc.aspect, cameraRotation);
    
    // 初始化颜色（使用淡棕色背景）
    vec3 bgColor = vec3(210.0 / 255.0, 180.0 / 255.0, 140.0 / 255.0);
    vec3 col = bgColor;
    
    float minDist = 1000.0;
    vec3 hitPos = vec3(0.0);
    vec3 hitColor = vec3(0.0);
    vec3 hitNormal = vec3(0.0);
    
    // 遍历所有立方体
    for (float i = 0.0; i < cubeNum; i += 1.0) {
        for (float j = 0.0; j < cubeNum; j += 1.0) {
            // 计算立方体位置
            vec2 gridPos = vec2(i, j);
            vec2 normalizedGrid = (gridPos - cubeNum * 0.5 + 0.5) / cubeNum;
            vec3 cubePos = vec3(normalizedGrid.x * 1.3, normalizedGrid.y * 1.3, 0.0);  // 稍微增大范围
            
            // 立方体大小（增大以便更清晰）
            vec3 cubeSize = vec3(cubeW * 0.65, cubeH * 0.65, cubeW * 0.65);  // 进一步增大
            
            // 为每个立方体生成随机旋转
            vec2 cubeSeed = gridPos + seed * 100.0;
            float speed = 3.0;
            float cubeRotX = hash(cubeSeed.x * 20.0) * 360.0 * DEG2RAD + pc.time * speed * (hash(cubeSeed.x * 21.0) * 2.0 - 1.0) * 0.1;
            float cubeRotY = hash(cubeSeed.y * 20.0) * 360.0 * DEG2RAD + pc.time * speed * (hash(cubeSeed.y * 21.0) * 2.0 - 1.0) * 0.1;
            float cubeRotZ = hash((cubeSeed.x + cubeSeed.y) * 20.0) * 360.0 * DEG2RAD + pc.time * speed * (hash((cubeSeed.x + cubeSeed.y) * 21.0) * 2.0 - 1.0) * 0.1;
            
            mat3 cubeRot = rotateX(cubeRotX) * rotateY(cubeRotY) * rotateZ(cubeRotZ);
            
            // 检查射线与立方体的交点（使用更新后的相机位置）
            float t = intersectCube(cameraPos, rayDir, cubePos, cubeSize, cubeRot);
            
            if (t > 0.0 && t < minDist) {
                minDist = t;
                hitPos = cameraPos + rayDir * t;
                hitNormal = getCubeNormal(hitPos, cubePos, cubeSize, cubeRot);
                
                // 立方体颜色
                hitColor = hash3(cubeSeed + 1000.0);
                hitColor = pow(hitColor, vec3(1.0 / 1.8));
            }
        }
    }
    
    // 如果有命中，使用立方体颜色
    if (minDist < 1000.0) {
        // 改进的光照计算
        vec3 lightDir = normalize(vec3(1.0, 1.0, 1.0));
        float NdotL = max(dot(hitNormal, lightDir), 0.0);
        
        // 环境光
        float ambient = 0.4;
        
        // 漫反射光照
        float diffuse = NdotL * 0.6;
        
        // 总光照强度
        float light = ambient + diffuse;
        
        // 应用光照
        col = hitColor * light;
        
        // 添加边缘高光（更清晰）
        float edge = 1.0 - smoothstep(0.0, 0.03, minDist - 0.97);
        col = mix(col, hitColor * 1.6, edge * 0.35);
        
        // 添加一些对比度增强和锐化
        col = pow(col, vec3(0.85));  // 提高对比度
        col = clamp(col, 0.0, 1.0);  // 确保颜色在有效范围内
    }
    
    return col;
}

void main() {
    // 使用CPU端更新后的相机状态（相机状态在CPU端累积更新）
    vec3 cameraPos = vec3(pc.cameraPosX, pc.cameraPosY, pc.cameraPosZ);
    float cameraYaw = pc.cameraYaw;
    float cameraPitch = pc.cameraPitch;
    
    // 构建相机旋转矩阵（使用正确的顺序）
    mat3 cameraRotation = buildCameraRotationMatrix(cameraYaw, cameraPitch);
    
    // fragCoord范围是-1到1（正方形），直接使用，aspect ratio在射线构建时已考虑
    vec2 uv = fragCoord;
    
    // 抗锯齿：使用3x3超采样（9个采样点）
    // 计算一个像素在标准化坐标中的大小（基于实际窗口大小）
    // 假设窗口大小，用于计算抗锯齿偏移
    float pixelSize = 2.0 / 800.0;  // 一个像素在标准化坐标中的大小（参考值）
    float aaOffset = pixelSize * 0.33;  // 1/3像素偏移，用于3x3采样
    
    // 3x3采样点偏移（在标准化坐标空间中，不需要乘以aspect）
    vec2 offsets[9];
    float radius = aaOffset;
    offsets[0] = vec2(0.0, 0.0);  // 中心
    offsets[1] = vec2(-radius, -radius);
    offsets[2] = vec2( radius, -radius);
    offsets[3] = vec2(-radius,  radius);
    offsets[4] = vec2( radius,  radius);
    offsets[5] = vec2(0.0, -radius);
    offsets[6] = vec2(0.0,  radius);
    offsets[7] = vec2(-radius, 0.0);
    offsets[8] = vec2( radius, 0.0);
    
    // 对9个采样点进行采样并平均
    vec3 col = vec3(0.0);
    for (int i = 0; i < 9; i++) {
        col += renderPixel(uv + offsets[i], cameraPos, cameraRotation);
    }
    col /= 9.0;
    
    outColor = vec4(col, 1.0);
}

// GPU相机控制模块
// 所有相机计算都在GPU上完成

// 相机状态结构
struct CameraState {
    vec3 position;
    float yaw;      // 水平旋转（弧度）
    float pitch;    // 垂直旋转（弧度）
};

// 输入状态结构
struct InputState {
    float mouseDeltaX;   // 鼠标水平移动增量
    float mouseDeltaY;   // 鼠标垂直移动增量
    float mouseButtonDown; // 鼠标左键是否按下 (0.0 或 1.0)
    float keyW;          // W键是否按下 (0.0 或 1.0)
    float keyA;          // A键是否按下 (0.0 或 1.0)
    float keyS;          // S键是否按下 (0.0 或 1.0)
    float keyD;          // D键是否按下 (0.0 或 1.0)
    float deltaTime;     // 帧时间（秒）
};

// 相机参数
struct CameraParams {
    float moveSpeed;     // 移动速度
    float rotationSensitivity; // 旋转灵敏度
    float maxPitch;      // 最大pitch角度（弧度）
};

// 从yaw和pitch构建正确的旋转矩阵
// 正确的顺序：先绕Y轴旋转（yaw），再绕X轴旋转（pitch）
// 参考Godot的实现方式
mat3 buildCameraRotationMatrix(float yaw, float pitch) {
    float cosYaw = cos(yaw);
    float sinYaw = sin(yaw);
    float cosPitch = cos(pitch);
    float sinPitch = sin(pitch);
    
    // 构建旋转矩阵：R = R_y(yaw) * R_x(pitch)
    // 先绕X轴旋转pitch，再绕Y轴旋转yaw
    // 注意：矩阵乘法是从右到左的
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
    
    // 组合旋转：先X后Y
    return rotY * rotX;
}

// 从旋转矩阵提取前、右、上向量
void getCameraBasis(mat3 rotation, out vec3 forward, out vec3 right, out vec3 up) {
    // 相机默认看向-Z方向
    forward = normalize(rotation * vec3(0.0, 0.0, -1.0));
    right = normalize(rotation * vec3(1.0, 0.0, 0.0));
    up = normalize(rotation * vec3(0.0, 1.0, 0.0));
}

// 更新相机旋转（基于鼠标输入）
CameraState updateCameraRotation(CameraState camera, InputState input, CameraParams params) {
    CameraState newCamera = camera;
    
    if (input.mouseButtonDown > 0.5) {
        // 更新yaw和pitch
        newCamera.yaw += input.mouseDeltaX * params.rotationSensitivity;
        newCamera.pitch -= input.mouseDeltaY * params.rotationSensitivity; // 注意取反
        
        // 限制pitch角度
        newCamera.pitch = clamp(newCamera.pitch, -params.maxPitch, params.maxPitch);
    }
    
    return newCamera;
}

// 更新相机位置（基于WASD输入）
CameraState updateCameraPosition(CameraState camera, InputState input, CameraParams params) {
    CameraState newCamera = camera;
    
    // 构建旋转矩阵
    mat3 rotation = buildCameraRotationMatrix(camera.yaw, camera.pitch);
    
    // 获取相机基向量
    vec3 forward, right, up;
    getCameraBasis(rotation, forward, right, up);
    
    // 计算移动距离
    float moveDistance = params.moveSpeed * input.deltaTime;
    
    // 计算移动向量
    vec3 moveDir = vec3(0.0);
    
    if (input.keyW > 0.5) {
        moveDir += forward; // 向前
    }
    if (input.keyS > 0.5) {
        moveDir -= forward; // 向后
    }
    if (input.keyA > 0.5) {
        moveDir -= right; // 向左
    }
    if (input.keyD > 0.5) {
        moveDir += right; // 向右
    }
    
    // 归一化移动方向（如果同时按多个键）
    if (length(moveDir) > 0.001) {
        moveDir = normalize(moveDir);
        newCamera.position += moveDir * moveDistance;
    }
    
    return newCamera;
}

// 更新相机状态（组合旋转和位置更新）
CameraState updateCamera(CameraState camera, InputState input, CameraParams params) {
    CameraState newCamera = updateCameraRotation(camera, input, params);
    newCamera = updateCameraPosition(newCamera, input, params);
    return newCamera;
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


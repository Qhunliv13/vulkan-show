#pragma once

// 相机控制器接口 - 负责相机控制和输入处理
class ICameraController {
public:
    virtual ~ICameraController() = default;
    
    // 设置鼠标输入
    virtual void SetMouseInput(float deltaX, float deltaY, bool buttonDown) = 0;
    
    // 设置键盘输入
    virtual void SetKeyInput(bool w, bool a, bool s, bool d) = 0;
    
    // 更新相机（基于时间步）
    virtual void UpdateCamera(float deltaTime) = 0;
    
    // 重置相机状态
    virtual void ResetCamera() = 0;
};


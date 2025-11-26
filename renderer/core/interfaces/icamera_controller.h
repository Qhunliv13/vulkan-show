#pragma once

/**
 * 相机控制器接口 - 负责相机控制和输入处理
 * 
 * 职责：处理相机输入（鼠标、键盘）并更新相机状态
 * 设计：通过接口抽象，支持多种相机控制实现
 * 
 * 使用方式：
 * 1. 通过 IRenderer::GetCameraController() 获取接口指针
 * 2. 使用接口指针控制相机，无需了解具体实现
 */
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


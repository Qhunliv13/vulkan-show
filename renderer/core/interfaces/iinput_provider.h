#pragma once

// 输入提供者接口 - 用于解耦输入处理与渲染调度
class IInputProvider {
public:
    virtual ~IInputProvider() = default;
    
    // 检查按键是否按下
    virtual bool IsKeyPressed(int keyCode) const = 0;
    
    // 获取WASD按键状态（用于相机控制）
    virtual void GetWASDKeys(bool& w, bool& a, bool& s, bool& d) const = 0;
    
    // 检查ESC键是否按下
    virtual bool IsEscapePressed() const = 0;
};


#pragma once

/**
 * 输入提供者接口 - 用于解耦输入处理与渲染调度
 * 
 * 职责：提供键盘输入访问接口，支持渲染调度器获取输入状态
 * 设计：通过接口抽象，解耦渲染调度与输入处理实现
 * 
 * 使用方式：
 * 1. 通过依赖注入获取接口指针
 * 2. 使用 IsKeyPressed()、GetWASDKeys() 等方法获取输入状态
 */
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


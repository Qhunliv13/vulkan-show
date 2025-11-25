#pragma once

#include <windows.h>
#include "core/config/constants.h"
#include "core/config/stretch_params.h"
#include "core/interfaces/irenderer.h"
#include "core/interfaces/iinput_provider.h"
#include "core/interfaces/iinput_handler.h"
#include <vulkan/vulkan.h>

// 前向声明
class Window;

// 输入处理器 - 负责坐标转换和输入状态管理（实现 IInputProvider 和 IInputHandler 接口）
class InputHandler : public IInputProvider, public IInputHandler {
public:
    InputHandler();
    ~InputHandler();
    
    // 初始化（使用接口而不是具体类）
    void Initialize(IRenderer* renderer, Window* window, StretchMode stretchMode);
    
    // 将窗口坐标转换为UI坐标系坐标
    // 返回：转换后的坐标，如果点击在视口外则返回(-1, -1)
    void ConvertWindowToUICoords(int windowX, int windowY, float& uiX, float& uiY);
    
    // 设置拉伸模式（窗口大小变化时可能需要更新）
    void SetStretchMode(StretchMode mode) { m_stretchMode = mode; }
    
    // IInputProvider 接口实现
    bool IsKeyPressed(int keyCode) const override;
    void GetWASDKeys(bool& w, bool& a, bool& s, bool& d) const override;
    bool IsEscapePressed() const override;

private:
    IRenderer* m_renderer = nullptr;  // 使用接口而不是具体类
    Window* m_window = nullptr;
    StretchMode m_stretchMode = StretchMode::Fit;
};


#pragma once

#include <windows.h>

// 前向声明
class Window;

#include "core/config/constants.h"
#include "core/config/stretch_params.h"
#include "core/interfaces/iinput_handler.h"
#include "core/interfaces/iinput_provider.h"
#include "core/interfaces/irenderer.h"

/**
 * 输入处理器 - 负责坐标转换和输入状态管理
 * 
 * 职责：实现 IInputProvider 和 IInputHandler 接口，处理窗口坐标到UI坐标的转换
 * 设计：通过接口依赖注入，避免直接依赖具体实现类，支持多种渲染器和窗口实现
 */
class InputHandler : public IInputProvider, public IInputHandler {
public:
    InputHandler();
    ~InputHandler();
    
    /**
     * 初始化输入处理器
     * @param renderer 渲染器接口（不拥有所有权，由外部管理生命周期）
     * @param window 窗口对象（不拥有所有权，由外部管理生命周期）
     * @param stretchMode 拉伸模式，用于坐标转换
     */
    void Initialize(IRenderer* renderer, Window* window, StretchMode stretchMode);
    
    /**
     * 清理资源
     * 重置所有依赖指针，准备销毁
     */
    void Cleanup();
    
    /**
     * 将窗口坐标转换为UI坐标系坐标
     * @param windowX 窗口X坐标
     * @param windowY 窗口Y坐标
     * @param uiX 输出：转换后的UI X坐标（如果点击在视口外则为-1）
     * @param uiY 输出：转换后的UI Y坐标（如果点击在视口外则为-1）
     */
    void ConvertWindowToUICoords(int windowX, int windowY, float& uiX, float& uiY);
    
    /**
     * 设置拉伸模式（窗口大小变化时可能需要更新）
     * @param mode 新的拉伸模式
     */
    void SetStretchMode(StretchMode mode) { m_stretchMode = mode; }
    
    // IInputProvider 接口实现
    bool IsKeyPressed(int keyCode) const override;
    void GetWASDKeys(bool& w, bool& a, bool& s, bool& d) const override;
    bool IsEscapePressed() const override;

private:
    IRenderer* m_renderer = nullptr;  // 渲染器接口（不拥有所有权，由外部管理生命周期）
    Window* m_window = nullptr;  // 窗口对象（不拥有所有权，由外部管理生命周期）
    StretchMode m_stretchMode = StretchMode::Fit;  // 拉伸模式，用于坐标转换
};


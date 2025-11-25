#pragma once

#include <vector>
#include "core/config/constants.h"

// 前向声明
class Button;
class Slider;
class LoadingAnimation;
class TextRenderer;
class IRenderer;

// UI渲染提供者接口 - 用于解耦UI管理与渲染调度
class IUIRenderProvider {
public:
    virtual ~IUIRenderProvider() = default;
    
    // 获取UI组件用于渲染
    virtual LoadingAnimation* GetLoadingAnimation() const = 0;
    virtual Button* GetEnterButton() const = 0;
    virtual Button* GetColorButton() const = 0;
    virtual Button* GetLeftButton() const = 0;
    virtual Slider* GetOrangeSlider() const = 0;
    
    // 获取所有按钮和滑块（用于批量渲染）
    virtual void GetAllButtons(std::vector<Button*>& buttons) const = 0;
    virtual void GetAllSliders(std::vector<Slider*>& sliders) const = 0;
    
    // 处理窗口大小变化
    virtual void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) = 0;
};


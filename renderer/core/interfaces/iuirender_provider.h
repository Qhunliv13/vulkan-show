#pragma once

#include <vector>  // 2. 系统头文件
#include "core/config/constants.h"  // 4. 项目头文件（配置）

// 前向声明
class Button;
class Slider;
class LoadingAnimation;
class TextRenderer;
class IRenderer;

/**
 * UI渲染提供者接口 - 用于解耦UI管理与渲染调度
 * 
 * 职责：提供UI组件访问接口，支持渲染调度器获取UI组件
 * 设计：通过接口抽象，解耦渲染调度与UI管理实现
 * 
 * 使用方式：
 * 1. 通过依赖注入获取接口指针
 * 2. 使用 Get 方法获取UI组件用于渲染
 * 3. 使用 GetAll 方法批量获取组件
 */
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


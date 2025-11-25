#pragma once

#include <memory>
#include <vector>
#include "core/constants.h"
#include "core/render_context.h"
#include "core/iwindow_resize_handler.h"
#include "ui/slider/slider.h"
#include "ui/color_controller/color_controller.h"

// 前向声明
class IRenderer;
class Window;

// 滑块UI管理器 - 负责管理所有滑块组件
class SliderUIManager : public IWindowResizeHandler {
public:
    SliderUIManager();
    ~SliderUIManager();
    
    // 初始化滑块组件
    bool Initialize(const VulkanRenderContext& renderContext, Window* window, StretchMode stretchMode);
    
    // 清理资源
    void Cleanup();
    
    // IWindowResizeHandler 接口实现
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;
    
    // 获取滑块
    Slider* GetOrangeSlider() const { return m_orangeSlider.get(); }
    
    // 获取所有滑块（包括颜色控制器中的滑块）
    void GetAllSliders(std::vector<Slider*>& sliders, 
                      ColorController* colorController = nullptr,
                      const std::vector<std::unique_ptr<ColorController>>* boxColorControllers = nullptr) const;
    
private:
    bool InitializeOrangeSlider(VulkanRenderContext& renderContext, StretchMode stretchMode);
    
    std::unique_ptr<Slider> m_orangeSlider;
    bool m_sliderInitialized = false;
    Window* m_window = nullptr;
};


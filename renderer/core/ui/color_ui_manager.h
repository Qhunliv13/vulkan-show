#pragma once

#include <memory>
#include <vector>
#include "core/config/constants.h"
#include "core/interfaces/iwindow_resize_handler.h"
#include "ui/color_controller/color_controller.h"

// 前向声明
class ITextRenderer;
class IRenderer;
class LoadingAnimation;
class IWindow;
class IRenderContext;

// 颜色UI管理器 - 负责管理所有颜色相关的UI组件
class ColorUIManager : public IWindowResizeHandler {
public:
    ColorUIManager();
    ~ColorUIManager();
    
    // 初始化颜色UI组件
    bool Initialize(IRenderer* renderer,
                   const IRenderContext& renderContext,
                   ITextRenderer* textRenderer,
                   IWindow* window,
                   StretchMode stretchMode,
                   float screenWidth,
                   float screenHeight,
                   LoadingAnimation* loadingAnim);
    
    // 清理资源
    void Cleanup();
    
    // IWindowResizeHandler 接口实现
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;
    
    // 获取颜色控制器
    ColorController* GetColorController() const { return m_colorController.get(); }
    const std::vector<std::unique_ptr<ColorController>>& GetBoxColorControllers() const { return m_boxColorControllers; }
    
    // 获取/设置按钮颜色
    void GetButtonColor(float& r, float& g, float& b, float& a) const {
        r = m_buttonColorR; g = m_buttonColorG; b = m_buttonColorB; a = m_buttonColorA;
    }
    void SetButtonColor(float r, float g, float b, float a) {
        m_buttonColorR = r; m_buttonColorG = g; m_buttonColorB = b; m_buttonColorA = a;
    }
    
private:
    bool InitializeColorController(IRenderer* renderer, IRenderContext& renderContext,
                                   StretchMode stretchMode, float screenWidth, float screenHeight);
    bool InitializeBoxColorControllers(IRenderer* renderer, IRenderContext& renderContext,
                                      StretchMode stretchMode, float screenWidth, float screenHeight);
    
    void UpdateColorControllerPositions(float screenWidth, float screenHeight, StretchMode stretchMode, IRenderer* renderer);
    
    std::unique_ptr<ColorController> m_colorController;
    std::vector<std::unique_ptr<ColorController>> m_boxColorControllers;
    
    bool m_colorControllerInitialized = false;
    std::vector<bool> m_boxColorControllersInitialized;
    
    float m_buttonColorR = 1.0f;
    float m_buttonColorG = 1.0f;
    float m_buttonColorB = 1.0f;
    float m_buttonColorA = 1.0f;
    
    LoadingAnimation* m_loadingAnim = nullptr;
    IWindow* m_window = nullptr;
};


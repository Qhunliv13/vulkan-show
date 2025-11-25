#pragma once

#include <memory>
#include <vector>
#include "core/config/constants.h"
#include "core/config/render_context.h"
#include "core/interfaces/iwindow_resize_handler.h"
#include "ui/button/button.h"

// 前向声明
class ITextRenderer;
class IRenderer;
class Window;

// 按钮UI管理器 - 负责管理所有按钮组件
class ButtonUIManager : public IWindowResizeHandler {
public:
    ButtonUIManager();
    ~ButtonUIManager();
    
    // 初始化按钮组件
    bool Initialize(const VulkanRenderContext& renderContext, 
                   ITextRenderer* textRenderer,
                   Window* window,
                   StretchMode stretchMode,
                   float screenWidth, 
                   float screenHeight);
    
    // 清理资源
    void Cleanup();
    
    // IWindowResizeHandler 接口实现
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;
    
    // 获取按钮
    Button* GetEnterButton() const { return m_enterButton.get(); }
    Button* GetColorButton() const { return m_colorButton.get(); }
    Button* GetLeftButton() const { return m_leftButton.get(); }
    Button* GetColorAdjustButton() const { return m_colorAdjustButton.get(); }
    const std::vector<std::unique_ptr<Button>>& GetColorButtons() const { return m_colorButtons; }
    const std::vector<std::unique_ptr<Button>>& GetBoxColorButtons() const { return m_boxColorButtons; }
    
    // 获取所有按钮（用于渲染）
    void GetAllButtons(std::vector<Button*>& buttons) const;
    
    // 设置按钮颜色
    void SetButtonColor(float r, float g, float b, float a);
    
    // 获取按钮颜色
    void GetButtonColor(float& r, float& g, float& b, float& a) const {
        r = m_buttonColorR; g = m_buttonColorG; b = m_buttonColorB; a = m_buttonColorA;
    }
    
private:
    bool InitializeEnterButton(VulkanRenderContext& renderContext, StretchMode stretchMode);
    bool InitializeColorButton(VulkanRenderContext& renderContext, StretchMode stretchMode);
    bool InitializeLeftButton(VulkanRenderContext& renderContext, StretchMode stretchMode);
    bool InitializeColorButtons(VulkanRenderContext& renderContext, StretchMode stretchMode, 
                               float screenWidth, float screenHeight);
    bool InitializeBoxColorButtons(VulkanRenderContext& renderContext, StretchMode stretchMode,
                                  float screenWidth, float screenHeight);
    bool InitializeColorAdjustButton(VulkanRenderContext& renderContext, StretchMode stretchMode);
    
    void UpdateButtonPositions(float screenWidth, float screenHeight, StretchMode stretchMode, IRenderer* renderer);
    
    std::unique_ptr<Button> m_enterButton;
    std::unique_ptr<Button> m_colorButton;
    std::unique_ptr<Button> m_leftButton;
    std::unique_ptr<Button> m_colorAdjustButton;
    std::vector<std::unique_ptr<Button>> m_colorButtons;
    std::vector<std::unique_ptr<Button>> m_boxColorButtons;
    
    bool m_textRendererInitialized = false;
    std::vector<bool> m_colorButtonsInitialized;
    std::vector<bool> m_boxColorButtonsInitialized;
    bool m_colorAdjustButtonInitialized = false;
    
    float m_buttonColorR = 1.0f;
    float m_buttonColorG = 1.0f;
    float m_buttonColorB = 1.0f;
    float m_buttonColorA = 1.0f;
    
    ITextRenderer* m_textRenderer = nullptr;
    Window* m_window = nullptr;
};


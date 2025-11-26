#pragma once

#include "core/interfaces/iuirender_provider.h"  // 4. 项目头文件（接口）

// 前向声明
class UIManager;

/**
 * UI渲染提供者适配器 - 实现 IUIRenderProvider 接口，委托给 UIManager
 * 
 * 职责：将 UI 渲染提供者接口与 UI 管理器解耦，实现接口职责单一原则
 * 设计：通过适配器模式，将 IUIRenderProvider 接口的实现委托给 UIManager
 * 
 * 使用方式：
 * 1. 创建适配器实例，传入 UIManager 指针
 * 2. 使用适配器作为 IUIRenderProvider 接口传递给需要该接口的组件
 */
class UIRenderProviderAdapter : public IUIRenderProvider {
public:
    /**
     * 构造函数
     * 
     * @param uiManager UI管理器（不拥有所有权，由外部管理生命周期）
     */
    explicit UIRenderProviderAdapter(UIManager* uiManager);
    
    /**
     * IUIRenderProvider 接口实现
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由 UIManager 管理生命周期
     */
    LoadingAnimation* GetLoadingAnimation() const override;
    Button* GetEnterButton() const override;
    Button* GetColorButton() const override;
    Button* GetLeftButton() const override;
    Slider* GetOrangeSlider() const override;
    void GetAllButtons(std::vector<Button*>& buttons) const override;
    void GetAllSliders(std::vector<Slider*>& sliders) const override;
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;

private:
    UIManager* m_uiManager = nullptr;  // UI管理器（不拥有所有权，由外部管理生命周期）
};


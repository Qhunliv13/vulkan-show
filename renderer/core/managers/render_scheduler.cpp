#include "core/managers/render_scheduler.h"
#include "core/interfaces/irenderer.h"
#include "core/interfaces/iscene_provider.h"
#include "core/interfaces/iuirender_provider.h"
#include "core/interfaces/iinput_provider.h"
#include "text/text_renderer.h"
#include "core/interfaces/itext_renderer.h"
#include "window/window.h"
#include "ui/button/button.h"
#include "ui/slider/slider.h"
#include "loading/loading_animation.h"
#include <windows.h>
#include <stdio.h>

RenderScheduler::RenderScheduler() {
}

RenderScheduler::~RenderScheduler() {
}

void RenderScheduler::Initialize(IRenderer* renderer, 
                                 ISceneProvider* sceneProvider,
                                 IUIRenderProvider* uiRenderProvider,
                                 IInputProvider* inputProvider,
                                 ITextRenderer* textRenderer,
                                 Window* window,
                                 StretchMode stretchMode) {
    m_renderer = renderer;
    m_sceneProvider = sceneProvider;
    m_uiRenderProvider = uiRenderProvider;
    m_inputProvider = inputProvider;
    m_textRenderer = textRenderer;
    m_window = window;
    m_stretchMode = stretchMode;
}

void RenderScheduler::RenderFrame(float time, float deltaTime, float& fps) {
    if (!m_sceneProvider || !m_uiRenderProvider || !m_renderer) {
        return;
    }
    
    AppState currentState = m_sceneProvider->GetState();
    
    switch (currentState) {
        case AppState::LoadingCubes:
            RenderLoadingCubes(time, deltaTime, fps);
            break;
        case AppState::Loading:
            RenderLoading(time, fps);
            break;
        case AppState::Shader:
            RenderShader(time, fps);
            break;
    }
}

void RenderScheduler::RenderLoadingCubes(float time, float deltaTime, float& fps) {
    if (!m_inputProvider || !m_sceneProvider || !m_renderer) return;
    
    // 渲染3D场景
    if (m_sceneProvider->IsLoadingCubesPipelineCreated()) {
        // 通过输入提供者获取键盘输入（解耦Window依赖）
        bool wPressed, aPressed, sPressed, dPressed;
        m_inputProvider->GetWASDKeys(wPressed, aPressed, sPressed, dPressed);
        m_renderer->SetKeyInput(wPressed, aPressed, sPressed, dPressed);
        
        // 更新相机状态
        m_renderer->UpdateCamera(deltaTime);
        
        // 渲染loading_cubes shader
        m_renderer->DrawFrame(time, true, m_textRenderer, fps);
        
        // 按ESC键返回Loading状态（通过场景提供者接口）
        if (m_inputProvider->IsEscapePressed()) {
            printf("[DEBUG] ESC pressed, returning to Loading state\n");
            m_sceneProvider->SwitchToLoading();
        }
    } else {
        // 如果pipeline还没创建，先渲染黑色背景
        m_renderer->DrawFrame(time, false, m_textRenderer, fps);
    }
}

void RenderScheduler::RenderLoading(float time, float& fps) {
    if (!m_window || !m_uiRenderProvider || !m_renderer) return;
    
    // 更新UI组件位置（安全检查）
    RECT currentRect;
    GetClientRect(m_window->GetHandle(), &currentRect);
    float currentWidth = (float)(currentRect.right - currentRect.left);
    float currentHeight = (float)(currentRect.bottom - currentRect.top);
    
    if (m_stretchMode == StretchMode::Scaled) {
        m_uiRenderProvider->HandleWindowResize(m_stretchMode, m_renderer);
    }
    
    // 更新加载动画
    auto* loadingAnim = m_uiRenderProvider->GetLoadingAnimation();
    if (loadingAnim) {
        loadingAnim->Update(time);
    }
    
    // 获取所有按钮和滑块用于渲染（通过接口）
    std::vector<Button*> allButtons;
    m_uiRenderProvider->GetAllButtons(allButtons);
    
    std::vector<Slider*> allSliders;
    m_uiRenderProvider->GetAllSliders(allSliders);
    
    // 渲染加载界面
    DrawFrameWithLoadingParams params;
    params.time = time;
    params.loadingAnim = loadingAnim;
    params.enterButton = m_uiRenderProvider->GetEnterButton();
    params.textRenderer = m_textRenderer;
    params.colorButton = m_uiRenderProvider->GetColorButton();
    params.leftButton = m_uiRenderProvider->GetLeftButton();
    params.additionalButtons = &allButtons;
    params.slider = m_uiRenderProvider->GetOrangeSlider();
    params.additionalSliders = &allSliders;
    params.fps = fps;
    m_renderer->DrawFrameWithLoading(params);
}

void RenderScheduler::RenderShader(float time, float& fps) {
    // 渲染shader
    m_renderer->DrawFrame(time, false, m_textRenderer, fps);
}


#include "core/render_scheduler.h"
#include "core/scene_manager.h"
#include "core/ui_manager.h"
#include "core/irenderer.h"
#include "text/text_renderer.h"
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
                                 SceneManager* sceneManager,
                                 UIManager* uiManager,
                                 TextRenderer* textRenderer,
                                 Window* window,
                                 StretchMode stretchMode) {
    m_renderer = renderer;
    m_sceneManager = sceneManager;
    m_uiManager = uiManager;
    m_textRenderer = textRenderer;
    m_window = window;
    m_stretchMode = stretchMode;
}

void RenderScheduler::RenderFrame(float time, float deltaTime, float& fps) {
    if (!m_sceneManager || !m_uiManager || !m_renderer) {
        return;
    }
    
    AppState currentState = m_sceneManager->GetState();
    
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
    if (!m_window) return;
    
    // 渲染3D场景
    if (m_sceneManager->IsLoadingCubesPipelineCreated()) {
        // 设置键盘输入
        bool wPressed = m_window->IsKeyPressed('W') || m_window->IsKeyPressed(VK_UP);
        bool aPressed = m_window->IsKeyPressed('A') || m_window->IsKeyPressed(VK_LEFT);
        bool sPressed = m_window->IsKeyPressed('S') || m_window->IsKeyPressed(VK_DOWN);
        bool dPressed = m_window->IsKeyPressed('D') || m_window->IsKeyPressed(VK_RIGHT);
        m_renderer->SetKeyInput(wPressed, aPressed, sPressed, dPressed);
        
        // 更新相机状态
        m_renderer->UpdateCamera(deltaTime);
        
        // 渲染loading_cubes shader
        m_renderer->DrawFrame(time, true, m_textRenderer, fps);
        
        // 按ESC键返回Loading状态
        if (m_window->IsKeyPressed(VK_ESCAPE)) {
            printf("[DEBUG] ESC pressed, returning to Loading state\n");
            m_sceneManager->SwitchToLoading();
        }
    } else {
        // 如果pipeline还没创建，先渲染黑色背景
        m_renderer->DrawFrame(time, false, m_textRenderer, fps);
    }
}

void RenderScheduler::RenderLoading(float time, float& fps) {
    if (!m_window) return;
    
    // 更新UI组件位置（安全检查）
    RECT currentRect;
    GetClientRect(m_window->GetHandle(), &currentRect);
    float currentWidth = (float)(currentRect.right - currentRect.left);
    float currentHeight = (float)(currentRect.bottom - currentRect.top);
    
    if (m_stretchMode == StretchMode::Scaled) {
        m_uiManager->HandleWindowResize(m_stretchMode, m_renderer);
    }
    
    // 更新加载动画
    auto* loadingAnim = m_uiManager->GetLoadingAnimation();
    if (loadingAnim) {
        loadingAnim->Update(time);
    }
    
    // 获取所有按钮和滑块用于渲染
    std::vector<Button*> allButtons;
    m_uiManager->GetAllButtons(allButtons);
    
    std::vector<Slider*> allSliders;
    m_uiManager->GetAllSliders(allSliders);
    
    // 渲染加载界面
    DrawFrameWithLoadingParams params;
    params.time = time;
    params.loadingAnim = loadingAnim;
    params.enterButton = m_uiManager->GetEnterButton();
    params.textRenderer = m_textRenderer;
    params.colorButton = m_uiManager->GetColorButton();
    params.leftButton = m_uiManager->GetLeftButton();
    params.additionalButtons = &allButtons;
    params.slider = m_uiManager->GetOrangeSlider();
    params.additionalSliders = &allSliders;
    params.fps = fps;
    m_renderer->DrawFrameWithLoading(params);
}

void RenderScheduler::RenderShader(float time, float& fps) {
    // 渲染shader
    m_renderer->DrawFrame(time, false, m_textRenderer, fps);
}


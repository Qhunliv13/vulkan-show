#pragma once

#include "core/config/constants.h"
#include "core/interfaces/irenderer.h"

// 前向声明
class SceneManager;
class UIManager;
class TextRenderer;
class Window;

// 渲染调度器 - 负责根据场景状态调度渲染逻辑
class RenderScheduler {
public:
    RenderScheduler();
    ~RenderScheduler();
    
    // 初始化渲染调度器
    void Initialize(IRenderer* renderer, 
                   SceneManager* sceneManager,
                   UIManager* uiManager,
                   TextRenderer* textRenderer,
                   Window* window,
                   StretchMode stretchMode);
    
    // 渲染一帧（根据当前场景状态调度渲染）
    void RenderFrame(float time, float deltaTime, float& fps);
    
private:
    // 渲染LoadingCubes场景
    void RenderLoadingCubes(float time, float deltaTime, float& fps);
    
    // 渲染Loading场景
    void RenderLoading(float time, float& fps);
    
    // 渲染Shader场景
    void RenderShader(float time, float& fps);
    
    IRenderer* m_renderer = nullptr;
    SceneManager* m_sceneManager = nullptr;
    UIManager* m_uiManager = nullptr;
    TextRenderer* m_textRenderer = nullptr;
    Window* m_window = nullptr;
    StretchMode m_stretchMode = StretchMode::Fit;
};


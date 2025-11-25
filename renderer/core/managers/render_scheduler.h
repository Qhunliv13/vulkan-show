#pragma once

#include "core/config/constants.h"
#include "core/interfaces/irenderer.h"
#include "core/interfaces/iscene_provider.h"
#include "core/interfaces/iuirender_provider.h"
#include "core/interfaces/iinput_provider.h"

// 前向声明
class TextRenderer;
class Window;

// 渲染调度器 - 负责根据场景状态调度渲染逻辑（使用接口解耦）
class RenderScheduler {
public:
    RenderScheduler();
    ~RenderScheduler();
    
    // 初始化渲染调度器（使用接口而不是具体类）
    void Initialize(IRenderer* renderer, 
                   ISceneProvider* sceneProvider,
                   IUIRenderProvider* uiRenderProvider,
                   IInputProvider* inputProvider,
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
    ISceneProvider* m_sceneProvider = nullptr;
    IUIRenderProvider* m_uiRenderProvider = nullptr;
    IInputProvider* m_inputProvider = nullptr;
    TextRenderer* m_textRenderer = nullptr;
    Window* m_window = nullptr;  // 仅用于获取窗口句柄，不用于输入
    StretchMode m_stretchMode = StretchMode::Fit;
};


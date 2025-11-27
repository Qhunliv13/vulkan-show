#pragma once

#include "core/config/constants.h"  // 4. 项目头文件（配置）
#include "core/interfaces/iinput_provider.h"  // 4. 项目头文件（接口）
#include "core/interfaces/irenderer.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iscene_provider.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iuirender_provider.h"  // 4. 项目头文件（接口）

// 前向声明
class ITextRenderer;
class IWindow;

/**
 * 渲染调度器 - 负责根据场景状态调度渲染逻辑
 * 
 * 职责：根据当前场景状态调度不同的渲染逻辑（LoadingCubes、Loading、Shader）
 * 设计：使用接口解耦，通过依赖注入传入所有依赖
 * 
 * 使用方式：
 * 1. 通过依赖注入传入所有依赖（IRenderer、ISceneProvider、IUIRenderProvider等）
 * 2. 调用 Initialize() 初始化
 * 3. 调用 RenderFrame() 渲染一帧
 */
class RenderScheduler {
public:
    RenderScheduler();
    ~RenderScheduler();
    
    // 初始化渲染调度器（使用接口而不是具体类）
    void Initialize(IRenderer* renderer, 
                   ISceneProvider* sceneProvider,
                   IUIRenderProvider* uiRenderProvider,
                   IInputProvider* inputProvider,
                   ITextRenderer* textRenderer,
                   IWindow* window,
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
    ITextRenderer* m_textRenderer = nullptr;
    IWindow* m_window = nullptr;  // 仅用于获取窗口句柄，不用于输入
    StretchMode m_stretchMode = StretchMode::Fit;
};


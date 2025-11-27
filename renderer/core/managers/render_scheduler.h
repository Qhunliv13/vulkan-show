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
    
    /**
     * 初始化渲染调度器（使用接口而不是具体类）
     * 
     * 通过依赖注入传入所有必需的依赖，实现组件间解耦
     * 
     * @param renderer 渲染器（不拥有所有权，用于渲染）
     * @param sceneProvider 场景提供者（不拥有所有权，用于获取场景状态）
     * @param uiRenderProvider UI渲染提供者（不拥有所有权，用于UI渲染）
     * @param inputProvider 输入提供者（不拥有所有权，用于获取输入）
     * @param textRenderer 文字渲染器（不拥有所有权，用于文字渲染）
     * @param window 窗口（不拥有所有权，仅用于获取窗口句柄）
     * @param stretchMode 拉伸模式
     */
    void Initialize(IRenderer* renderer, 
                   ISceneProvider* sceneProvider,
                   IUIRenderProvider* uiRenderProvider,
                   IInputProvider* inputProvider,
                   ITextRenderer* textRenderer,
                   IWindow* window,
                   StretchMode stretchMode);
    
    /**
     * 渲染一帧（根据当前场景状态调度渲染）
     * 
     * 根据当前场景状态调用相应的渲染方法
     * 
     * @param time 总时间
     * @param deltaTime 帧时间
     * @param fps 帧率（输出参数，可能被修改）
     */
    void RenderFrame(float time, float deltaTime, float& fps);
    
private:
    /**
     * 渲染LoadingCubes场景
     * 
     * 渲染3D立方体场景，处理相机控制和键盘输入
     * 
     * @param time 总时间
     * @param deltaTime 帧时间
     * @param fps 帧率（输出参数）
     */
    void RenderLoadingCubes(float time, float deltaTime, float& fps);
    
    /**
     * 渲染Loading场景
     * 
     * 渲染加载界面，包括UI组件和加载动画
     * 
     * @param time 总时间
     * @param fps 帧率（输出参数）
     */
    void RenderLoading(float time, float& fps);
    
    /**
     * 渲染Shader场景
     * 
     * 渲染Shader场景
     * 
     * @param time 总时间
     * @param fps 帧率（输出参数）
     */
    void RenderShader(float time, float& fps);
    
    IRenderer* m_renderer = nullptr;  // 渲染器（不拥有所有权）
    ISceneProvider* m_sceneProvider = nullptr;  // 场景提供者（不拥有所有权）
    IUIRenderProvider* m_uiRenderProvider = nullptr;  // UI渲染提供者（不拥有所有权）
    IInputProvider* m_inputProvider = nullptr;  // 输入提供者（不拥有所有权）
    ITextRenderer* m_textRenderer = nullptr;  // 文字渲染器（不拥有所有权）
    IWindow* m_window = nullptr;  // 窗口（不拥有所有权，仅用于获取窗口句柄，不用于输入）
    StretchMode m_stretchMode = StretchMode::Fit;  // 拉伸模式
};


#pragma once

#include "core/constants.h"
#include "core/irenderer.h"
#include "core/iscene_provider.h"

// 前向声明
class LoadingAnimation;
class IConfigProvider;

// 应用状态
enum class AppState {
    LoadingCubes,  // 显示loading_cubes shader（启动界面）
    Loading,       // 显示加载动画
    Shader         // 显示shader
};

// 场景管理器 - 负责场景状态管理和切换（实现ISceneProvider接口）
class SceneManager : public ISceneProvider {
public:
    SceneManager();
    ~SceneManager();
    
    // ISceneProvider 接口实现
    AppState GetState() const override { return m_appState; }
    bool ShouldHandleInput() const override;
    
    // 设置状态
    void SetState(AppState state) { m_appState = state; }
    
    // 切换到Shader场景（使用接口）
    bool SwitchToShader(IRenderer* renderer, IConfigProvider* configProvider);
    
    // 切换到LoadingCubes场景（使用接口）
    bool SwitchToLoadingCubes(IRenderer* renderer, IConfigProvider* configProvider);
    
    // 切换到Loading场景
    void SwitchToLoading() { m_appState = AppState::Loading; }
    
    // 检查pipeline是否已创建
    bool IsShaderPipelineCreated() const { return m_shaderPipelineCreated; }
    bool IsLoadingCubesPipelineCreated() const { return m_loadingCubesPipelineCreated; }

private:
    AppState m_appState = AppState::Loading;
    bool m_shaderPipelineCreated = false;
    bool m_loadingCubesPipelineCreated = false;
};


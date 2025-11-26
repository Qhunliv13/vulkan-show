#pragma once

#include "core/config/constants.h"  // 4. 项目头文件（配置）
#include "core/interfaces/irenderer.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iscene_provider.h"  // 4. 项目头文件（接口）

// 前向声明
class LoadingAnimation;
class IConfigProvider;

/**
 * 场景管理器 - 负责场景状态管理和切换
 * 
 * 职责：实现 ISceneProvider 接口，管理应用场景状态和管线创建
 * 设计：通过接口访问渲染器和配置，遵循接口隔离原则
 * 
 * 使用方式：
 * 1. 创建 SceneManager 实例
 * 2. 通过 ISceneProvider 接口访问场景状态
 * 3. 使用 SwitchToShader()、SwitchToLoadingCubes() 切换场景
 */
class SceneManager : public ISceneProvider {
public:
    SceneManager();
    ~SceneManager();
    
    // ISceneProvider 接口实现
    AppState GetState() const override { return m_appState; }
    bool ShouldHandleInput() const override;
    void SwitchToLoading() override { m_appState = AppState::Loading; }
    bool IsLoadingCubesPipelineCreated() const override { return m_loadingCubesPipelineCreated; }
    
    // 设置状态
    void SetState(AppState state) { m_appState = state; }
    
    // 切换到Shader场景（使用接口）
    bool SwitchToShader(IRenderer* renderer, IConfigProvider* configProvider);
    
    // 切换到LoadingCubes场景（使用接口）
    bool SwitchToLoadingCubes(IRenderer* renderer, IConfigProvider* configProvider);
    
    // 检查pipeline是否已创建
    bool IsShaderPipelineCreated() const { return m_shaderPipelineCreated; }

private:
    AppState m_appState = AppState::Loading;
    bool m_shaderPipelineCreated = false;
    bool m_loadingCubesPipelineCreated = false;
};


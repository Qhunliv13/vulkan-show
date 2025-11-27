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
    /**
     * 获取当前应用状态
     * 
     * @return AppState 当前应用状态
     */
    AppState GetState() const override { return m_appState; }
    
    /**
     * 检查是否应该处理输入
     * 
     * 只在Loading状态下处理鼠标事件
     * 
     * @return bool 如果应该处理输入返回 true，否则返回 false
     */
    bool ShouldHandleInput() const override;
    
    /**
     * 切换到Loading状态
     */
    void SwitchToLoading() override { m_appState = AppState::Loading; }
    
    /**
     * 检查LoadingCubes管线是否已创建
     * 
     * @return bool 如果管线已创建返回 true，否则返回 false
     */
    bool IsLoadingCubesPipelineCreated() const override { return m_loadingCubesPipelineCreated; }
    
    /**
     * 设置应用状态
     * 
     * @param state 要设置的应用状态
     */
    void SetState(AppState state) { m_appState = state; }
    
    /**
     * 切换到Shader场景（使用接口）
     * 
     * 创建Shader管线（如果尚未创建）并切换到Shader状态
     * 
     * @param renderer 渲染器（不拥有所有权，用于创建管线）
     * @param configProvider 配置提供者（不拥有所有权，用于获取shader路径）
     * @return bool 如果切换成功返回 true，否则返回 false
     */
    bool SwitchToShader(IRenderer* renderer, IConfigProvider* configProvider);
    
    /**
     * 切换到LoadingCubes场景（使用接口）
     * 
     * 创建LoadingCubes管线（如果尚未创建）并切换到LoadingCubes状态
     * 
     * @param renderer 渲染器（不拥有所有权，用于创建管线）
     * @param configProvider 配置提供者（不拥有所有权，用于获取shader路径）
     * @return bool 如果切换成功返回 true，否则返回 false
     */
    bool SwitchToLoadingCubes(IRenderer* renderer, IConfigProvider* configProvider);
    
    /**
     * 检查Shader管线是否已创建
     * 
     * @return bool 如果管线已创建返回 true，否则返回 false
     */
    bool IsShaderPipelineCreated() const { return m_shaderPipelineCreated; }

private:
    AppState m_appState = AppState::Loading;  // 当前应用状态
    bool m_shaderPipelineCreated = false;  // Shader管线是否已创建
    bool m_loadingCubesPipelineCreated = false;  // LoadingCubes管线是否已创建
};


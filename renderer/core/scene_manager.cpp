#include "core/scene_manager.h"
#include "core/iconfig_provider.h"
#include "window/window.h"
#include <stdio.h>

SceneManager::SceneManager() {
}

SceneManager::~SceneManager() {
}

bool SceneManager::ShouldHandleInput() const {
    // 只在Loading状态下处理鼠标事件
    return m_appState == AppState::Loading;
}

bool SceneManager::SwitchToShader(IRenderer* renderer, IConfigProvider* configProvider) {
    if (!renderer || !configProvider) {
        return false;
    }
    
    m_appState = AppState::Shader;
    
    if (!m_shaderPipelineCreated) {
        std::string vertPath = configProvider->GetShaderVertexPath();
        std::string fragPath = configProvider->GetShaderFragmentPath();
        
        if (!renderer->CreateGraphicsPipeline(vertPath.c_str(), fragPath.c_str())) {
            Window::ShowError("Failed to create shader pipeline!");
            m_appState = AppState::Loading;
            return false;
        } else {
            m_shaderPipelineCreated = true;
        }
    }
    
    return true;
}

bool SceneManager::SwitchToLoadingCubes(IRenderer* renderer, IConfigProvider* configProvider) {
    if (!renderer || !configProvider) {
        return false;
    }
    
    m_appState = AppState::LoadingCubes;
    
    if (!m_loadingCubesPipelineCreated) {
        std::string vertPath = configProvider->GetLoadingCubesVertexPath();
        std::string fragPath = configProvider->GetLoadingCubesFragmentPath();
        
        // 先尝试 .spv 路径
        if (!renderer->CreateLoadingCubesPipeline(vertPath.c_str(), fragPath.c_str())) {
            // 如果失败，尝试不带 .spv 扩展名的路径（用于调试）
            std::string vertPathNoSpv = vertPath;
            std::string fragPathNoSpv = fragPath;
            if (vertPathNoSpv.size() > 4 && vertPathNoSpv.substr(vertPathNoSpv.size() - 4) == ".spv") {
                vertPathNoSpv = vertPathNoSpv.substr(0, vertPathNoSpv.size() - 4);
            }
            if (fragPathNoSpv.size() > 4 && fragPathNoSpv.substr(fragPathNoSpv.size() - 4) == ".spv") {
                fragPathNoSpv = fragPathNoSpv.substr(0, fragPathNoSpv.size() - 4);
            }
            
            if (!renderer->CreateLoadingCubesPipeline(vertPathNoSpv.c_str(), fragPathNoSpv.c_str())) {
                printf("[ERROR] Failed to create loading cubes pipeline!\n");
                Window::ShowError("Failed to create loading cubes pipeline!");
                m_appState = AppState::Loading;
                return false;
            } else {
                m_loadingCubesPipelineCreated = true;
            }
        } else {
            m_loadingCubesPipelineCreated = true;
        }
    }
    
    return true;
}


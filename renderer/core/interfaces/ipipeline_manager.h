#pragma once

#include <string>  // 2. 系统头文件

/**
 * 管线管理器接口 - 负责图形管线的创建和管理
 * 
 * 职责：管理渲染管线的创建、配置和生命周期
 * 设计：通过接口抽象，支持多种渲染后端的管线管理
 * 
 * 使用方式：
 * 1. 通过 IRenderer::GetPipelineManager() 获取接口指针
 * 2. 使用接口指针创建和管理管线，无需了解具体实现
 */
class IPipelineManager {
public:
    virtual ~IPipelineManager() = default;
    
    // 创建图形管线
    virtual bool CreateGraphicsPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath) = 0;
    
    // 创建加载立方体管线
    virtual bool CreateLoadingCubesPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath) = 0;
    
    // 光线追踪支持
    virtual bool IsRayTracingSupported() const = 0;
    virtual bool CreateRayTracingPipeline() = 0;
};


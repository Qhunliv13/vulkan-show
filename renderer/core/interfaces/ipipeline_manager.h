#pragma once

#include <string>

// 管线管理器接口 - 负责图形管线的创建和管理
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


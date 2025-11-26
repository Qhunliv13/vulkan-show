#pragma once

#include "core/config/constants.h"
#include <string>

// 配置提供者接口 - 用于依赖注入配置，替代单例
class IConfigProvider {
public:
    virtual ~IConfigProvider() = default;
    
    // 初始化配置（从命令行参数解析）
    virtual void Initialize(const char* lpCmdLine) = 0;
    
    // 获取配置值
    virtual AspectRatioMode GetAspectRatioMode() const = 0;
    virtual StretchMode GetStretchMode() const = 0;
    virtual BackgroundStretchMode GetBackgroundStretchMode() const = 0;
    
    // 设置配置值
    virtual void SetAspectRatioMode(AspectRatioMode mode) = 0;
    virtual void SetStretchMode(StretchMode mode) = 0;
    virtual void SetBackgroundStretchMode(BackgroundStretchMode mode) = 0;
    
    // 资源路径管理
    virtual std::string GetShaderVertexPath() const = 0;
    virtual std::string GetShaderFragmentPath() const = 0;
    virtual std::string GetLoadingCubesVertexPath() const = 0;
    virtual std::string GetLoadingCubesFragmentPath() const = 0;
    virtual std::string GetBackgroundTexturePath() const = 0;
    virtual std::string GetWindowIconPath() const = 0;
    
    // 窗口配置
    virtual int GetWindowWidth() const = 0;
    virtual int GetWindowHeight() const = 0;
    
    // 日志路径
    virtual std::string GetLogPath() const = 0;
};


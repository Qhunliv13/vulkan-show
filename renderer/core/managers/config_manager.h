#pragma once

#include "core/config/constants.h"
#include "core/interfaces/iconfig_provider.h"
#include <string>
#include <unordered_map>

// 配置管理器 - 统一管理应用配置（实现IConfigProvider接口，支持依赖注入）
class ConfigManager : public IConfigProvider {
public:
    // 保留单例方法以支持向后兼容，但推荐使用依赖注入
    static ConfigManager& GetInstance();
    
    // IConfigProvider 接口实现
    void Initialize(const char* lpCmdLine) override;
    AspectRatioMode GetAspectRatioMode() const override { return m_aspectMode; }
    StretchMode GetStretchMode() const override { return m_stretchMode; }
    BackgroundStretchMode GetBackgroundStretchMode() const override { return m_backgroundMode; }
    
    void SetAspectRatioMode(AspectRatioMode mode) override { m_aspectMode = mode; }
    void SetStretchMode(StretchMode mode) override { m_stretchMode = mode; }
    void SetBackgroundStretchMode(BackgroundStretchMode mode) override { m_backgroundMode = mode; }
    
    std::string GetShaderVertexPath() const override;
    std::string GetShaderFragmentPath() const override;
    std::string GetLoadingCubesVertexPath() const override;
    std::string GetLoadingCubesFragmentPath() const override;
    std::string GetBackgroundTexturePath() const override;
    std::string GetWindowIconPath() const override;
    
    int GetWindowWidth() const override { return m_windowWidth; }
    int GetWindowHeight() const override { return m_windowHeight; }
    
    std::string GetLogPath() const override { return m_logPath; }
    
    // 设置资源路径（扩展方法，不在接口中）
    void SetShaderVertexPath(const std::string& path) { m_shaderVertexPath = path; }
    void SetShaderFragmentPath(const std::string& path) { m_shaderFragmentPath = path; }
    void SetLoadingCubesVertexPath(const std::string& path) { m_loadingCubesVertexPath = path; }
    void SetLoadingCubesFragmentPath(const std::string& path) { m_loadingCubesFragmentPath = path; }
    void SetBackgroundTexturePath(const std::string& path) { m_backgroundTexturePath = path; }
    void SetWindowIconPath(const std::string& path) { m_windowIconPath = path; }
    
    void SetWindowWidth(int width) { m_windowWidth = width; }
    void SetWindowHeight(int height) { m_windowHeight = height; }
    void SetLogPath(const std::string& path) { m_logPath = path; }

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    // 解析命令行参数
    void ParseCommandLine(const char* lpCmdLine);
    
    // 配置值
    AspectRatioMode m_aspectMode = AspectRatioMode::Keep;
    StretchMode m_stretchMode = StretchMode::Fit;
    BackgroundStretchMode m_backgroundMode = BackgroundStretchMode::Fit;
    
    // 资源路径（默认值）
    std::string m_shaderVertexPath = "renderer/shader/shader.vert.spv";
    std::string m_shaderFragmentPath = "renderer/shader/shader.frag.spv";
    std::string m_loadingCubesVertexPath = "renderer/loading/loading_cubes.vert.spv";
    std::string m_loadingCubesFragmentPath = "renderer/loading/loading_cubes.frag.spv";
    std::string m_backgroundTexturePath = "assets/space_background.png";
    std::string m_windowIconPath = "assets/test.png";
    
    // 窗口配置
    int m_windowWidth = WINDOW_WIDTH;
    int m_windowHeight = WINDOW_HEIGHT;
    
    // 日志路径
    std::string m_logPath = "shader_app.log";
};


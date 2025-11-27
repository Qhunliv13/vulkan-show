#pragma once

#include <string>  // 2. 系统头文件
#include <unordered_map>  // 2. 系统头文件
#include "core/config/constants.h"  // 4. 项目头文件（配置）
#include "core/interfaces/iconfig_provider.h"  // 4. 项目头文件（接口）

/**
 * 配置管理器 - 统一管理应用配置
 * 
 * 职责：实现 IConfigProvider 接口，管理应用配置参数和资源路径
 * 设计：通过依赖注入使用，禁止使用单例模式
 * 
 * 使用方式：
 * 1. 创建 ConfigManager 实例
 * 2. 调用 Initialize() 从命令行参数初始化配置
 * 3. 通过 IConfigProvider 接口访问配置参数
 */
class ConfigManager : public IConfigProvider {
public:
    // 公共构造函数（支持依赖注入）
    ConfigManager() = default;
    ~ConfigManager() = default;
    
    // IConfigProvider 接口实现
    /**
     * 初始化配置（从命令行参数）
     * 
     * 解析命令行参数并设置配置值
     * 
     * @param lpCmdLine 命令行参数字符串
     */
    void Initialize(const char* lpCmdLine) override;
    
    /**
     * 获取拉伸模式
     * 
     * @return StretchMode 当前拉伸模式
     */
    StretchMode GetStretchMode() const override { return m_stretchMode; }
    
    /**
     * 获取背景拉伸模式
     * 
     * @return BackgroundStretchMode 当前背景拉伸模式
     */
    BackgroundStretchMode GetBackgroundStretchMode() const override { return m_backgroundMode; }
    
    /**
     * 设置拉伸模式
     * 
     * @param mode 要设置的拉伸模式
     */
    void SetStretchMode(StretchMode mode) override { m_stretchMode = mode; }
    
    /**
     * 设置背景拉伸模式
     * 
     * @param mode 要设置的背景拉伸模式
     */
    void SetBackgroundStretchMode(BackgroundStretchMode mode) override { m_backgroundMode = mode; }
    
    /**
     * 获取Shader顶点着色器路径
     * 
     * @return std::string Shader顶点着色器文件路径
     */
    std::string GetShaderVertexPath() const override;
    
    /**
     * 获取Shader片段着色器路径
     * 
     * @return std::string Shader片段着色器文件路径
     */
    std::string GetShaderFragmentPath() const override;
    
    /**
     * 获取LoadingCubes顶点着色器路径
     * 
     * @return std::string LoadingCubes顶点着色器文件路径
     */
    std::string GetLoadingCubesVertexPath() const override;
    
    /**
     * 获取LoadingCubes片段着色器路径
     * 
     * @return std::string LoadingCubes片段着色器文件路径
     */
    std::string GetLoadingCubesFragmentPath() const override;
    
    /**
     * 获取背景纹理路径
     * 
     * @return std::string 背景纹理文件路径
     */
    std::string GetBackgroundTexturePath() const override;
    
    /**
     * 获取窗口图标路径
     * 
     * @return std::string 窗口图标文件路径
     */
    std::string GetWindowIconPath() const override;
    
    /**
     * 获取窗口宽度
     * 
     * @return int 窗口宽度（像素）
     */
    int GetWindowWidth() const override { return m_windowWidth; }
    
    /**
     * 获取窗口高度
     * 
     * @return int 窗口高度（像素）
     */
    int GetWindowHeight() const override { return m_windowHeight; }
    
    /**
     * 获取日志文件路径
     * 
     * @return std::string 日志文件路径
     */
    std::string GetLogPath() const override { return m_logPath; }
    
    // 设置资源路径（扩展方法，不在接口中）
    /**
     * 设置Shader顶点着色器路径
     * 
     * @param path 文件路径
     */
    void SetShaderVertexPath(const std::string& path) { m_shaderVertexPath = path; }
    
    /**
     * 设置Shader片段着色器路径
     * 
     * @param path 文件路径
     */
    void SetShaderFragmentPath(const std::string& path) { m_shaderFragmentPath = path; }
    
    /**
     * 设置LoadingCubes顶点着色器路径
     * 
     * @param path 文件路径
     */
    void SetLoadingCubesVertexPath(const std::string& path) { m_loadingCubesVertexPath = path; }
    
    /**
     * 设置LoadingCubes片段着色器路径
     * 
     * @param path 文件路径
     */
    void SetLoadingCubesFragmentPath(const std::string& path) { m_loadingCubesFragmentPath = path; }
    
    /**
     * 设置背景纹理路径
     * 
     * @param path 文件路径
     */
    void SetBackgroundTexturePath(const std::string& path) { m_backgroundTexturePath = path; }
    
    /**
     * 设置窗口图标路径
     * 
     * @param path 文件路径
     */
    void SetWindowIconPath(const std::string& path) { m_windowIconPath = path; }
    
    /**
     * 设置窗口宽度
     * 
     * @param width 窗口宽度（像素）
     */
    void SetWindowWidth(int width) { m_windowWidth = width; }
    
    /**
     * 设置窗口高度
     * 
     * @param height 窗口高度（像素）
     */
    void SetWindowHeight(int height) { m_windowHeight = height; }
    
    /**
     * 设置日志文件路径
     * 
     * @param path 日志文件路径
     */
    void SetLogPath(const std::string& path) { m_logPath = path; }

private:
    // 禁止拷贝和赋值
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    /**
     * 解析命令行参数
     * 
     * 从命令行字符串中解析配置参数（如拉伸模式、背景模式等）
     * 
     * @param lpCmdLine 命令行参数字符串
     */
    void ParseCommandLine(const char* lpCmdLine);
    
    // 配置值
    StretchMode m_stretchMode = StretchMode::Fit;  // 拉伸模式
    BackgroundStretchMode m_backgroundMode = BackgroundStretchMode::Fit;  // 背景拉伸模式
    
    // 资源路径（默认值）
    std::string m_shaderVertexPath = "renderer/shader/shader.vert.spv";  // Shader顶点着色器路径
    std::string m_shaderFragmentPath = "renderer/shader/shader.frag.spv";  // Shader片段着色器路径
    std::string m_loadingCubesVertexPath = "renderer/loading/loading_cubes.vert.spv";  // LoadingCubes顶点着色器路径
    std::string m_loadingCubesFragmentPath = "renderer/loading/loading_cubes.frag.spv";  // LoadingCubes片段着色器路径
    std::string m_backgroundTexturePath = "assets/space_background.png";  // 背景纹理路径
    std::string m_windowIconPath = "assets/test.png";  // 窗口图标路径
    
    // 窗口配置
    int m_windowWidth = config::WINDOW_WIDTH;  // 窗口宽度（像素）
    int m_windowHeight = config::WINDOW_HEIGHT;  // 窗口高度（像素）
    
    // 日志路径
    std::string m_logPath = "shader_app.log";  // 日志文件路径
};


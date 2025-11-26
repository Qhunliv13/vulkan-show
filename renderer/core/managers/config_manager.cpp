#include "core/managers/config_manager.h"
#include <algorithm>
#include <cctype>
#include <cstring>

ConfigManager& ConfigManager::GetInstance() {
    static ConfigManager instance;
    return instance;
}

void ConfigManager::Initialize(const char* lpCmdLine) {
    ParseCommandLine(lpCmdLine);
}

void ConfigManager::ParseCommandLine(const char* lpCmdLine) {
    // 重置为默认值
    m_aspectMode = AspectRatioMode::Keep;
    m_stretchMode = StretchMode::Fit;
    m_backgroundMode = BackgroundStretchMode::Fit;
    
    if (!lpCmdLine || strlen(lpCmdLine) == 0) {
        return;
    }
    
    std::string cmdLine(lpCmdLine);
    std::string cmdLineLower = cmdLine;
    std::transform(cmdLineLower.begin(), cmdLineLower.end(), cmdLineLower.begin(), ::tolower);
    
    // 解析宽高比模式
    if (cmdLineLower.find("--aspect=keep") != std::string::npos || cmdLineLower.find("-a keep") != std::string::npos) {
        m_aspectMode = AspectRatioMode::Keep;
    } else if (cmdLineLower.find("--aspect=expand") != std::string::npos || cmdLineLower.find("-a expand") != std::string::npos) {
        m_aspectMode = AspectRatioMode::Expand;
    } else if (cmdLineLower.find("--aspect=keepwidth") != std::string::npos || cmdLineLower.find("-a keepwidth") != std::string::npos) {
        m_aspectMode = AspectRatioMode::KeepWidth;
    } else if (cmdLineLower.find("--aspect=keepheight") != std::string::npos || cmdLineLower.find("-a keepheight") != std::string::npos) {
        m_aspectMode = AspectRatioMode::KeepHeight;
    } else if (cmdLineLower.find("--aspect=center") != std::string::npos || cmdLineLower.find("-a center") != std::string::npos) {
        m_aspectMode = AspectRatioMode::Center;
    }
    
    // 解析拉伸模式
    if (cmdLineLower.find("--stretch=disabled") != std::string::npos || cmdLineLower.find("-s disabled") != std::string::npos) {
        m_stretchMode = StretchMode::Disabled;
    } else if (cmdLineLower.find("--stretch=scaled") != std::string::npos || cmdLineLower.find("-s scaled") != std::string::npos ||
               cmdLineLower.find("--stretch=canvas_items") != std::string::npos || cmdLineLower.find("-s canvas_items") != std::string::npos ||
               cmdLineLower.find("--stretch=2d") != std::string::npos || cmdLineLower.find("-s 2d") != std::string::npos) {
        m_stretchMode = StretchMode::Scaled;
    } else if (cmdLineLower.find("--stretch=fit") != std::string::npos || cmdLineLower.find("-s fit") != std::string::npos) {
        m_stretchMode = StretchMode::Fit;
    }
    
    // 解析背景模式
    if (cmdLineLower.find("--background=fit") != std::string::npos || cmdLineLower.find("-b fit") != std::string::npos) {
        m_backgroundMode = BackgroundStretchMode::Fit;
    } else if (cmdLineLower.find("--background=scaled") != std::string::npos || cmdLineLower.find("-b scaled") != std::string::npos) {
        m_backgroundMode = BackgroundStretchMode::Scaled;
    }
}

std::string ConfigManager::GetShaderVertexPath() const {
    return m_shaderVertexPath;
}

std::string ConfigManager::GetShaderFragmentPath() const {
    return m_shaderFragmentPath;
}

std::string ConfigManager::GetLoadingCubesVertexPath() const {
    return m_loadingCubesVertexPath;
}

std::string ConfigManager::GetLoadingCubesFragmentPath() const {
    return m_loadingCubesFragmentPath;
}

std::string ConfigManager::GetBackgroundTexturePath() const {
    return m_backgroundTexturePath;
}

std::string ConfigManager::GetWindowIconPath() const {
    return m_windowIconPath;
}


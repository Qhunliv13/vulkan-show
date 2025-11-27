#pragma once

#include <windows.h>  // 2. 系统头文件

// 前向声明
class IRendererFactory;
class IConfigProvider;
class ILogger;
class IEventBus;
class IWindowFactory;
class ITextRendererFactory;

// 应用初始化配置 - 封装所有初始化参数，简化接口
// 职责：集中管理初始化依赖，减少参数传递复杂度
struct AppInitializationConfig {
    IRendererFactory* rendererFactory = nullptr;
    HINSTANCE hInstance = nullptr;
    const char* lpCmdLine = nullptr;
    IConfigProvider* configProvider = nullptr;
    ILogger* logger = nullptr;
    IEventBus* eventBus = nullptr;
    IWindowFactory* windowFactory = nullptr;
    ITextRendererFactory* textRendererFactory = nullptr;
    
    // 验证配置是否有效
    bool IsValid() const {
        return rendererFactory != nullptr &&
               hInstance != nullptr &&
               configProvider != nullptr &&
               logger != nullptr &&
               eventBus != nullptr &&
               windowFactory != nullptr &&
               textRendererFactory != nullptr;
    }
};


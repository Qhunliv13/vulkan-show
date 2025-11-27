#pragma once

#include <windows.h>  // 2. 系统头文件

// 前向声明
class IRendererFactory;
class IConfigProvider;
class ILogger;
class IEventBus;
class IWindowFactory;
class ITextRendererFactory;

/**
 * 应用初始化配置 - 封装所有初始化参数，简化接口
 * 
 * 职责：集中管理初始化依赖，减少参数传递复杂度
 * 设计：将多个初始化参数封装为单个配置对象，简化 AppInitializer::Initialize() 接口
 * 
 * 使用方式：
 * 1. 创建 AppInitializationConfig 实例
 * 2. 设置所有必需的依赖指针
 * 3. 调用 IsValid() 验证配置有效性
 * 4. 传递给 AppInitializer::Initialize() 进行初始化
 */
struct AppInitializationConfig {
    IRendererFactory* rendererFactory = nullptr;
    HINSTANCE hInstance = nullptr;
    const char* lpCmdLine = nullptr;
    IConfigProvider* configProvider = nullptr;
    ILogger* logger = nullptr;
    IEventBus* eventBus = nullptr;
    IWindowFactory* windowFactory = nullptr;
    ITextRendererFactory* textRendererFactory = nullptr;  // 文字渲染器工厂（不拥有所有权）
    
    /**
     * 验证配置是否有效
     * 
     * 检查所有必需的依赖指针是否已设置
     * 
     * @return bool 如果所有必需的依赖都已设置返回 true，否则返回 false
     */
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


#pragma once

#include "initialization_result.h"
#include <windows.h>

// 前向声明
class IRendererFactory;
class WindowManager;
class IRenderer;
class IConfigProvider;
class ILogger;

// 渲染器初始化器 - 负责渲染器相关的初始化
class RendererInitializer {
public:
    RendererInitializer();
    ~RendererInitializer();
    
    // 初始化渲染器
    InitializationResult Initialize(IRendererFactory* rendererFactory, 
                                   WindowManager* windowManager,
                                   IConfigProvider* configProvider,
                                   HINSTANCE hInstance,
                                   IRenderer** outRenderer);
    
    // 清理
    void Cleanup();
    
    // 获取渲染器
    IRenderer* GetRenderer() const { return m_renderer; }

private:
    IRendererFactory* m_rendererFactory = nullptr;
    IRenderer* m_renderer = nullptr;
    ILogger* m_logger = nullptr;
};


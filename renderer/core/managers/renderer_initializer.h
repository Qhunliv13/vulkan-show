#pragma once

#include <windows.h>  // 2. 系统头文件
#include "initialization_result.h"  // 4. 项目头文件（初始化结果）

// 前向声明
class IRendererFactory;
class WindowManager;
class IRenderer;
class IConfigProvider;
class ILogger;

/**
 * 渲染器初始化器 - 负责渲染器相关的初始化
 * 
 * 职责：封装渲染器的初始化逻辑，简化 AppInitializer 的职责
 * 设计：通过依赖注入获取所有依赖，使用工厂模式创建渲染器
 * 
 * 使用方式：
 * 1. 创建 RendererInitializer 实例
 * 2. 调用 Initialize() 初始化渲染器（传入所有依赖）
 * 3. 使用 GetRenderer() 获取初始化后的渲染器
 * 4. 调用 Cleanup() 清理资源
 */
class RendererInitializer {
public:
    RendererInitializer();
    ~RendererInitializer();
    
    /**
     * 初始化渲染器
     * 
     * 使用工厂创建渲染器并初始化，设置配置参数
     * 
     * @param rendererFactory 渲染器工厂（不拥有所有权，用于创建渲染器）
     * @param windowManager 窗口管理器（不拥有所有权，用于获取窗口句柄）
     * @param configProvider 配置提供者（不拥有所有权，用于获取配置参数）
     * @param hInstance Windows 实例句柄
     * @param outRenderer 输出参数，返回创建的渲染器指针（不拥有所有权）
     * @return InitializationResult 初始化结果
     */
    InitializationResult Initialize(IRendererFactory* rendererFactory, 
                                   WindowManager* windowManager,
                                   IConfigProvider* configProvider,
                                   HINSTANCE hInstance,
                                   IRenderer** outRenderer);
    
    /**
     * 清理资源
     * 
     * 清理渲染器并释放相关资源
     */
    void Cleanup();
    
    /**
     * 获取渲染器
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由 RendererInitializer 管理生命周期
     * 
     * @return IRenderer* 渲染器指针，可能为 nullptr
     */
    IRenderer* GetRenderer() const { return m_renderer; }

private:
    IRendererFactory* m_rendererFactory = nullptr;  // 渲染器工厂（不拥有所有权）
    IRenderer* m_renderer = nullptr;  // 渲染器（不拥有所有权，由工厂管理）
    ILogger* m_logger = nullptr;  // 日志器（不拥有所有权）
};


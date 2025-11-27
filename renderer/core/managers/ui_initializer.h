#pragma once

#include "initialization_result.h"  // 4. 项目头文件（初始化结果）

// 前向声明
class IRenderer;
class ITextRenderer;
class Window;
class IConfigProvider;
class UIManager;
class IEventBus;
class ILogger;

/**
 * UI初始化器 - 负责UI相关的初始化
 * 
 * 职责：封装UI系统的初始化逻辑，简化 AppInitializer 的职责
 * 设计：通过依赖注入获取所有依赖，创建并初始化UI管理器
 * 
 * 使用方式：
 * 1. 创建 UIInitializer 实例
 * 2. 调用 Initialize() 初始化UI系统（传入所有依赖）
 * 3. 使用 GetUIManager() 获取初始化后的UI管理器
 * 4. 调用 Cleanup() 清理资源
 */
class UIInitializer {
public:
    UIInitializer();
    ~UIInitializer();
    
    /**
     * 初始化UI
     * 
     * 创建并初始化UI管理器，设置事件总线和回调
     * 
     * @param renderer 渲染器（不拥有所有权，用于UI渲染）
     * @param textRenderer 文字渲染器（不拥有所有权，用于文字渲染）
     * @param window 窗口（不拥有所有权，用于获取窗口信息）
     * @param configProvider 配置提供者（不拥有所有权，用于获取配置参数）
     * @param eventBus 事件总线（不拥有所有权，用于事件通信）
     * @param outUIManager 输出参数，返回创建的UI管理器指针（不拥有所有权）
     * @return InitializationResult 初始化结果
     */
    InitializationResult Initialize(IRenderer* renderer,
                                   ITextRenderer* textRenderer,
                                   Window* window,
                                   IConfigProvider* configProvider,
                                   IEventBus* eventBus,
                                   UIManager** outUIManager);
    
    /**
     * 清理资源
     * 
     * 清理UI管理器并释放相关资源
     */
    void Cleanup();
    
    /**
     * 获取UI管理器
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由 UIInitializer 管理生命周期
     * 
     * @return UIManager* UI管理器指针，可能为 nullptr
     */
    UIManager* GetUIManager() const { return m_uiManager; }

private:
    UIManager* m_uiManager = nullptr;  // UI管理器（不拥有所有权，由内部管理）
    ITextRenderer* m_textRenderer = nullptr;  // 文字渲染器（不拥有所有权）
    ILogger* m_logger = nullptr;  // 日志器（不拥有所有权）
};


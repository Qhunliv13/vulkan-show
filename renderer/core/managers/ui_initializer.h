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

// UI初始化器 - 负责UI相关的初始化
class UIInitializer {
public:
    UIInitializer();
    ~UIInitializer();
    
    // 初始化UI
    InitializationResult Initialize(IRenderer* renderer,
                                   ITextRenderer* textRenderer,
                                   Window* window,
                                   IConfigProvider* configProvider,
                                   IEventBus* eventBus,
                                   UIManager** outUIManager);
    
    // 清理
    void Cleanup();
    
    // 获取UI管理器
    UIManager* GetUIManager() const { return m_uiManager; }

private:
    UIManager* m_uiManager = nullptr;
    ITextRenderer* m_textRenderer = nullptr;
    ILogger* m_logger = nullptr;
};


#pragma once

#include <windows.h>
#include "core/config/constants.h"

// 前向声明
class EventManager;

// 窗口消息处理器 - 已废弃，功能已迁移到 EventManager
// 保留此类仅用于向后兼容，实际消息处理由 EventManager::ProcessMessages() 完成
class WindowMessageHandler {
public:
    WindowMessageHandler();
    ~WindowMessageHandler();
    
    // 初始化（已废弃，使用 EventManager::Initialize 和 ProcessMessages）
    void Initialize(EventManager* eventManager, class Window* window, StretchMode stretchMode, class IRenderer* renderer);
    
    // 处理消息队列中的消息（已废弃，使用 EventManager::ProcessMessages）
    bool ProcessMessages();
    
    // 设置拉伸模式（已废弃）
    void SetStretchMode(StretchMode stretchMode) { m_stretchMode = stretchMode; }

private:
    EventManager* m_eventManager = nullptr;
    StretchMode m_stretchMode = StretchMode::Fit;
};


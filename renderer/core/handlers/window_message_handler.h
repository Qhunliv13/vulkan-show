#pragma once

#include <windows.h>  // 2. 系统头文件
#include "core/config/constants.h"  // 4. 项目头文件（配置）

// 前向声明
class EventManager;
class Window;
class IRenderer;

/**
 * 窗口消息处理器 - 兼容层
 * 
 * 此类作为兼容层保留，实际功能已迁移到 EventManager
 * 保留原因：为现有代码提供向后兼容接口，避免大规模重构
 * 新代码应直接使用 EventManager::ProcessMessages() 进行消息处理
 * 
 * 设计意图：通过委托模式将消息处理转发给 EventManager，实现平滑迁移
 */
class WindowMessageHandler {
public:
    WindowMessageHandler();
    ~WindowMessageHandler();
    
    /**
     * 初始化消息处理器
     * 
     * 注意：此方法仅用于兼容旧代码，新代码应直接使用 EventManager
     * 实际功能通过委托给 EventManager 实现
     * 
     * @param eventManager 事件管理器（不拥有所有权，由外部管理生命周期）
     * @param window 窗口对象（不拥有所有权，仅用于兼容接口，当前未使用）
     * @param stretchMode 拉伸模式
     * @param renderer 渲染器（不拥有所有权，仅用于兼容接口，当前未使用）
     */
    void Initialize(EventManager* eventManager, Window* window, StretchMode stretchMode, IRenderer* renderer);
    
    /**
     * 处理消息队列中的消息
     * 
     * 注意：此方法仅用于兼容旧代码，新代码应直接使用 EventManager::ProcessMessages()
     * 实际功能通过委托给 EventManager::ProcessMessages() 实现
     * 
     * @return true 如果消息处理成功，false 如果事件管理器未初始化
     */
    bool ProcessMessages();
    
    /**
     * 设置拉伸模式
     * 
     * 注意：此方法仅用于兼容旧代码，新代码应直接使用 EventManager 或 IRenderer
     * 
     * @param stretchMode 拉伸模式
     */
    void SetStretchMode(StretchMode stretchMode) { m_stretchMode = stretchMode; }

private:
    EventManager* m_eventManager = nullptr;  // 事件管理器（不拥有所有权，由外部管理生命周期）
    StretchMode m_stretchMode = StretchMode::Fit;  // 当前拉伸模式，用于传递给 EventManager
};


#include "core/handlers/window_message_handler.h"  // 1. 对应头文件

#include "core/managers/event_manager.h"  // 4. 项目头文件

/**
 * 构造函数
 * 
 * 初始化成员变量为默认值，不执行任何实际初始化逻辑
 */
WindowMessageHandler::WindowMessageHandler() {
}

/**
 * 析构函数
 * 
 * 由于不拥有任何资源的所有权，无需清理操作
 */
WindowMessageHandler::~WindowMessageHandler() {
}

/**
 * 初始化消息处理器
 * 
 * 此方法仅用于兼容旧代码，实际功能已迁移到 EventManager
 * 通过保存事件管理器指针和拉伸模式，实现委托模式
 * 
 * @param eventManager 事件管理器（不拥有所有权，由外部管理生命周期）
 * @param window 窗口对象（不拥有所有权，仅用于兼容接口，当前未使用）
 * @param stretchMode 拉伸模式，用于传递给 EventManager
 * @param renderer 渲染器（不拥有所有权，仅用于兼容接口，当前未使用）
 */
void WindowMessageHandler::Initialize(EventManager* eventManager, Window* window, 
                                     StretchMode stretchMode, IRenderer* renderer) {
    m_eventManager = eventManager;
    m_stretchMode = stretchMode;
    // window 和 renderer 参数保留用于接口兼容性，实际功能由 EventManager 处理
    (void)window;   // 未使用参数，保留用于接口兼容性
    (void)renderer; // 未使用参数，保留用于接口兼容性
}

/**
 * 处理消息队列中的消息
 * 
 * 此方法仅用于兼容旧代码，实际功能通过委托给 EventManager::ProcessMessages() 实现
 * 新代码应直接使用 EventManager::ProcessMessages() 进行消息处理
 * 
 * @return true 如果消息处理成功，false 如果事件管理器未初始化
 */
bool WindowMessageHandler::ProcessMessages() {
    if (m_eventManager) {
        // 委托给 EventManager 处理消息，传递当前拉伸模式
        return m_eventManager->ProcessMessages(m_stretchMode);
    }
    return false;
}


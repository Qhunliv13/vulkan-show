#include "core/managers/window_manager.h"  // 1. 对应头文件

#include "core/interfaces/iconfig_provider.h"  // 4. 项目头文件（接口）
#include "core/interfaces/iwindow_factory.h"  // 4. 项目头文件（接口）
#include "core/interfaces/ievent_bus.h"  // 4. 项目头文件（接口）
#include "core/interfaces/ilogger.h"  // 4. 项目头文件（接口）
#include "window/window.h"  // 4. 项目头文件（窗口实现）

WindowManager::WindowManager() {
}

WindowManager::~WindowManager() {
    Cleanup();
}

bool WindowManager::Initialize(HINSTANCE hInstance, IConfigProvider* configProvider, 
                              IWindowFactory* windowFactory, IEventBus* eventBus, ILogger* logger) {
    if (m_initialized) {
        return true;
    }
    
    // 必须提供所有依赖（强制依赖注入）
    if (!configProvider) {
        if (logger) logger->Error("WindowManager::Initialize: configProvider cannot be nullptr");
        return false;
    }
    if (!windowFactory) {
        if (logger) logger->Error("WindowManager::Initialize: windowFactory cannot be nullptr");
        return false;
    }
    if (!eventBus) {
        if (logger) logger->Error("WindowManager::Initialize: eventBus cannot be nullptr");
        return false;
    }
    
    m_logger = logger;  // 保存日志器引用
    
    // 使用工厂创建窗口
    m_window = windowFactory->CreateWindowInstance();
    if (!m_window) {
        if (m_logger) m_logger->Error("Failed to create window from factory");
        return false;
    }
    
    // 设置事件总线
    m_window->SetEventBus(eventBus);
    
    if (!m_window->Create(hInstance, 
                         configProvider->GetWindowWidth(), 
                         configProvider->GetWindowHeight(), 
                         "A try of vulkan", 
                         nullptr, 
                         false, 
                         configProvider->GetWindowIconPath().c_str())) {
        if (m_logger) m_logger->Error("Failed to create window");
        return false;
    }
    
    m_window->SetIcon(configProvider->GetWindowIconPath());
    
    m_initialized = true;
    if (m_logger) m_logger->Info("WindowManager initialized successfully");
    return true;
}

bool WindowManager::IsRunning() const {
    return m_window && m_window->IsRunning();
}

bool WindowManager::IsMinimized() const {
    return m_window && m_window->IsMinimized();
}

bool WindowManager::HandleMinimized() {
    if (IsMinimized()) {
        Sleep(100);  // 窗口最小化时休眠，减少CPU占用
        return true;  // 返回true表示应该跳过渲染
    }
    return false;
}

void WindowManager::Cleanup() {
    if (!m_initialized) {
        return;
    }
    
    if (m_window) {
        m_window->Destroy();
        m_window.reset();
    }
    
    m_initialized = false;
    if (m_logger) m_logger->Info("WindowManager cleaned up");
}


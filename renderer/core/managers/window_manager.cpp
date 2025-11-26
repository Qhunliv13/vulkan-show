#include "core/managers/window_manager.h"
#include "core/interfaces/iconfig_provider.h"
#include "core/interfaces/iwindow_factory.h"
#include "core/interfaces/ievent_bus.h"
#include "window/window.h"
#include "core/utils/logger.h"

WindowManager::WindowManager() {
}

WindowManager::~WindowManager() {
    Cleanup();
}

bool WindowManager::Initialize(HINSTANCE hInstance, IConfigProvider* configProvider, 
                              IWindowFactory* windowFactory, IEventBus* eventBus) {
    if (m_initialized) {
        return true;
    }
    
    // 必须提供所有依赖（强制依赖注入）
    if (!configProvider) {
        LOG_ERROR("WindowManager::Initialize: configProvider cannot be nullptr");
        return false;
    }
    if (!windowFactory) {
        LOG_ERROR("WindowManager::Initialize: windowFactory cannot be nullptr");
        return false;
    }
    if (!eventBus) {
        LOG_ERROR("WindowManager::Initialize: eventBus cannot be nullptr");
        return false;
    }
    
    // 使用工厂创建窗口
    m_window = windowFactory->CreateWindowInstance();
    if (!m_window) {
        LOG_ERROR("Failed to create window from factory");
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
        LOG_ERROR("Failed to create window");
        return false;
    }
    
    m_window->SetIcon(configProvider->GetWindowIconPath());
    
    m_initialized = true;
    LOG_INFO("WindowManager initialized successfully");
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
    LOG_INFO("WindowManager cleaned up");
}


#include "core/managers/window_manager.h"
#include "core/managers/config_manager.h"
#include "core/interfaces/iconfig_provider.h"
#include "window/window.h"
#include "core/utils/logger.h"

WindowManager::WindowManager() {
}

WindowManager::~WindowManager() {
    Cleanup();
}

bool WindowManager::Initialize(HINSTANCE hInstance, IConfigProvider* configProvider) {
    if (m_initialized) {
        return true;
    }
    
    // 如果没有提供配置提供者，使用单例（向后兼容）
    IConfigProvider* config = configProvider;
    if (!config) {
        config = &ConfigManager::GetInstance();
    }
    
    m_window = std::make_unique<Window>();
    if (!m_window->Create(hInstance, 
                         config->GetWindowWidth(), 
                         config->GetWindowHeight(), 
                         "Vulkan Shader Gouyu", 
                         nullptr, 
                         false, 
                         config->GetWindowIconPath().c_str())) {
        LOG_ERROR("Failed to create window");
        return false;
    }
    
    m_window->SetIcon(config->GetWindowIconPath());
    
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


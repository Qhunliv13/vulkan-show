#pragma once

#include "core/interfaces/iwindow_factory.h"

// 默认窗口工厂实现
class WindowFactory : public IWindowFactory {
public:
    WindowFactory() = default;
    ~WindowFactory() = default;
    
    std::unique_ptr<Window> CreateWindowInstance() override;
    void DestroyWindow(Window* window) override;
};


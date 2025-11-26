#include "core/factories/window_factory.h"
#include "window/window.h"

std::unique_ptr<Window> WindowFactory::CreateWindowInstance() {
    return std::make_unique<Window>();
}

void WindowFactory::DestroyWindow(Window* window) {
    // Window 使用 unique_ptr 管理，自动清理
    // 如果需要特殊清理逻辑，可以在这里添加
    if (window) {
        window->Destroy();
    }
}


#include "core/factories/window_factory.h"
#include "window/window.h"

std::unique_ptr<Window> WindowFactory::CreateWindowInstance() {
    return std::make_unique<Window>();
}

void WindowFactory::DestroyWindow(Window* window) {
    // Window 使用 unique_ptr 管理，自动清理
    if (window) {
        window->Destroy();
    }
}


#include "core/factories/window_factory.h"  // 1. 对应头文件

#include "window/window.h"  // 4. 项目头文件

/**
 * 创建窗口实例
 * 
 * 使用 std::make_unique 创建窗口对象，确保异常安全
 * 返回的 unique_ptr 自动管理窗口生命周期
 */
std::unique_ptr<Window> WindowFactory::CreateWindowInstance() {
    return std::make_unique<Window>();
}

/**
 * 销毁窗口实例
 * 
 * 由于使用 std::unique_ptr 管理，此方法主要用于特殊清理逻辑
 * 正常情况下窗口会通过 unique_ptr 自动销毁，调用 Destroy() 方法
 * 
 * @param window 窗口指针（不拥有所有权，仅用于清理）
 */
void WindowFactory::DestroyWindow(Window* window) {
    if (window) {
        window->Destroy();
    }
}


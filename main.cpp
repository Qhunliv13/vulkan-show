#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>  // 系统头文件

#include "renderer/core/interfaces/irenderer_factory.h"  // 项目头文件（接口）
#include "renderer/core/managers/application.h"  // 项目头文件（管理器）
#include "renderer/vulkan/vulkan_renderer_factory.h"  // 项目头文件（实现）

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 创建渲染器工厂（实现依赖倒置）
    VulkanRendererFactory rendererFactory;
    
    // 使用Application类管理整个应用
    Application app;
    
    if (!app.Initialize(&rendererFactory, hInstance, lpCmdLine)) {
        return 1;
    }
    
    return app.Run();
}

#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include "renderer/core/application.h"
#include "renderer/core/irenderer_factory.h"
#include "renderer/vulkan/vulkan_renderer_factory.h"

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

// 注意：所有UI组件（如colorButtons、boxColorButtons、colorController等）
// 的初始化逻辑已迁移到Application类中
// 当前Application类已实现核心功能：基本按钮、滑块、主循环、消息处理等

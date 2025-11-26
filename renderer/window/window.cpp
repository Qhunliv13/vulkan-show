#include "window/window.h"
// 注意：window.h已经包含了windows.h，所以LoadImage宏可能已经被定义
// 在包含image_loader.h之前取消宏定义
#ifdef LoadImage
#undef LoadImage  // 取消Windows API的LoadImage宏定义，避免与ImageLoader::LoadImage冲突
#endif
#include "image/image_loader.h"
#include "core/interfaces/ievent_bus.h"
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// 辅助函数：编码32位整数（小端序）
static void encode_uint32(uint32_t value, uint8_t* buffer) {
    buffer[0] = (value) & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
    buffer[2] = (value >> 16) & 0xFF;
    buffer[3] = (value >> 24) & 0xFF;
}

// 辅助函数：编码16位整数（小端序）
static void encode_uint16(uint16_t value, uint8_t* buffer) {
    buffer[0] = (value) & 0xFF;
    buffer[1] = (value >> 8) & 0xFF;
}

// 辅助函数：从ImageData创建指定尺寸的图标
static HICON CreateIconFromImageData(const renderer::image::ImageData& imageData, uint32_t targetSize) {
    using namespace Gdiplus;
    
    // 创建源位图
    Bitmap* sourceBitmap = new Bitmap(imageData.width, imageData.height, PixelFormat32bppARGB);
    if (sourceBitmap->GetLastStatus() != Ok) {
        delete sourceBitmap;
        return nullptr;
    }
    
    // 复制像素数据到源位图
    BitmapData sourceData;
    Rect sourceRect(0, 0, imageData.width, imageData.height);
    if (sourceBitmap->LockBits(&sourceRect, ImageLockModeWrite, PixelFormat32bppARGB, &sourceData) == Ok) {
        uint8_t* dst = (uint8_t*)sourceData.Scan0;
        const uint8_t* src = imageData.pixels.data();
        
        for (uint32_t y = 0; y < imageData.height; y++) {
            for (uint32_t x = 0; x < imageData.width; x++) {
                uint8_t* dstPixel = dst + (y * sourceData.Stride + x * 4);
                const uint8_t* srcPixel = src + ((y * imageData.width + x) * 4);
                
                // RGBA -> BGRA
                dstPixel[0] = srcPixel[2];  // B
                dstPixel[1] = srcPixel[1];  // G
                dstPixel[2] = srcPixel[0];  // R
                dstPixel[3] = srcPixel[3];  // A
            }
        }
        
        sourceBitmap->UnlockBits(&sourceData);
    }
    
    // 创建目标尺寸的位图
    Bitmap* targetBitmap = new Bitmap(targetSize, targetSize, PixelFormat32bppARGB);
    if (targetBitmap->GetLastStatus() != Ok) {
        delete sourceBitmap;
        delete targetBitmap;
        return nullptr;
    }
    
    // 使用高质量缩放
    Graphics* g = Graphics::FromImage(targetBitmap);
    if (g) {
        g->SetInterpolationMode(InterpolationModeHighQualityBicubic);
        g->SetSmoothingMode(SmoothingModeAntiAlias);
        g->SetPixelOffsetMode(PixelOffsetModeHalf);
        g->DrawImage(sourceBitmap, 0, 0, targetSize, targetSize);
        delete g;
    }
    
    // 创建图标
    HICON hIcon = nullptr;
    targetBitmap->GetHICON(&hIcon);
    
    delete sourceBitmap;
    delete targetBitmap;
    
    return hIcon;
}

Window::Window() {
}

Window::~Window() {
    Destroy();
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // 从窗口用户数据中获取Window实例指针
    Window* window = nullptr;
    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* cs = (CREATESTRUCT*)lParam;
        window = (Window*)cs->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
    } else {
        window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    if (!window) {
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    
    switch (uMsg) {
        case WM_DESTROY:
            window->m_running = false;
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            // 检查窗口是否最小化
            if (wParam == SIZE_MINIMIZED) {
                // 窗口最小化时，不更新尺寸，保持之前的尺寸
                // 这样可以避免使用无效的尺寸进行渲染
                return 0;
            }
            // 只有在窗口正常大小或最大化时才更新尺寸
            window->m_width = LOWORD(lParam);
            window->m_height = HIWORD(lParam);
            // 确保尺寸有效（至少为1，避免除零错误）
            if (window->m_width < 1) window->m_width = 1;
            if (window->m_height < 1) window->m_height = 1;
            return 0;
        case WM_GETMINMAXINFO: {
            // Set minimum window size (like Godot)
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = 400;  // WINDOW_MIN_WIDTH
            mmi->ptMinTrackSize.y = 400;  // WINDOW_MIN_HEIGHT
            return 0;
        }
        case WM_LBUTTONDOWN:
            window->m_leftButtonDown = true;
            window->m_lastMouseX = LOWORD(lParam);
            window->m_lastMouseY = HIWORD(lParam);
            SetCapture(hwnd);  // 捕获鼠标，即使移出窗口也能接收消息
            return 0;
        case WM_LBUTTONUP:
            window->m_leftButtonDown = false;
            ReleaseCapture();  // 释放鼠标捕获
            return 0;
        case WM_MOUSEMOVE:
            if (window->m_leftButtonDown && window->m_eventBus) {
                int currentX = LOWORD(lParam);
                int currentY = HIWORD(lParam);
                float deltaX = (float)(currentX - window->m_lastMouseX);
                float deltaY = (float)(currentY - window->m_lastMouseY);
                MouseMovedEvent event(deltaX, deltaY, true);
                window->m_eventBus->Publish(event);
                window->m_lastMouseX = currentX;
                window->m_lastMouseY = currentY;
            }
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if (wParam < 256) {
                if (!window->m_keyStates[wParam]) {
                    window->m_keyStates[wParam] = true;
                    if (window->m_eventBus) {
                        KeyPressedEvent event((int)wParam, true);
                        window->m_eventBus->Publish(event);
                    }
                }
            }
            if (wParam == VK_ESCAPE) {
                window->m_running = false;
                PostQuitMessage(0);
            }
            return 0;
        case WM_KEYUP:
        case WM_SYSKEYUP:
            if (wParam < 256) {
                if (window->m_keyStates[wParam]) {
                    window->m_keyStates[wParam] = false;
                    if (window->m_eventBus) {
                        KeyPressedEvent event((int)wParam, false);
                        window->m_eventBus->Publish(event);
                    }
                }
            }
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool Window::Create(HINSTANCE hInstance, int width, int height, const char* title, const char* className, bool fullscreen, const char* iconPath) {
    m_hInstance = hInstance;
    m_width = width;
    m_height = height;
    m_running = true;
    m_fullscreen = fullscreen;
    
    if (className) {
        m_className = className;
    }
    
    // 如果提供了图标路径，先加载图标（创建32x32图标用于任务栏）
    HICON hIcon = nullptr;
    HICON hIconSm = nullptr;
    if (iconPath) {
        printf("[ICON] Loading icon from: %s\n", iconPath);
        auto imageData = renderer::image::ImageLoader::LoadImage(iconPath);
        if (imageData.width > 0 && imageData.height > 0) {
            printf("[ICON] Image loaded: %ux%u pixels\n", imageData.width, imageData.height);
            
            // 创建32x32大图标（用于任务栏和Alt+Tab）
            hIcon = CreateIconFromImageData(imageData, 32);
            if (hIcon) {
                printf("[ICON] 32x32 icon created successfully, handle: %p\n", hIcon);
            } else {
                printf("[ICON] Failed to create 32x32 icon!\n");
            }
            
            // 创建16x16小图标（用于窗口标题栏）
            hIconSm = CreateIconFromImageData(imageData, 16);
            if (hIconSm) {
                printf("[ICON] 16x16 icon created successfully, handle: %p\n", hIconSm);
            } else {
                printf("[ICON] Failed to create 16x16 icon!\n");
                // 如果16x16创建失败，使用32x32作为备用
                hIconSm = hIcon;
            }
        } else {
            printf("[ICON] Failed to load image data\n");
        }
    }
    
    // 注册窗口类
    // 优先使用资源中的图标（IDI_APP_ICON），这样任务栏和文件管理器会显示正确的图标
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = m_className;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    // 尝试从资源加载图标（资源ID为1，在app_icon.rc中定义）
    // 如果资源中没有图标，则使用默认图标
    HICON hResourceIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));  // 资源ID = 1
    if (hResourceIcon) {
        wc.hIcon = hResourceIcon;
        wc.hIconSm = hResourceIcon;  // 小图标也使用同一个
        printf("[ICON] Using icon from resource (IDI_APP_ICON)\n");
    } else {
        // 如果资源中没有图标，使用默认图标
        wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
        wc.hIconSm = nullptr;  // 小图标稍后通过WM_SETICON设置
        printf("[ICON] Resource icon not found, using default icon\n");
    }
    
    if (!RegisterClassEx(&wc)) {
        ShowError("Failed to register window class");
        return false;
    }
    
    if (fullscreen) {
        // 全屏模式
        DEVMODE dmScreenSettings = {};
        dmScreenSettings.dmSize = sizeof(dmScreenSettings);
        EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dmScreenSettings);
        
        m_width = dmScreenSettings.dmPelsWidth;
        m_height = dmScreenSettings.dmPelsHeight;
        
        m_hwnd = CreateWindowEx(
            WS_EX_APPWINDOW,
            m_className,
            title,
            WS_POPUP | WS_VISIBLE,
            0, 0,
            m_width, m_height,
            NULL, NULL, hInstance, this  // 传递this指针
        );
        
        if (m_hwnd == NULL) {
            ShowError("Failed to create fullscreen window");
            return false;
        }
        
        // 隐藏鼠标光标（可选，全屏时更美观）
        ShowCursor(FALSE);
        
        ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
    } else {
        // 窗口模式
        RECT rect = {0, 0, width, height};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        int windowWidth = rect.right - rect.left;
        int windowHeight = rect.bottom - rect.top;
        
        // 计算屏幕中心位置
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int windowX = (screenWidth - windowWidth) / 2;
        int windowY = (screenHeight - windowHeight) / 2;
        
        m_hwnd = CreateWindowEx(
            0,
            m_className,
            title,
            WS_OVERLAPPEDWINDOW,
            windowX, windowY,
            windowWidth, windowHeight,
            NULL, NULL, hInstance, this  // 传递this指针
        );
        
        if (m_hwnd == NULL) {
            ShowError("Failed to create window");
            return false;
        }
        
        // 显示正常大小的窗口
        ShowWindow(m_hwnd, SW_SHOWNORMAL);
    }
    
    UpdateWindow(m_hwnd);
    
    // 如果创建了图标，在窗口完全创建后设置（按照Godot的方式）
    if (hIcon && m_hwnd) {
        // 按照Godot的方式：先清除错误，然后设置图标
        // 注意：Godot 先设置 ICON_SMALL，再设置 ICON_BIG
        SetLastError(0);
        LRESULT result_small = SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)(hIconSm ? hIconSm : hIcon));
        DWORD error = GetLastError();
        if (error) {
            printf("[ICON] Error setting ICON_SMALL: %lu\n", error);
        }
        
        SetLastError(0);
        LRESULT result_big = SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        error = GetLastError();
        if (error) {
            printf("[ICON] Error setting ICON_BIG: %lu\n", error);
        }
        
        printf("[ICON] Set icon - Small: %p, Big: %p\n", (void*)result_small, (void*)result_big);
        
        // 额外尝试：设置窗口类图标（某些情况下需要）
        SetClassLongPtr(m_hwnd, GCLP_HICON, (LONG_PTR)hIcon);
        SetClassLongPtr(m_hwnd, GCLP_HICONSM, (LONG_PTR)(hIconSm ? hIconSm : hIcon));
        
        // 强制刷新窗口和任务栏
        InvalidateRect(m_hwnd, NULL, TRUE);
        UpdateWindow(m_hwnd);
        
        // 强制刷新任务栏（通过最小化和恢复窗口）
        // 注意：这可能会闪烁，但可以确保任务栏图标更新
        BOOL isVisible = IsWindowVisible(m_hwnd);
        if (isVisible) {
            ShowWindow(m_hwnd, SW_MINIMIZE);
            ShowWindow(m_hwnd, SW_RESTORE);
            printf("[ICON] Forced taskbar refresh\n");
        }
    } else {
        if (!hIcon) printf("[ICON] No icon to set (hIcon is null)\n");
        if (!m_hwnd) printf("[ICON] No window handle (m_hwnd is null)\n");
    }
    
    return true;
}

void Window::Destroy() {
    // 恢复鼠标光标（如果之前隐藏了）
    ShowCursor(TRUE);
    
    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
    
    if (m_hInstance) {
        UnregisterClass(m_className, m_hInstance);
    }
}

void Window::ToggleFullscreen() {
    // 全屏切换功能（如果需要的话）
    // 这里可以添加切换逻辑
}

void Window::ShowError(const std::string& message) {
    MessageBoxA(NULL, message.c_str(), "Error", MB_OK | MB_ICONERROR);
}

void Window::ProcessMessages() {
    MSG msg = {};
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) {
            m_running = false;
            break;
        }
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

bool Window::SetIcon(const std::string& iconPath) {
    if (!m_hwnd) {
        printf("[ICON] SetIcon: No window handle\n");
        return false;
    }
    
    printf("[ICON] SetIcon: Loading icon from: %s\n", iconPath.c_str());
    
    // 加载图标图像
    auto imageData = renderer::image::ImageLoader::LoadImage(iconPath);
    if (imageData.width == 0 || imageData.height == 0) {
        printf("[ICON] SetIcon: Failed to load image data\n");
        return false;
    }
    
    printf("[ICON] SetIcon: Image loaded: %ux%u pixels\n", imageData.width, imageData.height);
    
    // 创建32x32大图标（用于任务栏和Alt+Tab）
    HICON hicon = CreateIconFromImageData(imageData, 32);
    if (!hicon) {
        printf("[ICON] SetIcon: Failed to create 32x32 icon!\n");
        return false;
    }
    printf("[ICON] SetIcon: 32x32 icon created successfully, handle: %p\n", hicon);
    
    // 创建16x16小图标（用于窗口标题栏）
    HICON hiconSm = CreateIconFromImageData(imageData, 16);
    if (!hiconSm) {
        printf("[ICON] SetIcon: Failed to create 16x16 icon, using 32x32 as fallback\n");
        hiconSm = hicon;
    } else {
        printf("[ICON] SetIcon: 16x16 icon created successfully, handle: %p\n", hiconSm);
    }
    
    // 按照Godot的方式：先清除错误，然后设置图标
    // 注意：Godot 先设置 ICON_SMALL，再设置 ICON_BIG
    SetLastError(0);
    LRESULT result_small = SendMessage(m_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hiconSm);
    DWORD error = GetLastError();
    if (error) {
        printf("[ICON] SetIcon: Error setting ICON_SMALL: %lu\n", error);
    }
    
    SetLastError(0);
    LRESULT result_big = SendMessage(m_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hicon);
    error = GetLastError();
    if (error) {
        printf("[ICON] SetIcon: Error setting ICON_BIG: %lu\n", error);
    }
    
    printf("[ICON] SetIcon: Set icon - Small: %p, Big: %p\n", (void*)result_small, (void*)result_big);
    
    // 额外尝试：设置窗口类图标（某些情况下需要）
    SetClassLongPtr(m_hwnd, GCLP_HICON, (LONG_PTR)hicon);
    SetClassLongPtr(m_hwnd, GCLP_HICONSM, (LONG_PTR)hiconSm);
    
    // 强制刷新窗口和任务栏
    InvalidateRect(m_hwnd, NULL, TRUE);
    UpdateWindow(m_hwnd);
    
    // 强制刷新任务栏（通过最小化和恢复窗口）
    BOOL isVisible = IsWindowVisible(m_hwnd);
    if (isVisible) {
        ShowWindow(m_hwnd, SW_MINIMIZE);
        ShowWindow(m_hwnd, SW_RESTORE);
        printf("[ICON] SetIcon: Forced taskbar refresh\n");
    }
    
    return true;
}

bool Window::IsKeyPressed(int keyCode) const {
    if (keyCode >= 0 && keyCode < 256) {
        return m_keyStates[keyCode];
    }
    return false;
}

bool Window::IsMinimized() const {
    if (!m_hwnd) {
        return false;
    }
    return IsIconic(m_hwnd) != FALSE;
}


#include "window/window.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

#ifdef LoadImage
#undef LoadImage
#endif

#include "core/config/constants.h"
#include "core/interfaces/ievent_bus.h"
#include "image/image_loader.h"

namespace {
    /**
     * 从ImageData创建指定尺寸的Windows图标
     * 使用GDI+进行图像处理和高质量缩放，支持RGBA到BGRA的颜色格式转换
     * @param imageData 源图像数据
     * @param targetSize 目标图标尺寸
     * @return 成功返回图标句柄，失败返回 nullptr
     */
    HICON CreateIconFromImageData(const renderer::image::ImageData& imageData, uint32_t targetSize) {
        const Gdiplus::PixelFormat pixelFormat = static_cast<Gdiplus::PixelFormat>(0x0026200A);
        Gdiplus::Bitmap* sourceBitmap = new Gdiplus::Bitmap(
            static_cast<INT>(imageData.width), 
            static_cast<INT>(imageData.height), 
            pixelFormat);
        if (sourceBitmap->GetLastStatus() != Gdiplus::Ok) {
            delete sourceBitmap;
            return nullptr;
        }
        
        Gdiplus::BitmapData sourceData;
        Gdiplus::Rect sourceRect(0, 0, static_cast<INT>(imageData.width), static_cast<INT>(imageData.height));
        const Gdiplus::ImageLockMode lockMode = static_cast<Gdiplus::ImageLockMode>(1);
        const Gdiplus::PixelFormat lockFormat = static_cast<Gdiplus::PixelFormat>(0x0026200A);
        if (sourceBitmap->LockBits(&sourceRect, lockMode, lockFormat, &sourceData) == Gdiplus::Ok) {
            uint8_t* dst = (uint8_t*)sourceData.Scan0;
            const uint8_t* src = imageData.pixels.data();
            
            for (uint32_t y = 0; y < imageData.height; y++) {
                for (uint32_t x = 0; x < imageData.width; x++) {
                    uint8_t* dstPixel = dst + (y * sourceData.Stride + x * 4);
                    const uint8_t* srcPixel = src + ((y * imageData.width + x) * 4);
                    
                    dstPixel[0] = srcPixel[2];
                    dstPixel[1] = srcPixel[1];
                    dstPixel[2] = srcPixel[0];
                    dstPixel[3] = srcPixel[3];
                }
            }
            
            sourceBitmap->UnlockBits(&sourceData);
        }
        
        Gdiplus::Bitmap* targetBitmap = new Gdiplus::Bitmap(
            static_cast<INT>(targetSize), 
            static_cast<INT>(targetSize), 
            pixelFormat);
        if (targetBitmap->GetLastStatus() != Gdiplus::Ok) {
            delete sourceBitmap;
            delete targetBitmap;
            return nullptr;
        }
        
        Gdiplus::Graphics* g = Gdiplus::Graphics::FromImage(targetBitmap);
        if (g) {
            g->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
            g->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
            g->SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf);
            g->DrawImage(sourceBitmap, 0, 0, targetSize, targetSize);
            delete g;
        }
        
        HICON hIcon = nullptr;
        targetBitmap->GetHICON(&hIcon);
        
        delete sourceBitmap;
        delete targetBitmap;
        
        return hIcon;
    }
}

Window::Window() {
}

Window::~Window() {
    Destroy();
}

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
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
            if (wParam == SIZE_MINIMIZED) {
                return 0;
            }
            window->m_width = LOWORD(lParam);
            window->m_height = HIWORD(lParam);
            if (window->m_width < 1) window->m_width = 1;
            if (window->m_height < 1) window->m_height = 1;
            return 0;
        case WM_GETMINMAXINFO: {
            MINMAXINFO* mmi = (MINMAXINFO*)lParam;
            mmi->ptMinTrackSize.x = config::WINDOW_MIN_WIDTH;
            mmi->ptMinTrackSize.y = config::WINDOW_MIN_HEIGHT;
            return 0;
        }
        case WM_LBUTTONDOWN:
            window->m_leftButtonDown = true;
            window->m_lastMouseX = LOWORD(lParam);
            window->m_lastMouseY = HIWORD(lParam);
            SetCapture(hwnd);
            return 0;
        case WM_LBUTTONUP:
            window->m_leftButtonDown = false;
            ReleaseCapture();
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
    
    HICON hIcon = nullptr;
    HICON hIconSm = nullptr;
    if (iconPath) {
        printf("[ICON] Loading icon from: %s\n", iconPath);
        auto imageData = renderer::image::ImageLoader::LoadImage(iconPath);
        if (imageData.width > 0 && imageData.height > 0) {
            printf("[ICON] Image loaded: %ux%u pixels\n", imageData.width, imageData.height);
            
            hIcon = CreateIconFromImageData(imageData, 32);
            if (hIcon) {
                printf("[ICON] 32x32 icon created successfully, handle: %p\n", hIcon);
            } else {
                printf("[ICON] Failed to create 32x32 icon!\n");
            }
            
            hIconSm = CreateIconFromImageData(imageData, 16);
            if (hIconSm) {
                printf("[ICON] 16x16 icon created successfully, handle: %p\n", hIconSm);
            } else {
                printf("[ICON] Failed to create 16x16 icon!\n");
                hIconSm = hIcon;
            }
        } else {
            printf("[ICON] Failed to load image data\n");
        }
    }
    
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = m_className;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    
    HICON hResourceIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    if (hResourceIcon) {
        wc.hIcon = hResourceIcon;
        wc.hIconSm = hResourceIcon;
        printf("[ICON] Using icon from resource (IDI_APP_ICON)\n");
    } else {
        wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
        wc.hIconSm = nullptr;
        printf("[ICON] Resource icon not found, using default icon\n");
    }
    
    if (!RegisterClassEx(&wc)) {
        ShowError("Failed to register window class");
        return false;
    }
    
    if (fullscreen) {
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
            NULL, NULL, hInstance, this
        );
        
        if (m_hwnd == NULL) {
            ShowError("Failed to create fullscreen window");
            return false;
        }
        
        ShowCursor(FALSE);
        ShowWindow(m_hwnd, SW_SHOWMAXIMIZED);
    } else {
        RECT rect = {0, 0, width, height};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
        int windowWidth = rect.right - rect.left;
        int windowHeight = rect.bottom - rect.top;
        
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
            NULL, NULL, hInstance, this
        );
        
        if (m_hwnd == NULL) {
            ShowError("Failed to create window");
            return false;
        }
        
        ShowWindow(m_hwnd, SW_SHOWNORMAL);
    }
    
    UpdateWindow(m_hwnd);
    
    if (hIcon && m_hwnd) {
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
        
        SetClassLongPtr(m_hwnd, GCLP_HICON, (LONG_PTR)hIcon);
        SetClassLongPtr(m_hwnd, GCLP_HICONSM, (LONG_PTR)(hIconSm ? hIconSm : hIcon));
        
        InvalidateRect(m_hwnd, NULL, TRUE);
        UpdateWindow(m_hwnd);
        
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
    
    auto imageData = renderer::image::ImageLoader::LoadImage(iconPath);
    if (imageData.width == 0 || imageData.height == 0) {
        printf("[ICON] SetIcon: Failed to load image data\n");
        return false;
    }
    
    printf("[ICON] SetIcon: Image loaded: %ux%u pixels\n", imageData.width, imageData.height);
    
    HICON hicon = CreateIconFromImageData(imageData, 32);
    if (!hicon) {
        printf("[ICON] SetIcon: Failed to create 32x32 icon!\n");
        return false;
    }
    printf("[ICON] SetIcon: 32x32 icon created successfully, handle: %p\n", hicon);
    
    HICON hiconSm = CreateIconFromImageData(imageData, 16);
    if (!hiconSm) {
        printf("[ICON] SetIcon: Failed to create 16x16 icon, using 32x32 as fallback\n");
        hiconSm = hicon;
    } else {
        printf("[ICON] SetIcon: 16x16 icon created successfully, handle: %p\n", hiconSm);
    }
    
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
    
    SetClassLongPtr(m_hwnd, GCLP_HICON, (LONG_PTR)hicon);
    SetClassLongPtr(m_hwnd, GCLP_HICONSM, (LONG_PTR)hiconSm);
    
    InvalidateRect(m_hwnd, NULL, TRUE);
    UpdateWindow(m_hwnd);
    
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


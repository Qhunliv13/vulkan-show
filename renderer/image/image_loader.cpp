#include "image/image_loader.h"
#include "window/window.h"
#include <fstream>
#include <vector>
#include <windows.h>
#ifdef LoadImage
#undef LoadImage  // 取消Windows API的LoadImage宏定义
#endif
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// WebP支持（使用stb_image单头文件库）
// 注意：需要下载stb_image.h到项目的renderer/thirdparty/目录
#include <algorithm>
#include <cctype>

// 如果启用了stb_image支持，包含头文件
#ifdef USE_STB_IMAGE
    // stb_image.h应该放在renderer/thirdparty/目录下
    #define STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_STATIC
    #include "thirdparty/stb_image.h"
#endif

using namespace Gdiplus;
using namespace renderer::image;

// GDI+初始化辅助类
class GdiplusInitializer {
public:
    GdiplusInitializer() {
        GdiplusStartupInput gdiplusStartupInput;
        GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
    }
    
    ~GdiplusInitializer() {
        GdiplusShutdown(m_gdiplusToken);
    }
    
private:
    ULONG_PTR m_gdiplusToken;
};

static GdiplusInitializer g_gdiplusInit;

ImageData ImageLoader::LoadImage(const std::string& filepath) {
    // 检查文件扩展名
    size_t dotPos = filepath.find_last_of('.');
    if (dotPos != std::string::npos) {
        std::string ext = filepath.substr(dotPos + 1);
        // 转换为小写进行比较
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        if (ext == "png") {
            return ImageLoader::LoadPNG(filepath);
        } else if (ext == "webp") {
            return ImageLoader::LoadWebP(filepath);
        }
    }
    
    // 默认尝试PNG
    return ImageLoader::LoadPNG(filepath);
}

ImageData ImageLoader::LoadPNG(const std::string& filepath) {
    ImageData result;
    
    // 使用GDI+加载PNG
    std::wstring wpath(filepath.begin(), filepath.end());
    Bitmap* bitmap = new Bitmap(wpath.c_str());
    
    if (bitmap->GetLastStatus() != Ok) {
        Window::ShowError("Failed to load image: " + filepath);
        delete bitmap;
        return result;
    }
    
    result.width = bitmap->GetWidth();
    result.height = bitmap->GetHeight();
    result.channels = 4;  // RGBA
    
    // 分配像素数据
    result.pixels.resize(result.width * result.height * result.channels);
    
    // 读取像素数据
    BitmapData bitmapData;
    Rect rect(0, 0, result.width, result.height);
    
    if (bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) == Ok) {
        uint8_t* src = (uint8_t*)bitmapData.Scan0;
        uint8_t* dst = result.pixels.data();
        
        for (uint32_t y = 0; y < result.height; y++) {
            for (uint32_t x = 0; x < result.width; x++) {
                uint8_t* srcPixel = src + (y * bitmapData.Stride + x * 4);
                uint8_t* dstPixel = dst + ((y * result.width + x) * 4);
                
                // BGRA -> RGBA
                dstPixel[0] = srcPixel[2];  // R
                dstPixel[1] = srcPixel[1];  // G
                dstPixel[2] = srcPixel[0];  // B
                dstPixel[3] = srcPixel[3];  // A
            }
        }
        
        bitmap->UnlockBits(&bitmapData);
    }
    
    delete bitmap;
    return result;
}

ImageData ImageLoader::LoadImageFromMemory(const uint8_t* data, size_t size) {
    ImageData result;
    
    // 从内存创建流
    IStream* stream = nullptr;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!hMem) {
        return result;
    }
    
    void* pMem = GlobalLock(hMem);
    memcpy(pMem, data, size);
    GlobalUnlock(hMem);
    
    if (CreateStreamOnHGlobal(hMem, TRUE, &stream) == S_OK) {
        Bitmap* bitmap = new Bitmap(stream);
        
        if (bitmap->GetLastStatus() == Ok) {
            result.width = bitmap->GetWidth();
            result.height = bitmap->GetHeight();
            result.channels = 4;
            result.pixels.resize(result.width * result.height * result.channels);
            
            BitmapData bitmapData;
            Rect rect(0, 0, result.width, result.height);
            
            if (bitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) == Ok) {
                uint8_t* src = (uint8_t*)bitmapData.Scan0;
                uint8_t* dst = result.pixels.data();
                
                for (uint32_t y = 0; y < result.height; y++) {
                    for (uint32_t x = 0; x < result.width; x++) {
                        uint8_t* srcPixel = src + (y * bitmapData.Stride + x * 4);
                        uint8_t* dstPixel = dst + ((y * result.width + x) * 4);
                        
                        dstPixel[0] = srcPixel[2];  // R
                        dstPixel[1] = srcPixel[1];  // G
                        dstPixel[2] = srcPixel[0];  // B
                        dstPixel[3] = srcPixel[3];  // A
                    }
                }
                
                bitmap->UnlockBits(&bitmapData);
            }
        }
        
        delete bitmap;
        stream->Release();
    }
    
    GlobalFree(hMem);
    return result;
}

ImageData ImageLoader::LoadWebP(const std::string& filepath) {
    ImageData result;
    
#ifdef USE_STB_IMAGE
    // 使用stb_image加载WebP（stb_image会自动检测WebP格式）
    int width, height, channels;
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &channels, 4); // 强制RGBA
    
    if (data == nullptr) {
        Window::ShowError("Failed to load WebP file: " + filepath + " (stb_image error)");
        return result;
    }
    
    result.width = static_cast<uint32_t>(width);
    result.height = static_cast<uint32_t>(height);
    result.channels = 4;  // RGBA
    
    // 分配并复制像素数据
    result.pixels.resize(result.width * result.height * result.channels);
    memcpy(result.pixels.data(), data, result.width * result.height * result.channels);
    
    stbi_image_free(data);
    
#else
    // 如果没有stb_image，显示错误信息
    Window::ShowError("WebP support not compiled. Please download stb_image.h from https://github.com/nothings/stb and place it in renderer/thirdparty/, then define USE_STB_IMAGE in SConstruct.");
#endif
    
    return result;
}

#include "image/image_loader.h"  // 1. 对应头文件

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>  // 2. 系统头文件
#include <objidl.h>   // 2. 系统头文件（提供IStream等COM接口定义，GDI+需要）
#include <fstream>
#include <vector>
#include <algorithm>
#include <cctype>

#ifdef LoadImage
#undef LoadImage  // 取消Windows API的LoadImage宏定义
#endif
#include <gdiplus.h>  // 3. 第三方库头文件
#pragma comment(lib, "gdiplus.lib")

// WebP支持（使用stb_image单头文件库）
// 需要下载stb_image.h到项目的renderer/thirdparty/目录并定义USE_STB_IMAGE宏
#ifdef USE_STB_IMAGE
    #define STB_IMAGE_IMPLEMENTATION
    #define STB_IMAGE_STATIC
    #include "thirdparty/stb_image.h"  // 3. 第三方库头文件
#endif

#include "window/window.h"  // 4. 项目头文件

// GDI+初始化辅助类
class GdiplusInitializer {
public:
    GdiplusInitializer() {
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
    }
    
    ~GdiplusInitializer() {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
    }
    
private:
    ULONG_PTR m_gdiplusToken;
};

static GdiplusInitializer g_gdiplusInit;

renderer::image::ImageData renderer::image::ImageLoader::LoadImage(const std::string& filepath) {
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

renderer::image::ImageData renderer::image::ImageLoader::LoadPNG(const std::string& filepath) {
    ImageData result;
    
    // 使用GDI+加载PNG
    std::wstring wpath(filepath.begin(), filepath.end());
    Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(wpath.c_str());
    
    if (bitmap->GetLastStatus() != Gdiplus::Ok) {
        Window::ShowError("Failed to load image: " + filepath);
        delete bitmap;
        return result;
    }
    
    result.width = bitmap->GetWidth();
    result.height = bitmap->GetHeight();
    result.channels = 4;  // RGBA
    
    // 分配像素数据
    result.pixels.resize(result.width * result.height * result.channels);
    
    // 使用GetPixel方法逐个读取像素，避免Rect构造的宏冲突问题
    // 虽然性能略低，但更解耦，不依赖Rect构造
    for (uint32_t y = 0; y < result.height; y++) {
        for (uint32_t x = 0; x < result.width; x++) {
                    Gdiplus::Color color;
                    if (bitmap->GetPixel(static_cast<INT>(x), static_cast<INT>(y), &color) == Gdiplus::Ok) {
                        uint32_t index = (y * result.width + x) * 4;
                        // 使用GetValue()和位操作避免GetR/GetG/GetB/GetA可能的宏冲突
                        Gdiplus::ARGB argb = color.GetValue();
                        result.pixels[index] = (argb >> 16) & 0xFF;     // R
                        result.pixels[index + 1] = (argb >> 8) & 0xFF;  // G
                        result.pixels[index + 2] = argb & 0xFF;         // B
                        result.pixels[index + 3] = (argb >> 24) & 0xFF; // A
                    }
        }
    }
    
    delete bitmap;
    return result;
}

renderer::image::ImageData renderer::image::ImageLoader::LoadImageFromMemory(const uint8_t* data, size_t size) {
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
        Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(stream);
        
        if (bitmap->GetLastStatus() == Gdiplus::Ok) {
            result.width = bitmap->GetWidth();
            result.height = bitmap->GetHeight();
            result.channels = 4;
            result.pixels.resize(result.width * result.height * result.channels);
            
            // 使用GetPixel方法逐个读取像素，避免Rect构造的宏冲突问题
            // 虽然性能略低，但更解耦，不依赖Rect构造
            for (uint32_t y = 0; y < result.height; y++) {
                for (uint32_t x = 0; x < result.width; x++) {
                    Gdiplus::Color color;
                    if (bitmap->GetPixel(static_cast<INT>(x), static_cast<INT>(y), &color) == Gdiplus::Ok) {
                        uint32_t index = (y * result.width + x) * 4;
                        // 使用GetValue()和位操作避免GetR/GetG/GetB/GetA可能的宏冲突
                        Gdiplus::ARGB argb = color.GetValue();
                        result.pixels[index] = (argb >> 16) & 0xFF;     // R
                        result.pixels[index + 1] = (argb >> 8) & 0xFF;  // G
                        result.pixels[index + 2] = argb & 0xFF;         // B
                        result.pixels[index + 3] = (argb >> 24) & 0xFF; // A
                    }
                }
            }
            
            // 注释掉LockBits方法，改用GetPixel避免Rect宏冲突
            /*
            Gdiplus::BitmapData bitmapData;
            // 使用临时变量避免宏展开问题
            INT rectX = 0;
            INT rectY = 0;
            INT rectWidth = static_cast<INT>(result.width);
            INT rectHeight = static_cast<INT>(result.height);
            Gdiplus::Rect rect(rectX, rectY, rectWidth, rectHeight);
            
            if (bitmap->LockBits(&rect, Gdiplus::ImageLockModeRead, Gdiplus::PixelFormat32bppARGB, &bitmapData) == Gdiplus::Ok) {
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
            */
        }
        
        delete bitmap;
        stream->Release();
    }
    
    GlobalFree(hMem);
    return result;
}

renderer::image::ImageData renderer::image::ImageLoader::LoadWebP(const std::string& filepath) {
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

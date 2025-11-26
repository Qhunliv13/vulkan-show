#pragma once

#include <string>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件
#include <cstdint>  // 2. 系统头文件

namespace renderer {
namespace image {

// 图像数据结构 - 存储加载的图像像素数据
// 职责：提供RGBA格式的像素数据存储和基本查询功能
struct ImageData {
    std::vector<uint8_t> pixels;  // RGBA格式的像素数据
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 4;  // 通常为4（RGBA）
    
    // 获取指定位置的alpha值（0-255）
    // 用于检查像素的透明度，支持碰撞检测和透明度查询
    uint8_t GetAlpha(uint32_t x, uint32_t y) const {
        if (x >= width || y >= height) return 0;
        uint32_t index = (y * width + x) * channels;
        if (index + 3 < pixels.size()) {
            return pixels[index + 3];  // Alpha通道
        }
        return 0;
    }
    
    // 检查指定位置是否不透明（alpha > 阈值）
    // 用于快速判断像素是否可见，支持透明度阈值自定义
    bool IsOpaque(uint32_t x, uint32_t y, uint8_t threshold = 128) const {
        return GetAlpha(x, y) > threshold;
    }
};

// 图像加载器 - 从文件或内存加载图像数据
// 职责：提供统一的图像加载接口，支持PNG、WebP等格式
// 设计：使用静态方法提供加载功能，支持Windows GDI+和stb_image库
class ImageLoader {
public:
    // 从文件加载图像（支持PNG、WebP等）
    // 根据文件扩展名自动选择加载器，返回RGBA格式的像素数据
    static ImageData LoadImage(const std::string& filepath);
    
    // 从内存加载图像
    // 支持从内存缓冲区加载图像数据，用于资源嵌入或网络加载场景
    static ImageData LoadImageFromMemory(const uint8_t* data, size_t size);
    
private:
    // PNG加载实现（使用Windows GDI+）
    // 将BGRA格式转换为RGBA格式，确保跨平台一致性
    static ImageData LoadPNG(const std::string& filepath);
    
    // WebP加载实现（使用stb_image库）
    // 需要定义USE_STB_IMAGE宏并包含stb_image.h头文件
    static ImageData LoadWebP(const std::string& filepath);
};

} // namespace image
} // namespace renderer

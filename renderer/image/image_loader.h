#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace renderer {
namespace image {

// 图像数据结构
struct ImageData {
    std::vector<uint8_t> pixels;  // RGBA格式的像素数据
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 4;  // 通常为4（RGBA）
    
    // 获取指定位置的alpha值（0-255）
    uint8_t GetAlpha(uint32_t x, uint32_t y) const {
        if (x >= width || y >= height) return 0;
        uint32_t index = (y * width + x) * channels;
        if (index + 3 < pixels.size()) {
            return pixels[index + 3];  // Alpha通道
        }
        return 0;
    }
    
    // 检查指定位置是否不透明（alpha > 阈值）
    bool IsOpaque(uint32_t x, uint32_t y, uint8_t threshold = 128) const {
        return GetAlpha(x, y) > threshold;
    }
};

class ImageLoader {
public:
    // 从文件加载图像（支持PNG、WebP等）
    // 返回的ImageData包含RGBA格式的像素数据
    static ImageData LoadImage(const std::string& filepath);
    
    // 从内存加载图像
    static ImageData LoadImageFromMemory(const uint8_t* data, size_t size);
    
private:
    // PNG加载实现（使用Windows GDI+）
    static ImageData LoadPNG(const std::string& filepath);
    
    // WebP加载实现（使用libwebp）
    static ImageData LoadWebP(const std::string& filepath);
};

} // namespace image
} // namespace renderer

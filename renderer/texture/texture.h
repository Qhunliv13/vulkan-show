#pragma once

#include <cstdint>           // 2. 系统头文件
#include <string>            // 2. 系统头文件

#include <vulkan/vulkan.h>  // 3. 第三方库头文件

// 注意：如果之前包含了windows.h，LoadImage宏可能已经被定义
// 在包含image_loader.h之前取消宏定义
#ifdef LoadImage
#undef LoadImage  // 取消Windows API的LoadImage宏定义，避免与ImageLoader::LoadImage冲突
#endif

#include "image/image_loader.h"  // 4. 项目头文件

namespace renderer {
namespace texture {

// 纹理类 - 管理VkImage、VkImageView和VkSampler
// 负责从文件或图像数据创建Vulkan纹理资源，自动处理图像布局转换和内存管理
class Texture {
public:
    Texture();
    ~Texture();
    
    // 禁止拷贝
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;
    
    // 从文件加载纹理
    bool LoadFromFile(VkDevice device, VkPhysicalDevice physicalDevice, 
                      VkCommandPool commandPool, VkQueue graphicsQueue,
                      const std::string& filepath);
    
    // 从ImageData创建纹理
    bool CreateFromImageData(VkDevice device, VkPhysicalDevice physicalDevice,
                            VkCommandPool commandPool, VkQueue graphicsQueue,
                            const image::ImageData& imageData);
    
    // 设置物理设备（用于内部操作）
    void SetPhysicalDevice(VkPhysicalDevice physicalDevice) { m_physicalDevice = physicalDevice; }
    
    // 清理资源
    void Cleanup(VkDevice device);
    
    // 获取Vulkan对象
    VkImage GetImage() const { return m_image; }
    VkImageView GetImageView() const { return m_imageView; }
    VkSampler GetSampler() const { return m_sampler; }
    VkDescriptorImageInfo GetDescriptorInfo() const;
    
    // 获取纹理信息
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    VkFormat GetFormat() const { return m_format; }
    
    // 检查纹理是否有效
    bool IsValid() const { return m_image != VK_NULL_HANDLE; }

private:
    // 创建VkImage
    bool CreateImage(VkDevice device, VkPhysicalDevice physicalDevice,
                     uint32_t width, uint32_t height, VkFormat format,
                     VkImageUsageFlags usage);
    
    // 创建VkImageView
    bool CreateImageView(VkDevice device, VkFormat format, VkImageAspectFlags aspectFlags);
    
    // 创建VkSampler
    bool CreateSampler(VkDevice device);
    
    // 上传像素数据到GPU
    bool UploadImageData(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                        const image::ImageData& imageData);
    
    // 转换图像布局
    void TransitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                               VkImageLayout oldLayout, VkImageLayout newLayout);
    
    // 复制缓冲区到图像
    void CopyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                          VkBuffer buffer, uint32_t width, uint32_t height);
    
    // 查找内存类型
    uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, 
                            VkMemoryPropertyFlags properties);
    
    // 创建临时缓冲区
    VkBuffer CreateStagingBuffer(VkDevice device, VkPhysicalDevice physicalDevice, 
                                VkDeviceSize size, VkDeviceMemory& bufferMemory);
    
    // 销毁临时缓冲区
    void DestroyStagingBuffer(VkDevice device, VkBuffer buffer, VkDeviceMemory bufferMemory);

    VkImage m_image = VK_NULL_HANDLE;
    VkDeviceMemory m_imageMemory = VK_NULL_HANDLE;
    VkImageView m_imageView = VK_NULL_HANDLE;
    VkSampler m_sampler = VK_NULL_HANDLE;
    
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    VkFormat m_format = VK_FORMAT_R8G8B8A8_UNORM;
    
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
};

} // namespace texture
} // namespace renderer


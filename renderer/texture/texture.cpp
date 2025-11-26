#include "texture/texture.h"  // 1. 对应头文件

// 注意：必须在包含任何可能定义LoadImage宏的头文件之前取消宏定义
#ifdef LoadImage
#undef LoadImage
#endif

#include <cstring>     // 2. 系统头文件
#include <algorithm>    // 2. 系统头文件

#include "window/window.h"  // 4. 项目头文件

using namespace renderer::texture;
using namespace renderer::image;

Texture::Texture() {
}

Texture::~Texture() {
    // 析构函数中不清理资源，需要显式调用Cleanup
    // 原因：VkDevice可能已经被销毁，在析构函数中清理可能导致未定义行为
}

bool Texture::LoadFromFile(VkDevice device, VkPhysicalDevice physicalDevice,
                           VkCommandPool commandPool, VkQueue graphicsQueue,
                           const std::string& filepath) {
    printf("[TEXTURE] LoadFromFile: Loading texture from %s\n", filepath.c_str());
    m_device = device;
    m_physicalDevice = physicalDevice;
    
    // 加载图像数据
    // 确保LoadImage宏没有被定义
    #ifdef LoadImage
    #undef LoadImage
    #endif
    printf("[TEXTURE] Loading image data...\n");
    ImageData imageData = renderer::image::ImageLoader::LoadImage(filepath);
    if (imageData.width == 0 || imageData.height == 0) {
        printf("[TEXTURE] ERROR: Failed to load image data (width=%u, height=%u)\n", 
               imageData.width, imageData.height);
        Window::ShowError("Failed to load image: " + filepath);
        return false;
    }
    
    printf("[TEXTURE] Image data loaded: %ux%u, channels=%u, pixelCount=%zu\n",
           imageData.width, imageData.height, imageData.channels, imageData.pixels.size());
    
    bool result = CreateFromImageData(device, physicalDevice, commandPool, graphicsQueue, imageData);
    if (result) {
        printf("[TEXTURE] Texture created successfully: %ux%u, format=%u\n",
               m_width, m_height, (uint32_t)m_format);
    } else {
        printf("[TEXTURE] ERROR: Failed to create texture from image data\n");
    }
    
    return result;
}

bool Texture::CreateFromImageData(VkDevice device, VkPhysicalDevice physicalDevice,
                                  VkCommandPool commandPool, VkQueue graphicsQueue,
                                  const ImageData& imageData) {
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_width = imageData.width;
    m_height = imageData.height;
    m_format = VK_FORMAT_R8G8B8A8_UNORM;  // RGBA格式
    
    // 创建VkImage
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    if (!CreateImage(device, physicalDevice, m_width, m_height, m_format, usage)) {
        return false;
    }
    
    // 上传图像数据
    if (!UploadImageData(device, commandPool, graphicsQueue, imageData)) {
        Cleanup(device);
        return false;
    }
    
    // 创建ImageView
    if (!CreateImageView(device, m_format, VK_IMAGE_ASPECT_COLOR_BIT)) {
        Cleanup(device);
        return false;
    }
    
    // 创建Sampler
    if (!CreateSampler(device)) {
        Cleanup(device);
        return false;
    }
    
    return true;
}

void Texture::Cleanup(VkDevice device) {
    printf("[TEXTURE] Cleanup: Cleaning up texture resources (%ux%u)\n", m_width, m_height);
    
    if (m_sampler != VK_NULL_HANDLE) {
        printf("[TEXTURE] Destroying sampler\n");
        vkDestroySampler(device, m_sampler, nullptr);
        m_sampler = VK_NULL_HANDLE;
    }
    
    if (m_imageView != VK_NULL_HANDLE) {
        printf("[TEXTURE] Destroying image view\n");
        vkDestroyImageView(device, m_imageView, nullptr);
        m_imageView = VK_NULL_HANDLE;
    }
    
    if (m_image != VK_NULL_HANDLE) {
        printf("[TEXTURE] Destroying image\n");
        vkDestroyImage(device, m_image, nullptr);
        m_image = VK_NULL_HANDLE;
    }
    
    if (m_imageMemory != VK_NULL_HANDLE) {
        printf("[TEXTURE] Freeing image memory\n");
        vkFreeMemory(device, m_imageMemory, nullptr);
        m_imageMemory = VK_NULL_HANDLE;
    }
    
    m_width = 0;
    m_height = 0;
    m_device = VK_NULL_HANDLE;
    m_physicalDevice = VK_NULL_HANDLE;
    printf("[TEXTURE] Cleanup completed\n");
}

bool Texture::CreateImage(VkDevice device, VkPhysicalDevice physicalDevice,
                         uint32_t width, uint32_t height, VkFormat format,
                         VkImageUsageFlags usage) {
    printf("[TEXTURE] CreateImage: Creating VkImage %ux%u, format=%u, usage=0x%x\n",
           width, height, (uint32_t)format, usage);
    
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &m_image);
    if (result != VK_SUCCESS) {
        printf("[TEXTURE] ERROR: Failed to create VkImage, result=%d\n", result);
        Window::ShowError("Failed to create image!");
        return false;
    }
    printf("[TEXTURE] VkImage created successfully, handle=%p\n", (void*)m_image);
    
    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, m_image, &memRequirements);
    printf("[TEXTURE] Image memory requirements: size=%llu, alignment=%llu, typeBits=0x%x\n",
           memRequirements.size, memRequirements.alignment, memRequirements.memoryTypeBits);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    printf("[TEXTURE] Allocating image memory: size=%llu, typeIndex=%u\n",
           allocInfo.allocationSize, allocInfo.memoryTypeIndex);
    
    result = vkAllocateMemory(device, &allocInfo, nullptr, &m_imageMemory);
    if (result != VK_SUCCESS) {
        printf("[TEXTURE] ERROR: Failed to allocate image memory, result=%d\n", result);
        Window::ShowError("Failed to allocate image memory!");
        return false;
    }
    printf("[TEXTURE] Image memory allocated successfully, handle=%p\n", (void*)m_imageMemory);
    
    vkBindImageMemory(device, m_image, m_imageMemory, 0);
    printf("[TEXTURE] Image memory bound successfully\n");
    
    return true;
}

bool Texture::CreateImageView(VkDevice device, VkFormat format, VkImageAspectFlags aspectFlags) {
    printf("[TEXTURE] CreateImageView: Creating image view, format=%u, aspectFlags=0x%x\n",
           (uint32_t)format, aspectFlags);
    
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    VkResult result = vkCreateImageView(device, &viewInfo, nullptr, &m_imageView);
    if (result != VK_SUCCESS) {
        printf("[TEXTURE] ERROR: Failed to create image view, result=%d\n", result);
        Window::ShowError("Failed to create texture image view!");
        return false;
    }
    printf("[TEXTURE] Image view created successfully, handle=%p\n", (void*)m_imageView);
    
    return true;
}

bool Texture::CreateSampler(VkDevice device) {
    printf("[TEXTURE] CreateSampler: Creating texture sampler (linear filtering, clamp to edge addressing)\n");
    
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &m_sampler);
    if (result != VK_SUCCESS) {
        printf("[TEXTURE] ERROR: Failed to create sampler, result=%d\n", result);
        Window::ShowError("Failed to create texture sampler!");
        return false;
    }
    printf("[TEXTURE] Sampler created successfully, handle=%p\n", (void*)m_sampler);
    
    return true;
}

bool Texture::UploadImageData(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                             const ImageData& imageData) {
    VkDeviceSize imageSize = imageData.width * imageData.height * 4;  // RGBA
    printf("[TEXTURE] UploadImageData: Uploading %llu bytes (%ux%u RGBA) to GPU\n",
           imageSize, imageData.width, imageData.height);
    
    // 创建临时缓冲区
    printf("[TEXTURE] Creating staging buffer...\n");
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    stagingBuffer = CreateStagingBuffer(device, m_physicalDevice, imageSize, stagingBufferMemory);
    if (stagingBuffer == VK_NULL_HANDLE) {
        printf("[TEXTURE] ERROR: Failed to create staging buffer\n");
        return false;
    }
    printf("[TEXTURE] Staging buffer created successfully\n");
    
    // 复制数据到缓冲区
    printf("[TEXTURE] Copying pixel data to staging buffer...\n");
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, imageData.pixels.data(), static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);
    printf("[TEXTURE] Pixel data copied to staging buffer\n");
    
    // 转换图像布局为传输目标
    printf("[TEXTURE] Transitioning image layout: UNDEFINED -> TRANSFER_DST_OPTIMAL\n");
    TransitionImageLayout(device, commandPool, graphicsQueue,
                         VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    
    // 复制缓冲区到图像
    printf("[TEXTURE] Copying staging buffer to image...\n");
    CopyBufferToImage(device, commandPool, graphicsQueue, stagingBuffer, 
                      imageData.width, imageData.height);
    printf("[TEXTURE] Staging buffer copied to image\n");
    
    // 转换图像布局为着色器读取
    printf("[TEXTURE] Transitioning image layout: TRANSFER_DST_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL\n");
    TransitionImageLayout(device, commandPool, graphicsQueue,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    
    // 清理临时缓冲区
    printf("[TEXTURE] Destroying staging buffer...\n");
    DestroyStagingBuffer(device, stagingBuffer, stagingBufferMemory);
    printf("[TEXTURE] Image data uploaded successfully\n");
    
    return true;
}

void Texture::TransitionImageLayout(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                                   VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        Window::ShowError("Unsupported layout transition!");
        vkEndCommandBuffer(commandBuffer);
        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        return;
    }
    
    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
    
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void Texture::CopyBufferToImage(VkDevice device, VkCommandPool commandPool, VkQueue graphicsQueue,
                               VkBuffer buffer, uint32_t width, uint32_t height) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    
    vkCmdCopyBufferToImage(commandBuffer, buffer, m_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    vkEndCommandBuffer(commandBuffer);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);
    
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

uint32_t Texture::FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter,
                                 VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    Window::ShowError("Failed to find suitable memory type!");
    return 0;
}

VkBuffer Texture::CreateStagingBuffer(VkDevice device, VkPhysicalDevice physicalDevice,
                                      VkDeviceSize size, VkDeviceMemory& bufferMemory) {
    if (physicalDevice == VK_NULL_HANDLE) {
        Window::ShowError("Physical device is null, cannot create staging buffer!");
        return VK_NULL_HANDLE;
    }
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkBuffer stagingBuffer;
    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create staging buffer!");
        return VK_NULL_HANDLE;
    }
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(physicalDevice, memRequirements.memoryTypeBits,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    result = vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to allocate staging buffer memory!");
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        return VK_NULL_HANDLE;
    }
    
    vkBindBufferMemory(device, stagingBuffer, bufferMemory, 0);
    
    return stagingBuffer;
}

void Texture::DestroyStagingBuffer(VkDevice device, VkBuffer buffer, VkDeviceMemory bufferMemory) {
    vkDestroyBuffer(device, buffer, nullptr);
    vkFreeMemory(device, bufferMemory, nullptr);
}

VkDescriptorImageInfo Texture::GetDescriptorInfo() const {
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = m_imageView;
    imageInfo.sampler = m_sampler;
    return imageInfo;
}


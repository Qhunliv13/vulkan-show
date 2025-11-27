#include "text/text_renderer.h"  // 1. 对应头文件

#include <algorithm>  // 2. 系统头文件
#include <cmath>      // 2. 系统头文件
#include <cstring>    // 2. 系统头文件

#include <vulkan/vulkan.h>  // 3. 第三方库头文件

// 注意：直接包含shader/shader_loader.h和window/window.h是因为需要使用具体类的静态方法
// 根据开发标准第15.1节，应优先使用接口或前向声明，但静态方法需要完整定义
// 未来可考虑创建IShaderLoader接口和IErrorHandler接口以符合依赖注入原则
#include "shader/shader_loader.h"  // 4. 项目头文件
#include "window/window.h"         // 4. 项目头文件

TextRenderer::TextRenderer() {
}

TextRenderer::~TextRenderer() {
    Cleanup();
}

bool TextRenderer::Initialize(DeviceHandle device, PhysicalDeviceHandle physicalDevice,
                              CommandPoolHandle commandPool, QueueHandle graphicsQueue,
                              RenderPassHandle renderPass) {
    // 存储抽象句柄（使用不透明指针）
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_renderPass = renderPass;
    
    // 默认加载系统字体
    if (!LoadFont("Arial", 16)) {
        return false;
    }
    
    if (!CreateFontAtlas()) {
        return false;
    }
    
    if (!CreateVulkanTexture(m_atlasData.data(), m_atlasWidth, m_atlasHeight)) {
        return false;
    }
    
    if (!CreateVertexBuffer()) {
        return false;
    }
    
    // 使用 m_renderPass（不透明指针，在 CreatePipeline 中转换为 Vulkan 类型）
    if (!CreatePipeline(m_renderPass)) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void TextRenderer::Cleanup() {
    if (!m_initialized) return;
    
    // 将不透明指针转换为 Vulkan 类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    
    if (m_graphicsPipeline != nullptr) {
        vkDestroyPipeline(vkDevice, static_cast<VkPipeline>(m_graphicsPipeline), nullptr);
        m_graphicsPipeline = nullptr;
    }
    
    if (m_pipelineLayout != nullptr) {
        vkDestroyPipelineLayout(vkDevice, static_cast<VkPipelineLayout>(m_pipelineLayout), nullptr);
        m_pipelineLayout = nullptr;
    }
    
    if (m_descriptorSetLayout != nullptr) {
        vkDestroyDescriptorSetLayout(vkDevice, static_cast<VkDescriptorSetLayout>(m_descriptorSetLayout), nullptr);
        m_descriptorSetLayout = nullptr;
    }
    
    if (m_descriptorPool != nullptr) {
        vkDestroyDescriptorPool(vkDevice, static_cast<VkDescriptorPool>(m_descriptorPool), nullptr);
        m_descriptorPool = nullptr;
    }
    
    if (m_textureSampler != nullptr) {
        vkDestroySampler(vkDevice, static_cast<VkSampler>(m_textureSampler), nullptr);
        m_textureSampler = nullptr;
    }
    
    if (m_textureImageView != nullptr) {
        vkDestroyImageView(vkDevice, static_cast<VkImageView>(m_textureImageView), nullptr);
        m_textureImageView = nullptr;
    }
    
    if (m_textureImage != nullptr) {
        vkDestroyImage(vkDevice, static_cast<VkImage>(m_textureImage), nullptr);
        m_textureImage = nullptr;
    }
    
    if (m_textureImageMemory != nullptr) {
        vkFreeMemory(vkDevice, static_cast<VkDeviceMemory>(m_textureImageMemory), nullptr);
        m_textureImageMemory = nullptr;
    }
    
    if (m_vertexBuffer != nullptr) {
        vkDestroyBuffer(vkDevice, static_cast<VkBuffer>(m_vertexBuffer), nullptr);
        m_vertexBuffer = nullptr;
    }
    
    if (m_vertexBufferMemory != nullptr) {
        vkFreeMemory(vkDevice, static_cast<VkDeviceMemory>(m_vertexBufferMemory), nullptr);
        m_vertexBufferMemory = nullptr;
    }
    
    // 清理 GDI 资源
    if (m_hBitmap != nullptr) {
        DeleteObject(m_hBitmap);
        m_hBitmap = nullptr;
    }
    
    if (m_hDC != nullptr) {
        DeleteDC(m_hDC);
        m_hDC = nullptr;
    }
    
    if (m_hFont != nullptr) {
        DeleteObject(m_hFont);
        m_hFont = nullptr;
    }
    
    m_initialized = false;
}

bool TextRenderer::LoadFont(const std::string& fontName, int fontSize) {
    m_fontName = fontName;
    m_fontSize = fontSize;
    
    // 清理旧的字体资源
    if (m_hFont != nullptr) {
        DeleteObject(m_hFont);
        m_hFont = nullptr;
    }
    
    if (m_hDC != nullptr) {
        DeleteDC(m_hDC);
        m_hDC = nullptr;
    }
    
    if (m_hBitmap != nullptr) {
        DeleteObject(m_hBitmap);
        m_hBitmap = nullptr;
    }
    
    // 创建内存 DC
    HDC hScreenDC = GetDC(nullptr);
    m_hDC = CreateCompatibleDC(hScreenDC);
    ReleaseDC(nullptr, hScreenDC);
    
    if (m_hDC == nullptr) {
        return false;
    }
    
    // 创建字体
    m_hFont = CreateFontA(
        -fontSize, 0, 0, 0,
        FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE,
        fontName.c_str()
    );
    
    if (m_hFont == nullptr) {
        return false;
    }
    
    SelectObject(m_hDC, m_hFont);
    SetTextColor(m_hDC, RGB(255, 255, 255));
    SetBkColor(m_hDC, RGB(0, 0, 0));
    SetBkMode(m_hDC, TRANSPARENT);
    
    // 获取字体度量
    TEXTMETRICA tm;
    GetTextMetricsA(m_hDC, &tm);
    m_lineHeight = (float)tm.tmHeight;
    
    return true;
}

bool TextRenderer::CreateFontAtlas() {
    // 清空字形缓存
    m_glyphs.clear();
    m_currentX = 0.0f;
    m_currentY = 0.0f;
    
    // 初始化纹理图集数据（RGBA，初始为黑色透明）
    m_atlasData.resize(m_atlasWidth * m_atlasHeight * 4, 0);
    
    // 预渲染常用字符（ASCII 32-126）
    for (uint32_t c = 32; c <= 126; c++) {
        GetGlyph(c);
    }
    
    // 预渲染中文字符（常用汉字）
    // 预加载常用界面文本中的字符，减少运行时字形创建开销
    const uint32_t commonChinese[] = {
        0x52A0,  // 加
        0x8F7D,  // 载
        0x4E2D,  // 中
        0x70B9,  // 点
        0x51FB,  // 击
        0x8FDB,  // 进
        0x5165,  // 入
        0x6587, 0x6D4B, 0x8BD5, 0x5B57, 0x7B26  // 文测试字符
    };
    for (uint32_t c : commonChinese) {
        GetGlyph(c);
    }
    
    return true;
}

const TextRenderer::Glyph& TextRenderer::GetGlyph(uint32_t charCode) {
    // 检查是否已缓存
    auto it = m_glyphs.find(charCode);
    if (it != m_glyphs.end()) {
        return it->second;
    }
    
    // 创建新字形
    Glyph glyph;
    glyph.charCode = charCode;
    glyph.textureIndex = 0;
    
    // 获取字符尺寸
    SIZE size;
    wchar_t wchar = (wchar_t)charCode;
    if (GetTextExtentPoint32W(m_hDC, &wchar, 1, &size) == 0) {
        // 如果失败，尝试使用更可靠的方法
        // 对于中文字符，可能需要使用不同的方法
        ABCFLOAT abc;
        if (GetCharABCWidthsFloatW(m_hDC, charCode, charCode, &abc)) {
            size.cx = (int)(abc.abcfA + abc.abcfB + abc.abcfC);
        } else {
            // 如果还是失败，使用默认尺寸
            size.cx = m_fontSize;
        }
        size.cy = m_fontSize;
    }
    
    int charWidth = size.cx;
    int charHeight = size.cy;
    
    // 检查是否有足够空间
    const int padding = 2;
    if (m_currentX + charWidth + padding > m_atlasWidth) {
        m_currentX = 0;
        m_currentY += m_lineHeight + padding;
        if (m_currentY + charHeight + padding > m_atlasHeight) {
            // 图集已满，返回默认字形
            glyph.x = 0.0f;
            glyph.y = 0.0f;
            glyph.width = 0.0f;
            glyph.height = 0.0f;
            glyph.advanceX = (float)charWidth;
            glyph.offsetX = 0.0f;
            glyph.offsetY = 0.0f;
            m_glyphs[charCode] = glyph;
            return m_glyphs[charCode];
        }
    }
    
    // 创建临时位图用于渲染字符
    int tempWidth = charWidth + padding * 2;
    int tempHeight = charHeight + padding * 2;
    
    HDC hTempDC = CreateCompatibleDC(m_hDC);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = tempWidth;
    bmi.bmiHeader.biHeight = -tempHeight;  // 负值表示从上到下
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    
    void* tempData = nullptr;
    HBITMAP hTempBitmap = CreateDIBSection(hTempDC, &bmi, DIB_RGB_COLORS, &tempData, nullptr, 0);
    if (hTempBitmap == nullptr) {
        DeleteDC(hTempDC);
        // 返回默认字形
        glyph.x = 0.0f;
        glyph.y = 0.0f;
        glyph.width = 0.0f;
        glyph.height = 0.0f;
        glyph.advanceX = (float)charWidth;
        glyph.offsetX = 0.0f;
        glyph.offsetY = 0.0f;
        m_glyphs[charCode] = glyph;
        return m_glyphs[charCode];
    }
    
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hTempDC, hTempBitmap);
    SelectObject(hTempDC, m_hFont);
    SetTextColor(hTempDC, RGB(255, 255, 255));
    SetBkColor(hTempDC, RGB(0, 0, 0));
    SetBkMode(hTempDC, TRANSPARENT);
    
    // 渲染字符（使用之前声明的 wchar）
    TextOutW(hTempDC, padding, padding, &wchar, 1);
    
    // 获取字符的 ABC 宽度
    ABC abc;
    if (GetCharABCWidthsW(m_hDC, charCode, charCode, &abc)) {
        glyph.offsetX = (float)abc.abcA;
        glyph.advanceX = (float)(abc.abcA + abc.abcB + abc.abcC);
    } else {
        glyph.offsetX = 0.0f;
        glyph.advanceX = (float)charWidth;
    }
    
    // 获取字符基线偏移
    TEXTMETRICA tm;
    GetTextMetricsA(m_hDC, &tm);
    glyph.offsetY = (float)tm.tmAscent;
    
    // 复制到位图图集
    // 注意：Windows DIB 使用 BGRA 格式，且 DIB_RGB_COLORS 创建的位图 alpha 通道为 0
    // 由于文本是白色的，我们使用 R 通道（或任何颜色通道）的值作为 alpha
    int atlasX = (int)m_currentX;
    int atlasY = (int)m_currentY;
    
    for (int y = 0; y < tempHeight; y++) {
        for (int x = 0; x < tempWidth; x++) {
            int srcIdx = (y * tempWidth + x) * 4;
            int dstIdx = ((atlasY + y) * m_atlasWidth + (atlasX + x)) * 4;
            
            if (dstIdx + 3 < (int)m_atlasData.size()) {
                uint8_t* src = (uint8_t*)tempData + srcIdx;
                // Windows DIB 格式：B, G, R, A (但 A 为 0)
                uint8_t b = src[0];
                uint8_t g = src[1];
                uint8_t r = src[2];
                // 使用 R 通道作为 alpha（因为文本是白色的）
                uint8_t alpha = r;
                
                // 存储为 RGBA 格式
                m_atlasData[dstIdx] = 255;     // R (白色)
                m_atlasData[dstIdx + 1] = 255; // G (白色)
                m_atlasData[dstIdx + 2] = 255; // B (白色)
                m_atlasData[dstIdx + 3] = alpha; // A (从 R 通道提取)
            }
        }
    }
    
    // 设置字形信息
    glyph.x = (float)atlasX / (float)m_atlasWidth;
    glyph.y = (float)atlasY / (float)m_atlasHeight;
    glyph.width = (float)tempWidth / (float)m_atlasWidth;
    glyph.height = (float)tempHeight / (float)m_atlasHeight;
    
    // 更新当前位置
    m_currentX += tempWidth;
    
    // 清理临时资源
    SelectObject(hTempDC, hOldBitmap);
    DeleteObject(hTempBitmap);
    DeleteDC(hTempDC);
    
    // 缓存字形
    m_glyphs[charCode] = glyph;
    return m_glyphs[charCode];
}

bool TextRenderer::CreateVulkanTexture(const void* data, uint32_t width, uint32_t height) {
    // 将不透明指针转换为 Vulkan 类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    VkCommandPool vkCommandPool = static_cast<VkCommandPool>(m_commandPool);
    VkQueue vkGraphicsQueue = static_cast<VkQueue>(m_graphicsQueue);
    
    // 创建图像
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkImage vkTextureImage;
    if (vkCreateImage(vkDevice, &imageInfo, nullptr, &vkTextureImage) != VK_SUCCESS) {
        Window::ShowError("Failed to create texture image!");
        return false;
    }
    m_textureImage = vkTextureImage;
    
    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(vkDevice, vkTextureImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, 
                                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    VkDeviceMemory vkTextureImageMemory;
    if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &vkTextureImageMemory) != VK_SUCCESS) {
        Window::ShowError("Failed to allocate texture image memory!");
        return false;
    }
    m_textureImageMemory = vkTextureImageMemory;
    
    vkBindImageMemory(vkDevice, vkTextureImage, vkTextureImageMemory, 0);
    
    // 创建暂存缓冲区并上传数据
    VkDeviceSize imageSize = width * height * 4;
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = imageSize;
    bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        Window::ShowError("Failed to create staging buffer!");
        return false;
    }
    
    VkMemoryRequirements stagingMemReq;
    vkGetBufferMemoryRequirements(vkDevice, stagingBuffer, &stagingMemReq);
    
    VkMemoryAllocateInfo stagingAllocInfo = {};
    stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingAllocInfo.allocationSize = stagingMemReq.size;
    stagingAllocInfo.memoryTypeIndex = FindMemoryType(stagingMemReq.memoryTypeBits,
                                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(vkDevice, &stagingAllocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
        Window::ShowError("Failed to allocate staging buffer memory!");
        return false;
    }
    
    vkBindBufferMemory(vkDevice, stagingBuffer, stagingBufferMemory, 0);
    
    // 复制数据到暂存缓冲区
    void* mappedData;
    vkMapMemory(vkDevice, stagingBufferMemory, 0, imageSize, 0, &mappedData);
    memcpy(mappedData, data, imageSize);
    vkUnmapMemory(vkDevice, stagingBufferMemory);
    
    // 创建命令缓冲区
    VkCommandBufferAllocateInfo allocCmdInfo = {};
    allocCmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocCmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocCmdInfo.commandPool = vkCommandPool;
    allocCmdInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(vkDevice, &allocCmdInfo, &commandBuffer);
    
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    vkBeginCommandBuffer(commandBuffer, &beginInfo);
    
    // 转换图像布局
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = vkTextureImage;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    // 复制缓冲区到图像
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
    
    vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, vkTextureImage, 
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    // 转换图像布局为着色器读取
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    vkEndCommandBuffer(commandBuffer);
    
    // 提交命令缓冲区
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(vkGraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(vkGraphicsQueue);
    
    vkFreeCommandBuffers(vkDevice, vkCommandPool, 1, &commandBuffer);
    
    // 清理暂存缓冲区
    vkDestroyBuffer(vkDevice, stagingBuffer, nullptr);
    vkFreeMemory(vkDevice, stagingBufferMemory, nullptr);
    
    // 创建图像视图
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = vkTextureImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    VkImageView vkTextureImageView;
    if (vkCreateImageView(vkDevice, &viewInfo, nullptr, &vkTextureImageView) != VK_SUCCESS) {
        Window::ShowError("Failed to create texture image view!");
        return false;
    }
    m_textureImageView = vkTextureImageView;
    
    // 创建采样器
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
    
    VkSampler vkTextureSampler;
    if (vkCreateSampler(vkDevice, &samplerInfo, nullptr, &vkTextureSampler) != VK_SUCCESS) {
        Window::ShowError("Failed to create texture sampler!");
        return false;
    }
    m_textureSampler = vkTextureSampler;
    
    return true;
}

bool TextRenderer::CreatePipeline(void* renderPass) {
    // 将不透明指针转换为 Vulkan 类型
    VkRenderPass vkRenderPass = static_cast<VkRenderPass>(renderPass);
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    // 创建描述符集布局
    VkDescriptorSetLayoutBinding samplerBinding = {};
    samplerBinding.binding = 0;
    samplerBinding.descriptorCount = 1;
    samplerBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerBinding.pImmutableSamplers = nullptr;
    samplerBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerBinding;
    
    VkDescriptorSetLayout vkDescriptorSetLayout;
    if (vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &vkDescriptorSetLayout) != VK_SUCCESS) {
        Window::ShowError("Failed to create descriptor set layout!");
        return false;
    }
    m_descriptorSetLayout = vkDescriptorSetLayout;
    
    // 创建描述符池
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    
    VkDescriptorPool vkDescriptorPool;
    if (vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &vkDescriptorPool) != VK_SUCCESS) {
        Window::ShowError("Failed to create descriptor pool!");
        return false;
    }
    m_descriptorPool = vkDescriptorPool;
    
    // 分配描述符集
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vkDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &vkDescriptorSetLayout;
    
    VkDescriptorSet vkDescriptorSet;
    if (vkAllocateDescriptorSets(vkDevice, &allocInfo, &vkDescriptorSet) != VK_SUCCESS) {
        Window::ShowError("Failed to allocate descriptor set!");
        return false;
    }
    m_descriptorSet = vkDescriptorSet;
    
    // 更新描述符集
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = static_cast<VkImageView>(m_textureImageView);
    imageInfo.sampler = static_cast<VkSampler>(m_textureSampler);
    
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = vkDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;
    
    vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
    
    // 加载 shader（优先尝试加载 SPIR-V 文件，如果不存在则尝试编译 GLSL）
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    
    // 尝试加载 SPIR-V 文件
    vertShaderCode = renderer::shader::ShaderLoader::LoadSPIRV("renderer/text/text.vert.spv");
    fragShaderCode = renderer::shader::ShaderLoader::LoadSPIRV("renderer/text/text.frag.spv");
    
    // 如果 SPIR-V 文件不存在，尝试编译 GLSL
    if (vertShaderCode.empty()) {
        vertShaderCode = renderer::shader::ShaderLoader::CompileGLSLFromFile(
            "renderer/text/text.vert", ShaderStage::Vertex);
    }
    if (fragShaderCode.empty()) {
        fragShaderCode = renderer::shader::ShaderLoader::CompileGLSLFromFile(
            "renderer/text/text.frag", ShaderStage::Fragment);
    }
    
    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        Window::ShowError("Failed to load text shaders! Make sure renderer/text/text.vert.spv and renderer/text/text.frag.spv exist, or shaderc is available.");
        return false;
    }
    
    // 使用抽象类型，然后在需要时转换为Vulkan类型
    ShaderModuleHandle vertShaderModuleHandle = renderer::shader::ShaderLoader::CreateShaderModuleFromSPIRV(static_cast<DeviceHandle>(vkDevice), vertShaderCode);
    ShaderModuleHandle fragShaderModuleHandle = renderer::shader::ShaderLoader::CreateShaderModuleFromSPIRV(static_cast<DeviceHandle>(vkDevice), fragShaderCode);
    
    if (vertShaderModuleHandle == nullptr || fragShaderModuleHandle == nullptr) {
        Window::ShowError("Failed to create shader modules!");
        return false;
    }
    
    // 将抽象句柄转换为Vulkan类型用于创建管线
    VkShaderModule vertShaderModule = static_cast<VkShaderModule>(vertShaderModuleHandle);
    VkShaderModule fragShaderModule = static_cast<VkShaderModule>(fragShaderModuleHandle);
    
    // 创建 shader 阶段
    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    // 顶点输入
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(TextVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attributeDescriptions[3] = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(TextVertex, x);
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(TextVertex, u);
    
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(TextVertex, r);
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 3;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;
    
    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // 视口和裁剪（动态）
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;
    
    // 光栅化
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // 颜色混合（启用 alpha 混合）
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    // 使用标准alpha混合模式
    // 公式：result = srcColor * srcAlpha + dstColor * (1 - srcAlpha)
    // 文本按降序渲染（高层级先），这样高层级文本会覆盖低层级文本，避免叠加变粗
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    // Alpha混合：使用标准混合，但确保文本能正确覆盖背景
    // 使用ONE作为源alpha因子，ONE_MINUS_SRC_ALPHA作为目标alpha因子
    // 这样文本的alpha值会正确混合，不受背景按钮alpha值的影响
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // 动态状态
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    
    // 深度测试状态（禁用深度测试，因为渲染通道没有深度附件）
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    // Push constants
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 2; // screenSize (vec2)
    
    // 创建管线布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &vkDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    VkPipelineLayout vkPipelineLayout;
    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS) {
        Window::ShowError("Failed to create pipeline layout!");
        vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
        return false;
    }
    m_pipelineLayout = vkPipelineLayout;
    
    // 创建图形管线
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = vkPipelineLayout;
    pipelineInfo.renderPass = vkRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    
    VkPipeline vkGraphicsPipeline;
    if (vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkGraphicsPipeline) != VK_SUCCESS) {
        Window::ShowError("Failed to create graphics pipeline!");
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
        vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
        vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
        return false;
    }
    m_graphicsPipeline = vkGraphicsPipeline;
    
    // 清理 shader 模块
    vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
    
    return true;
}

uint32_t TextRenderer::FindMemoryType(uint32_t typeFilter, uint32_t properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    VkPhysicalDevice vkPhysicalDevice = static_cast<VkPhysicalDevice>(m_physicalDevice);
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    Window::ShowError("Failed to find suitable memory type!");
    return 0;
}

bool TextRenderer::CreateVertexBuffer() {
    // 将不透明指针转换为 Vulkan 类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    
    // 创建动态顶点缓冲区（可以动态更新）
    VkDeviceSize bufferSize = sizeof(TextVertex) * 1000; // 最多支持 1000 个字符（每个字符 6 个顶点）
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkBuffer vkVertexBuffer;
    if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &vkVertexBuffer) != VK_SUCCESS) {
        Window::ShowError("Failed to create vertex buffer!");
        return false;
    }
    m_vertexBuffer = vkVertexBuffer;
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkDevice, vkVertexBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits,
                                               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                                               VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    VkDeviceMemory vkVertexBufferMemory;
    if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &vkVertexBufferMemory) != VK_SUCCESS) {
        Window::ShowError("Failed to allocate vertex buffer memory!");
        return false;
    }
    m_vertexBufferMemory = vkVertexBufferMemory;
    
    vkBindBufferMemory(vkDevice, vkVertexBuffer, vkVertexBufferMemory, 0);
    
    return true;
}

void TextRenderer::BeginTextBatch() {
    m_batchVertices.clear();
    m_textBlocks.clear();
    m_inBatchMode = true;
}

void TextRenderer::AddTextToBatch(const std::string& text, float x, float y,
                                  float r, float g, float b, float a) {
    if (text.empty()) return;
    
    // 注意：这里的x, y应该是屏幕坐标系（Y向下），但AppendVerticesToBuffer期望翻转后的Y坐标
    // 所以需要确保调用者传入的y是翻转后的坐标，或者我们需要在这里翻转
    // 由于我们不知道screenHeight，假设调用者已经处理了坐标转换（翻转Y）
    AppendVerticesToBuffer(text, x, y, r, g, b, a);
}

void TextRenderer::AddTextCenteredToBatch(const std::string& text, float centerX, float centerY,
                                          float screenWidth, float screenHeight,
                                          float r, float g, float b, float a) {
    if (text.empty()) return;
    
    // 记录文本块的起始索引和中心点
    TextBlockInfo blockInfo;
    blockInfo.startIndex = m_batchVertices.size();
    
    // 获取文字尺寸
    float textWidth = 0.0f, textHeight = 0.0f;
    GetTextSize(text, textWidth, textHeight);
    
    // 计算文字左上角坐标
    float textCenterOffset = GetTextCenterOffset(text);
    float textX = centerX - textWidth / 2.0f;
    float textY = centerY + textCenterOffset;
    
    // 添加到批次（注意：这里的Y坐标会被AppendVerticesToBuffer翻转）
    float flippedY = screenHeight - textY;
    AppendVerticesToBuffer(text, textX, flippedY, r, g, b, a);
    
    // 记录文本块的结束索引
    blockInfo.endIndex = m_batchVertices.size();
    
    // 记录文本块的中心点（使用翻转后的坐标，与顶点坐标系统一致）
    // 顶点坐标是翻转后的坐标系统
    // 文本的实际位置是：textY = centerY + textCenterOffset（窗口坐标，Y向下）
    // 翻转后传递给AppendVerticesToBuffer的是：flippedY = screenHeight - textY
    // 字符位置是基于flippedY计算的（翻转后的坐标系统）
    // 所以，在缩放时，我们需要使用翻转后的中心点
    // 翻转后的中心点：flippedCenterY = screenHeight - centerY
    blockInfo.centerX = centerX;
    blockInfo.centerY = screenHeight - centerY;  // 翻转Y坐标，与顶点坐标系统一致
    
    m_textBlocks.push_back(blockInfo);
}

void TextRenderer::EndTextBatch(CommandBufferHandle commandBuffer, float screenWidth, float screenHeight,
                                float viewportX, float viewportY,
                                float scaleX, float scaleY) {
    if (m_batchVertices.empty()) return;
    
    // 将抽象句柄转换为 Vulkan 类型
    VkCommandBuffer vkCommandBuffer = static_cast<VkCommandBuffer>(commandBuffer);
    FlushBatch(vkCommandBuffer, screenWidth, screenHeight, viewportX, viewportY, scaleX, scaleY);
}

void TextRenderer::AppendVerticesToBuffer(const std::string& text, float x, float y, 
                                          float r, float g, float b, float a) {
    // 注意：这里传入的y已经是翻转后的坐标（flippedY = screenHeight - y）
    // 所以currentY是翻转后的Y坐标，字符位置计算需要考虑这一点
    
    std::vector<TextVertex> vertices;
    float currentX = x;
    float currentY = y;  // 这是翻转后的Y坐标
    
    // 将 UTF-8 字符串转换为宽字符
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wlen <= 0) return;
    
    std::vector<wchar_t> wtext(wlen);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wtext.data(), wlen);
    
    // 为每个字符创建两个三角形（6 个顶点）
    for (int i = 0; i < wlen - 1; i++) { // -1 因为末尾有 null terminator
        wchar_t wchar = wtext[i];
        const Glyph& glyph = GetGlyph((uint32_t)wchar);
        
        if (glyph.width == 0.0f || glyph.height == 0.0f) {
            // 跳过无效字符，使用advanceX保持字符间距
            currentX += glyph.advanceX;
            continue;
        }
        
        // 计算字符的屏幕位置
        float charX = currentX + glyph.offsetX;
        // 注意：currentY是传入的y坐标（窗口坐标，Y向下）
        // offsetY是字符基线偏移（正值表示字符在基线上方）
        // 所以 charY = currentY - offsetY 表示字符顶部位置
        // 但是，由于shader会翻转Y轴，我们需要确保字符位置正确
        float charY = currentY - glyph.offsetY;
        float charWidth = glyph.width * m_atlasWidth;
        float charHeight = glyph.height * m_atlasHeight;
        
        // 纹理坐标：位图是负高度（从上到下），但复制到图集时也是从上到下
        // 所以纹理坐标不需要翻转，glyph.y 是顶部，glyph.y + glyph.height 是底部
        // 但是由于 shader 翻转了 Y 轴，我们需要确保顶点顺序正确
        float texU1 = glyph.x;
        float texV1 = glyph.y;  // 纹理顶部
        float texU2 = glyph.x + glyph.width;
        float texV2 = glyph.y + glyph.height;  // 纹理底部
        
        // 创建两个三角形（矩形）
        // 由于 shader 翻转了 Y 轴，窗口坐标 Y=0（顶部）映射到 NDC y=1（顶部）
        // 所以需要交换纹理坐标的V值：窗口顶部对应纹理底部，窗口底部对应纹理顶部
        // 管线使用 COUNTER_CLOCKWISE，所以需要逆时针顺序
        // 第一个三角形（逆时针：左上 -> 右上 -> 右下）
        TextVertex v1 = {charX, charY, texU1, texV2, r, g, b, a};  // 左上，纹理底部（因为Y轴翻转）
        TextVertex v2 = {charX + charWidth, charY, texU2, texV2, r, g, b, a};  // 右上，纹理底部
        TextVertex v3 = {charX + charWidth, charY + charHeight, texU2, texV1, r, g, b, a};  // 右下，纹理顶部
        vertices.push_back(v1);
        vertices.push_back(v2);
        vertices.push_back(v3);
        
        // 第二个三角形（逆时针：左上 -> 右下 -> 左下）
        TextVertex v4 = {charX, charY, texU1, texV2, r, g, b, a};  // 左上，纹理底部
        TextVertex v5 = {charX + charWidth, charY + charHeight, texU2, texV1, r, g, b, a};  // 右下，纹理顶部
        TextVertex v6 = {charX, charY + charHeight, texU1, texV1, r, g, b, a};  // 左下，纹理顶部
        vertices.push_back(v4);
        vertices.push_back(v5);
        vertices.push_back(v6);
        
        currentX += glyph.advanceX;
    }
    
    // 将顶点追加到批次列表中
    for (const auto& v : vertices) {
        m_batchVertices.push_back(v);
    }
}

void TextRenderer::FlushBatch(void* commandBuffer, float screenWidth, float screenHeight,
                              float viewportX, float viewportY,
                              float scaleX, float scaleY) {
    // 将不透明指针转换为 Vulkan 类型
    VkCommandBuffer vkCommandBuffer = static_cast<VkCommandBuffer>(commandBuffer);
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    if (m_batchVertices.empty() || !m_initialized) {
        return;
    }
    
    // 在Fit模式下，需要对字符大小和字间距进行缩放
    // 完全参考Button::RenderText的做法：
    // - 文本中心点位置已经是窗口坐标（Button::RenderText已经转换）
    // - 但字符位置（包括字间距advanceX）和字符大小（像素值）都需要根据uiToViewportScale缩放
    // 
    // 注意：字符的位置和大小都是基于UI坐标系计算的，包括：
    // - 文本的起始位置（textX = centerX - textWidth/2，其中textWidth基于UI坐标系）
    // - 字符的偏移（offsetX, offsetY，基于UI坐标系）
    // - 字符之间的间距（advanceX，基于UI坐标系）
    // - 字符的大小（charWidth, charHeight，基于UI坐标系）
    // 所有这些都需要相对于文本中心点进行缩放
    if (scaleX != 1.0f || scaleY != 1.0f) {
        // 使用统一的缩放比例以保持字符的宽高比
        float uniformScale = (scaleX + scaleY) / 2.0f;
        
        // 对每个文本块分别处理，每个文本块作为整体进行缩放
        // 这样可以保证每个文本块内部的字间距正确缩放，而不同文本块之间不会相互影响
        for (const auto& block : m_textBlocks) {
            if (block.startIndex >= block.endIndex) continue;
            
            // 获取文本块的中心点（窗口坐标，已转换）
            float centerX = block.centerX;
            float centerY = block.centerY;
            
            // 相对于当前文本块的中心点进行缩放（保持中心点不变）
            // 这样字符的位置（包括字间距）和字符大小都会被正确缩放
            // 每个文本块独立缩放，不会相互影响
            for (size_t i = block.startIndex; i < block.endIndex; i++) {
                auto& v = m_batchVertices[i];
                // 计算相对于文本块中心点的偏移
                float offsetX = v.x - centerX;
                float offsetY = v.y - centerY;
                
                // 应用缩放，保持文本块中心点不变
                // 这样字符的位置和大小都会被缩放，包括字间距
                v.x = centerX + offsetX * uniformScale;
                v.y = centerY + offsetY * uniformScale;
            }
        }
    }
    
    // 如果有viewport偏移，调整顶点坐标（但在Fit模式下，坐标已经是窗口坐标，不需要调整）
    if (viewportX != 0.0f || viewportY != 0.0f) {
        // 注意：在Fit模式下，由于使用窗口大小作为screenSize，viewport偏移应该为0
        // 这里保留逻辑以防万一，但正常情况下不应该执行
        for (auto& vertex : m_batchVertices) {
            vertex.x -= viewportX;
            vertex.y -= viewportY;
        }
    }
    
    // 上传所有累积的顶点到GPU
    if (!m_batchVertices.empty()) {
        void* data;
        size_t bufferSize = sizeof(TextVertex) * m_batchVertices.size();
        VkDeviceMemory vkVertexBufferMemory = static_cast<VkDeviceMemory>(m_vertexBufferMemory);
        vkMapMemory(vkDevice, vkVertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_batchVertices.data(), bufferSize);
        vkUnmapMemory(vkDevice, vkVertexBufferMemory);
    }
    
    // 设置viewport和scissor（使用传入的screenSize）
    // 在Fit模式下，Button::RenderText使用窗口大小，所以这里也使用窗口大小
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = screenWidth;
    viewport.height = screenHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {(uint32_t)screenWidth, (uint32_t)screenHeight};
    
    // 绑定管线
    VkPipeline vkGraphicsPipeline = static_cast<VkPipeline>(m_graphicsPipeline);
    vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);
    
    // 设置viewport和scissor
    vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);
    
    // 绑定描述符集
    VkPipelineLayout vkPipelineLayout = static_cast<VkPipelineLayout>(m_pipelineLayout);
    VkDescriptorSet vkDescriptorSet = static_cast<VkDescriptorSet>(m_descriptorSet);
    vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                           vkPipelineLayout, 0, 1, &vkDescriptorSet, 0, nullptr);
    
    // 设置 push constants（屏幕大小）
    float screenSize[2] = {screenWidth, screenHeight};
    vkCmdPushConstants(vkCommandBuffer, vkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 
                       0, sizeof(float) * 2, screenSize);
    
    // 绑定顶点缓冲区
    VkBuffer vkVertexBuffer = static_cast<VkBuffer>(m_vertexBuffer);
    VkBuffer vertexBuffers[] = {vkVertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
    
    // 绘制所有顶点
    vkCmdDraw(vkCommandBuffer, (uint32_t)m_batchVertices.size(), 1, 0, 0);
    
    // 清空批次，准备下一帧
    m_batchVertices.clear();
    m_textBlocks.clear();
    m_inBatchMode = false;
}

void TextRenderer::UpdateVertexBuffer(const std::string& text, float x, float y, 
                                     float r, float g, float b, float a) {
    // 注意：这里传入的y已经是翻转后的坐标（flippedY = screenHeight - y）
    // 所以currentY是翻转后的Y坐标，字符位置计算需要考虑这一点
    
    // 使用AppendVerticesToBuffer生成顶点，然后立即上传到GPU（覆盖模式）
    AppendVerticesToBuffer(text, x, y, r, g, b, a);
    
    // 立即上传到GPU并清空批次（覆盖模式）
    if (!m_batchVertices.empty()) {
        // 将不透明指针转换为 Vulkan 类型
        VkDevice vkDevice = static_cast<VkDevice>(m_device);
        VkDeviceMemory vkVertexBufferMemory = static_cast<VkDeviceMemory>(m_vertexBufferMemory);
        
        void* data;
        size_t bufferSize = sizeof(TextVertex) * m_batchVertices.size();
        vkMapMemory(vkDevice, vkVertexBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, m_batchVertices.data(), bufferSize);
        vkUnmapMemory(vkDevice, vkVertexBufferMemory);
        m_batchVertices.clear();
    }
}

void TextRenderer::RenderText(CommandBufferHandle commandBuffer, const std::string& text, 
                             float x, float y, float screenWidth, float screenHeight,
                             float r, float g, float b, float a) {
    if (!m_initialized || text.empty()) {
        return;
    }
    
    // 将抽象句柄转换为 Vulkan 类型
    VkCommandBuffer vkCommandBuffer = static_cast<VkCommandBuffer>(commandBuffer);
    
    // 翻转文本绘制的Y坐标（仅翻转文字，不影响其他）
    // 窗口坐标Y向下，但文字需要翻转Y轴以正确显示
    float flippedY = screenHeight - y;
    
    // 如果处于批量模式，追加到批次而不是立即渲染
    if (m_inBatchMode) {
        AppendVerticesToBuffer(text, x, flippedY, r, g, b, a);
        return;
    }
    
    // 设置全屏viewport和scissor，确保文本不被裁剪
    // 注意：viewport和scissor应该在绑定管线之前设置，但这里在绑定管线之后设置也可以
    // 因为viewport和scissor是动态状态，可以在任何时候设置
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = screenWidth;
    viewport.height = screenHeight;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {(uint32_t)screenWidth, (uint32_t)screenHeight};
    
    // 更新顶点缓冲区
    UpdateVertexBuffer(text, x, flippedY, r, g, b, a);
    
    // 绑定管线
    VkPipeline vkGraphicsPipeline = static_cast<VkPipeline>(m_graphicsPipeline);
    vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);
    
    // 设置viewport和scissor（必须在绑定管线之后设置，因为它们是动态状态）
    vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);
    
    // 绑定描述符集
    VkPipelineLayout vkPipelineLayout = static_cast<VkPipelineLayout>(m_pipelineLayout);
    VkDescriptorSet vkDescriptorSet = static_cast<VkDescriptorSet>(m_descriptorSet);
    vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, 
                           vkPipelineLayout, 0, 1, &vkDescriptorSet, 0, nullptr);
    
    // 设置 push constants（屏幕大小）
    float screenSize[2] = {screenWidth, screenHeight};
    vkCmdPushConstants(vkCommandBuffer, vkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 
                       0, sizeof(float) * 2, screenSize);
    
    // 绑定顶点缓冲区
    VkBuffer vkVertexBuffer = static_cast<VkBuffer>(m_vertexBuffer);
    VkBuffer vertexBuffers[] = {vkVertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
    
    // 计算顶点数量（每个字符 6 个顶点）
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wlen > 0) {
        int vertexCount = (wlen - 1) * 6; // -1 因为末尾有 null terminator
        vkCmdDraw(vkCommandBuffer, vertexCount, 1, 0, 0);
    }
}

void TextRenderer::RenderTextCentered(CommandBufferHandle commandBuffer, const std::string& text,
                                      float centerX, float centerY, float screenWidth, float screenHeight,
                                      float r, float g, float b, float a) {
    if (!m_initialized || text.empty()) {
        return;
    }
    
    // 将抽象句柄转换为 Vulkan 类型
    VkCommandBuffer vkCommandBuffer = static_cast<VkCommandBuffer>(commandBuffer);
    
    // 如果处于批量模式，使用批量渲染API
    if (m_inBatchMode) {
        AddTextCenteredToBatch(text, centerX, centerY, screenWidth, screenHeight, r, g, b, a);
        return;
    }
    
    // 获取文字尺寸
    float textWidth = 0.0f, textHeight = 0.0f;
    GetTextSize(text, textWidth, textHeight);
    
    // 计算文字左上角坐标（窗口坐标，Y向下）
    // 水平居中：centerX - textWidth / 2.0f
    // 垂直居中：考虑字符基线偏移和字符高度，使用textCenterOffset调整Y坐标
    float textCenterOffset = GetTextCenterOffset(text);
    float textX = centerX - textWidth / 2.0f;
    float textY = centerY + textCenterOffset;
    
    // 调用RenderText渲染（使用左上角坐标）
    RenderText(commandBuffer, text, textX, textY, screenWidth, screenHeight, r, g, b, a);
}

void TextRenderer::GetTextSize(const std::string& text, float& width, float& height) {
    width = 0.0f;
    height = m_lineHeight;
    
    // 将 UTF-8 字符串转换为宽字符
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wlen <= 1) return; // 只有 null terminator
    
    std::vector<wchar_t> wtext(wlen);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wtext.data(), wlen);
    
    if (wlen <= 1) return; // 没有有效字符
    
    // 计算文字的实际宽度（考虑offsetX和实际字符宽度）
    // 模拟渲染过程：第一个字符的左边界和最后一个字符的右边界
    float firstCharOffsetX = 0.0f;
    float lastCharRightEdge = 0.0f;
    float currentX = 0.0f;
    const Glyph* lastValidGlyph = nullptr;
    float lastValidCurrentX = 0.0f;
    
    // 计算文字的实际高度（考虑offsetY和字符高度）
    float minCharTop = 0.0f;
    float maxCharBottom = 0.0f;
    bool firstChar = true;
    
    for (int i = 0; i < wlen - 1; i++) { // -1 因为末尾有 null terminator
        wchar_t wchar = wtext[i];
        const Glyph& glyph = GetGlyph((uint32_t)wchar);
        
        if (i == 0) {
            // 第一个字符的左边界
            firstCharOffsetX = glyph.offsetX;
        }
        
        if (glyph.width > 0.0f && glyph.height > 0.0f) {
            // 记录最后一个有效字符的信息
            lastValidGlyph = &glyph;
            lastValidCurrentX = currentX;
            
            // 计算字符的顶部和底部（相对于基线）
            // charY = currentY - glyph.offsetY，所以字符顶部是 -offsetY
            // 字符底部是 -offsetY + charHeight
            float charTop = -glyph.offsetY;
            float charBottom = -glyph.offsetY + glyph.height * m_atlasHeight;
            
            if (firstChar) {
                minCharTop = charTop;
                maxCharBottom = charBottom;
                firstChar = false;
            } else {
                if (charTop < minCharTop) minCharTop = charTop;
                if (charBottom > maxCharBottom) maxCharBottom = charBottom;
            }
        }
        
        currentX += glyph.advanceX;
    }
    
    if (lastValidGlyph) {
        // 最后一个字符的右边界 = currentX位置（在加上advanceX之前） + offsetX + 实际宽度
        lastCharRightEdge = lastValidCurrentX + lastValidGlyph->offsetX + lastValidGlyph->width * m_atlasWidth;
    }
    
    // 文字的实际宽度 = 最后一个字符的右边界 - 第一个字符的左边界
    width = lastCharRightEdge - firstCharOffsetX;
    
    // 文字的实际高度 = 字符的最大底部 - 字符的最小顶部
    if (!firstChar) {
        height = maxCharBottom - minCharTop;
    }
}

float TextRenderer::GetTextCenterOffset(const std::string& text) {
    // 计算文字中心相对于传入Y坐标的偏移
    // 在UpdateVertexBuffer中，字符Y坐标是：charY = currentY - glyph.offsetY
    // 其中currentY是翻转后的Y坐标（flippedY = screenHeight - y）
    // 字符顶部：charY = flippedY - glyph.offsetY
    // 字符底部：charY + charHeight = flippedY - glyph.offsetY + charHeight
    // 文字中心（在翻转后的坐标系中）：flippedY - averageOffsetY + averageCharHeight/2
    
    // 将 UTF-8 字符串转换为宽字符
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (wlen <= 1) return 0.0f; // 没有有效字符
    
    std::vector<wchar_t> wtext(wlen);
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wtext.data(), wlen);
    
    // 计算所有字符的平均offsetY和平均charHeight
    float sumOffsetY = 0.0f;
    float sumCharHeight = 0.0f;
    int validCharCount = 0;
    
    for (int i = 0; i < wlen - 1; i++) { // -1 因为末尾有 null terminator
        wchar_t wchar = wtext[i];
        const Glyph& glyph = GetGlyph((uint32_t)wchar);
        
        if (glyph.width > 0.0f && glyph.height > 0.0f) {
            sumOffsetY += glyph.offsetY;
            sumCharHeight += glyph.height * m_atlasHeight;
            validCharCount++;
        }
    }
    
    if (validCharCount == 0) return 0.0f;
    
    float avgOffsetY = sumOffsetY / validCharCount;
    float avgCharHeight = sumCharHeight / validCharCount;
    
    // 文字中心相对于传入Y坐标的偏移
    // 在翻转后的坐标系中，文字中心是：flippedY - avgOffsetY + avgCharHeight/2
    // 相对于传入的y坐标（翻转前），文字中心的偏移是：-avgOffsetY + avgCharHeight/2
    // 但是，由于我们需要考虑坐标翻转，实际偏移需要调整
    // 传入y坐标 -> flippedY = screenHeight - y
    // 字符Y坐标：charY = flippedY - offsetY = screenHeight - y - offsetY
    // 字符中心：charY + charHeight/2 = screenHeight - y - offsetY + charHeight/2
    // 相对于传入y坐标的偏移：-offsetY + charHeight/2
    
    // 由于shader会再次翻转Y轴，所以最终显示时：
    // 窗口坐标Y=0（顶部）-> NDC y=1（顶部）
    // 窗口坐标Y=height（底部）-> NDC y=-1（底部）
    // 所以如果我们想要文字中心在按钮中心，需要：
    // 文字中心Y（窗口坐标）= 按钮中心Y（窗口坐标）
    // 但是，由于字符Y坐标是 charY = flippedY - offsetY，我们需要补偿offsetY
    
    // 文字中心相对于传入Y坐标的偏移（正值表示文字中心在Y坐标下方）
    return -avgOffsetY + avgCharHeight / 2.0f;
}

void TextRenderer::SetFontSize(int fontSize) {
    if (fontSize == m_fontSize) {
        return;
    }
    
    // 重新加载字体并重建图集
    if (LoadFont(m_fontName, fontSize)) {
        CreateFontAtlas();
        // 需要重新创建 Vulkan 纹理
        if (m_textureImage != nullptr) {
            // 将不透明指针转换为 Vulkan 类型
            VkDevice vkDevice = static_cast<VkDevice>(m_device);
            VkSampler vkTextureSampler = static_cast<VkSampler>(m_textureSampler);
            VkImageView vkTextureImageView = static_cast<VkImageView>(m_textureImageView);
            VkImage vkTextureImage = static_cast<VkImage>(m_textureImage);
            VkDeviceMemory vkTextureImageMemory = static_cast<VkDeviceMemory>(m_textureImageMemory);
            
            vkDestroySampler(vkDevice, vkTextureSampler, nullptr);
            vkDestroyImageView(vkDevice, vkTextureImageView, nullptr);
            vkDestroyImage(vkDevice, vkTextureImage, nullptr);
            vkFreeMemory(vkDevice, vkTextureImageMemory, nullptr);
        }
        CreateVulkanTexture(m_atlasData.data(), m_atlasWidth, m_atlasHeight);
        
        // 更新描述符集
        VkDevice vkDevice = static_cast<VkDevice>(m_device);
        VkDescriptorSet vkDescriptorSet = static_cast<VkDescriptorSet>(m_descriptorSet);
        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = static_cast<VkImageView>(m_textureImageView);
        imageInfo.sampler = static_cast<VkSampler>(m_textureSampler);
        
        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = vkDescriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pImageInfo = &imageInfo;
        
        vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
    }
}
    
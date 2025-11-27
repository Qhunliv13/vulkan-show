#include "loading/loading_animation.h"  // 1. 对应头文件

#include <algorithm>  // 2. 系统头文件
#include <cmath>      // 2. 系统头文件
#include <cstdio>     // 2. 系统头文件
#include <fstream>    // 2. 系统头文件
#include <windows.h>  // 2. 系统头文件

#include <vulkan/vulkan.h>  // 3. 第三方库头文件

// 注意：直接包含shader/shader_loader.h和window/window.h是因为需要使用具体类的静态方法
// 根据开发标准第15.1节，应优先使用接口或前向声明，但静态方法需要完整定义
// 未来可考虑创建IShaderLoader接口和IErrorHandler接口以符合依赖注入原则
#include "shader/shader_loader.h"  // 4. 项目头文件
#include "window/window.h"         // 4. 项目头文件

LoadingAnimation::LoadingAnimation() {
}

LoadingAnimation::~LoadingAnimation() {
    Cleanup();
}

bool LoadingAnimation::Initialize(DeviceHandle device, PhysicalDeviceHandle physicalDevice, CommandPoolHandle commandPool, 
                                  QueueHandle graphicsQueue, RenderPassHandle renderPass, Extent2D swapchainExtent) {
    // 存储抽象句柄（在实现层转换为Vulkan类型使用）
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_renderPass = renderPass;
    m_swapchainExtent = swapchainExtent;
    
    InitializeBoxAnimation();
    
    if (!CreateBuffers()) {
        return false;
    }
    
    if (!CreatePipeline(renderPass)) {
        return false;
    }
    
    m_initialized = true;
    return true;
}

void LoadingAnimation::Cleanup() {
    if (!m_initialized) return;
    
    // 将抽象句柄转换为Vulkan类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    
    if (m_graphicsPipeline != nullptr) {
        vkDestroyPipeline(vkDevice, static_cast<VkPipeline>(m_graphicsPipeline), nullptr);
        m_graphicsPipeline = nullptr;
    }
    
    if (m_pipelineLayout != nullptr) {
        vkDestroyPipelineLayout(vkDevice, static_cast<VkPipelineLayout>(m_pipelineLayout), nullptr);
        m_pipelineLayout = nullptr;
    }
    
    // 清理所有方块的顶点缓冲区
    for (size_t i = 0; i < m_vertexBuffers.size(); i++) {
        if (m_vertexBuffers[i] != nullptr) {
            vkDestroyBuffer(vkDevice, static_cast<VkBuffer>(m_vertexBuffers[i]), nullptr);
            m_vertexBuffers[i] = nullptr;
        }
        if (m_vertexBufferMemories[i] != nullptr) {
            vkFreeMemory(vkDevice, static_cast<VkDeviceMemory>(m_vertexBufferMemories[i]), nullptr);
            m_vertexBufferMemories[i] = nullptr;
        }
    }
    m_vertexBuffers.clear();
    m_vertexBufferMemories.clear();
    
    m_initialized = false;
}

void LoadingAnimation::InitializeBoxAnimation() {
    m_boxes.clear();
    m_boxes.resize(BOX_COUNT);
    
    // 初始化方块颜色数组
    m_boxColors.resize(BOX_COUNT);
    for (int i = 0; i < BOX_COUNT; i++) {
        m_boxColors[i].r = 1.0f;
        m_boxColors[i].g = 1.0f;
        m_boxColors[i].b = 1.0f;
        m_boxColors[i].a = 1.0f;
    }
    
    // 初始化9个方块的网格位置（3x3）
    // 根据CSS：.banter-loader是72x72px，居中显示
    for (int i = 0; i < BOX_COUNT; i++) {
        int row = i / (int)GRID_SIZE;
        int col = i % (int)GRID_SIZE;
        
        m_boxes[i].boxIndex = i;
        m_boxes[i].baseX = col * (BOX_SIZE + BOX_SPACING);
        m_boxes[i].baseY = row * (BOX_SIZE + BOX_SPACING);
        
        // 根据CSS特殊规则调整初始位置
        // .banter-loader__box:nth-child(1):before, .banter-loader__box:nth-child(4):before { margin-left: 26px; }
        if (i == 0 || i == 3) {
            m_boxes[i].baseX += 26.0f;
        }
        
        // .banter-loader__box:nth-child(3):before { margin-top: 52px; }
        if (i == 2) {
            m_boxes[i].baseY += 52.0f;
        }
        
        m_boxes[i].x = m_boxes[i].baseX;
        m_boxes[i].y = m_boxes[i].baseY;
    }
}

void LoadingAnimation::UpdateBoxPosition(BoxAnimation& box, float time) {
    // 根据CSS动画定义移动路径
    // 动画周期为0.5秒（大幅加快，原为4秒）
    float cycleTime = fmodf(time, 2.5f);
    float progress = cycleTime / 2.5f;
    
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    
    // 根据boxIndex应用不同的动画路径
    // 这里简化实现，使用CSS中定义的路径
    switch (box.boxIndex) {
        case 0: { // moveBox-1
            if (progress < 0.090909f) offsetX = -26.0f;
            else if (progress < 0.181818f) offsetX = 0.0f;
            else if (progress < 0.272727f) offsetX = 0.0f;
            else if (progress < 0.363636f) offsetX = 26.0f;
            else if (progress < 0.454545f) { offsetX = 26.0f; offsetY = 26.0f; }
            else if (progress < 0.545455f) { offsetX = 26.0f; offsetY = 26.0f; }
            else if (progress < 0.636364f) { offsetX = 26.0f; offsetY = 26.0f; }
            else if (progress < 0.727273f) { offsetX = 26.0f; offsetY = 0.0f; }
            else if (progress < 0.818182f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.909091f) { offsetX = -26.0f; offsetY = 0.0f; }
            else { offsetX = 0.0f; offsetY = 0.0f; }
            break;
        }
        case 1: { // moveBox-2
            if (progress < 0.090909f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.181818f) { offsetX = 26.0f; offsetY = 0.0f; }
            else if (progress < 0.272727f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.363636f) { offsetX = 26.0f; offsetY = 0.0f; }
            else if (progress < 0.454545f) { offsetX = 26.0f; offsetY = 26.0f; }
            else if (progress < 0.545455f) { offsetX = 26.0f; offsetY = 26.0f; }
            else if (progress < 0.636364f) { offsetX = 26.0f; offsetY = 26.0f; }
            else if (progress < 0.727273f) { offsetX = 26.0f; offsetY = 26.0f; }
            else if (progress < 0.818182f) { offsetX = 0.0f; offsetY = 26.0f; }
            else if (progress < 0.909091f) { offsetX = 0.0f; offsetY = 26.0f; }
            else { offsetX = 0.0f; offsetY = 0.0f; }
            break;
        }
        case 2: { // moveBox-3
            if (progress < 0.090909f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.181818f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.272727f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.363636f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.454545f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.545455f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.636364f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.727273f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.818182f) { offsetX = -26.0f; offsetY = -26.0f; }
            else if (progress < 0.909091f) { offsetX = 0.0f; offsetY = -26.0f; }
            else { offsetX = 0.0f; offsetY = 0.0f; }
            break;
        }
        case 3: { // moveBox-4
            if (progress < 0.090909f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.181818f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.272727f) { offsetX = -26.0f; offsetY = -26.0f; }
            else if (progress < 0.363636f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.454545f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.545455f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.636364f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.727273f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.818182f) { offsetX = -26.0f; offsetY = -26.0f; }
            else if (progress < 0.909091f) { offsetX = -26.0f; offsetY = 0.0f; }
            else { offsetX = 0.0f; offsetY = 0.0f; }
            break;
        }
        case 4: { // moveBox-5
            if (progress < 0.090909f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.181818f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.272727f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.363636f) { offsetX = 26.0f; offsetY = 0.0f; }
            else if (progress < 0.454545f) { offsetX = 26.0f; offsetY = 0.0f; }
            else if (progress < 0.545455f) { offsetX = 26.0f; offsetY = 0.0f; }
            else if (progress < 0.636364f) { offsetX = 26.0f; offsetY = 0.0f; }
            else if (progress < 0.727273f) { offsetX = 26.0f; offsetY = 0.0f; }
            else if (progress < 0.818182f) { offsetX = 26.0f; offsetY = -26.0f; }
            else if (progress < 0.909091f) { offsetX = 0.0f; offsetY = -26.0f; }
            else { offsetX = 0.0f; offsetY = 0.0f; }
            break;
        }
        case 5: { // moveBox-6
            if (progress < 0.090909f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.181818f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.272727f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.363636f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.454545f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.545455f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.636364f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.727273f) { offsetX = 0.0f; offsetY = 26.0f; }
            else if (progress < 0.818182f) { offsetX = -26.0f; offsetY = 26.0f; }
            else if (progress < 0.909091f) { offsetX = -26.0f; offsetY = 0.0f; }
            else { offsetX = 0.0f; offsetY = 0.0f; }
            break;
        }
        case 6: { // moveBox-7
            if (progress < 0.090909f) { offsetX = 26.0f; offsetY = 0.0f; }
            else if (progress < 0.181818f) { offsetX = 26.0f; offsetY = 0.0f; }
            else if (progress < 0.272727f) { offsetX = 26.0f; offsetY = 0.0f; }
            else if (progress < 0.363636f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.454545f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.545455f) { offsetX = 26.0f; offsetY = -26.0f; }
            else if (progress < 0.636364f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.727273f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.818182f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.909091f) { offsetX = 26.0f; offsetY = 0.0f; }
            else { offsetX = 0.0f; offsetY = 0.0f; }
            break;
        }
        case 7: { // moveBox-8
            if (progress < 0.090909f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.181818f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.272727f) { offsetX = -26.0f; offsetY = -26.0f; }
            else if (progress < 0.363636f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.454545f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.545455f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.636364f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.727273f) { offsetX = 0.0f; offsetY = -26.0f; }
            else if (progress < 0.818182f) { offsetX = 26.0f; offsetY = -26.0f; }
            else if (progress < 0.909091f) { offsetX = 26.0f; offsetY = 0.0f; }
            else { offsetX = 0.0f; offsetY = 0.0f; }
            break;
        }
        case 8: { // moveBox-9
            if (progress < 0.090909f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.181818f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.272727f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.363636f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.454545f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.545455f) { offsetX = 0.0f; offsetY = 0.0f; }
            else if (progress < 0.636364f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.727273f) { offsetX = -26.0f; offsetY = 0.0f; }
            else if (progress < 0.818182f) { offsetX = -52.0f; offsetY = 0.0f; }
            else if (progress < 0.909091f) { offsetX = -26.0f; offsetY = 0.0f; }
            else { offsetX = 0.0f; offsetY = 0.0f; }
            break;
        }
    }
    
    box.x = box.baseX + offsetX;
    box.y = box.baseY + offsetY;
}

void LoadingAnimation::Update(float time) {
    // 更新所有方块位置
    for (auto& box : m_boxes) {
        UpdateBoxPosition(box, time);
    }
}

bool LoadingAnimation::CreateBuffers() {
    // 将抽象句柄转换为Vulkan类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    
    // 为每个方块创建独立的顶点缓冲区
    // 每个方块需要6个顶点（2个三角形组成矩形）
    struct Vertex {
        float x, y;
        float r, g, b, a;  // 颜色
    };
    
    // 创建一个方块的顶点数据模板（单位矩形，在Render中通过变换定位）
    Vertex vertexTemplate[] = {
        // 第一个三角形
        {0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f},  // 左上
        {1.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f},  // 右上
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},  // 右下
        // 第二个三角形
        {0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f},  // 左上
        {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},  // 右下
        {0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f},  // 左下
    };
    
    VkDeviceSize bufferSize = sizeof(vertexTemplate);
    
    // 为每个方块创建缓冲区
    m_vertexBuffers.resize(BOX_COUNT);
    m_vertexBufferMemories.resize(BOX_COUNT);
    
    for (int i = 0; i < BOX_COUNT; i++) {
        // 创建缓冲区
        VkBufferCreateInfo bufferInfo = {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        
        VkBuffer vkBuffer;
        if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &vkBuffer) != VK_SUCCESS) {
            Window::ShowError("Failed to create vertex buffer for loading animation!");
            // 清理已创建的缓冲区
            for (int j = 0; j < i; j++) {
                vkDestroyBuffer(vkDevice, static_cast<VkBuffer>(m_vertexBuffers[j]), nullptr);
                vkFreeMemory(vkDevice, static_cast<VkDeviceMemory>(m_vertexBufferMemories[j]), nullptr);
            }
            return false;
        }
        m_vertexBuffers[i] = vkBuffer;
        
        // 分配内存
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(vkDevice, vkBuffer, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        // 将抽象类型转换为Vulkan类型
        MemoryPropertyFlag properties = MemoryPropertyFlag::HostVisible | MemoryPropertyFlag::HostCoherent;
        uint32_t memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);
        
        // 检查内存类型查找是否成功
        if (memoryTypeIndex == UINT32_MAX) {
            Window::ShowError("Failed to find suitable memory type for loading animation!");
            vkDestroyBuffer(vkDevice, vkBuffer, nullptr);
            // 清理已创建的缓冲区
            for (int j = 0; j < i; j++) {
                vkDestroyBuffer(vkDevice, static_cast<VkBuffer>(m_vertexBuffers[j]), nullptr);
                vkFreeMemory(vkDevice, static_cast<VkDeviceMemory>(m_vertexBufferMemories[j]), nullptr);
            }
            return false;
        }
        
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        VkDeviceMemory vkMemory;
        if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &vkMemory) != VK_SUCCESS) {
            Window::ShowError("Failed to allocate vertex buffer memory for loading animation!");
            // 清理已创建的缓冲区
            vkDestroyBuffer(vkDevice, vkBuffer, nullptr);
            for (int j = 0; j < i; j++) {
                vkDestroyBuffer(vkDevice, static_cast<VkBuffer>(m_vertexBuffers[j]), nullptr);
                vkFreeMemory(vkDevice, static_cast<VkDeviceMemory>(m_vertexBufferMemories[j]), nullptr);
            }
            return false;
        }
        m_vertexBufferMemories[i] = vkMemory;
        
        vkBindBufferMemory(vkDevice, vkBuffer, vkMemory, 0);
        
        // 使用当前方块的颜色填充数据
        Vertex vertices[6];
        memcpy(vertices, vertexTemplate, sizeof(vertexTemplate));
        for (int j = 0; j < 6; j++) {
            vertices[j].r = m_boxColors[i].r;
            vertices[j].g = m_boxColors[i].g;
            vertices[j].b = m_boxColors[i].b;
            vertices[j].a = m_boxColors[i].a;
        }
        
        // 填充数据
        void* data;
        vkMapMemory(vkDevice, vkMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices, (size_t)bufferSize);
        vkUnmapMemory(vkDevice, vkMemory);
    }
    
    return true;
}

uint32_t LoadingAnimation::FindMemoryType(uint32_t typeFilter, MemoryPropertyFlag properties) {
    // 将抽象句柄转换为Vulkan类型
    VkPhysicalDevice vkPhysicalDevice = static_cast<VkPhysicalDevice>(m_physicalDevice);
    
    // 将抽象类型转换为Vulkan类型
    VkMemoryPropertyFlags vkProperties = 0;
    if ((properties & MemoryPropertyFlag::DeviceLocal) != MemoryPropertyFlag::None) {
        vkProperties |= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }
    if ((properties & MemoryPropertyFlag::HostVisible) != MemoryPropertyFlag::None) {
        vkProperties |= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }
    if ((properties & MemoryPropertyFlag::HostCoherent) != MemoryPropertyFlag::None) {
        vkProperties |= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }
    if ((properties & MemoryPropertyFlag::HostCached) != MemoryPropertyFlag::None) {
        vkProperties |= VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
    }
    
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & vkProperties) == vkProperties) {
            return i;
        }
    }
    
    // 未找到匹配的内存类型
    Window::ShowError("Failed to find suitable memory type for loading animation!");
    // 返回无效值，调用者应该检查此值
    return UINT32_MAX;
}

bool LoadingAnimation::CreatePipeline(RenderPassHandle renderPass) {
    // 将抽象句柄转换为Vulkan类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    VkRenderPass vkRenderPass = static_cast<VkRenderPass>(renderPass);
    // 加载shader
    std::vector<char> vertCode = renderer::shader::ShaderLoader::LoadSPIRV("renderer/loading/loading.vert.spv");
    std::vector<char> fragCode = renderer::shader::ShaderLoader::LoadSPIRV("renderer/loading/loading.frag.spv");
    
    if (vertCode.empty() || fragCode.empty()) {
        // 如果SPIR-V文件不存在，尝试从源码编译
        #ifdef USE_SHADERC
        std::ifstream vertFile("renderer/loading/loading.vert");
        std::ifstream fragFile("renderer/loading/loading.frag");
        if (vertFile.is_open() && fragFile.is_open()) {
            std::string vertSource((std::istreambuf_iterator<char>(vertFile)), std::istreambuf_iterator<char>());
            std::string fragSource((std::istreambuf_iterator<char>(fragFile)), std::istreambuf_iterator<char>());
            // 将抽象类型传递给ShaderLoader
            vertCode = renderer::shader::ShaderLoader::CompileGLSLFromSource(vertSource, ShaderStage::Vertex, "loading.vert");
            fragCode = renderer::shader::ShaderLoader::CompileGLSLFromSource(fragSource, ShaderStage::Fragment, "loading.frag");
        }
        #endif
    }
    
    if (vertCode.empty() || fragCode.empty()) {
        Window::ShowError("Failed to load shaders for loading animation!");
        return false;
    }
    
    ShaderModuleHandle vertShaderModuleHandle = renderer::shader::ShaderLoader::CreateShaderModuleFromSPIRV(m_device, vertCode);
    ShaderModuleHandle fragShaderModuleHandle = renderer::shader::ShaderLoader::CreateShaderModuleFromSPIRV(m_device, fragCode);
    
    if (vertShaderModuleHandle == nullptr || fragShaderModuleHandle == nullptr) {
        Window::ShowError("Failed to create shader modules for loading animation!");
        return false;
    }
    
    // 将抽象句柄转换为Vulkan类型用于创建管线
    VkShaderModule vertShaderModule = static_cast<VkShaderModule>(vertShaderModuleHandle);
    VkShaderModule fragShaderModule = static_cast<VkShaderModule>(fragShaderModuleHandle);
    
    // Shader阶段
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
    bindingDescription.stride = sizeof(float) * 6; // x, y, r, g, b, a
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attributeDescriptions[2] = {};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = sizeof(float) * 2;
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 2;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions;
    
    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // 视口（使用动态状态，在渲染时设置）
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;  // Dynamic
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;    // Dynamic
    
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
    
    // 颜色混合
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
                                          VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // Dynamic states (viewport and scissor)
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    
    // Push constants
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 6; // position(2) + size(2) + screenSize(2)
    
    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    VkPipelineLayout vkPipelineLayout;
    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
        vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
        Window::ShowError("Failed to create pipeline layout for loading animation!");
        return false;
    }
    m_pipelineLayout = vkPipelineLayout;
    
    // 创建管线
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = vkPipelineLayout;
    pipelineInfo.renderPass = vkRenderPass;
    pipelineInfo.subpass = 0;
    
    VkPipeline vkGraphicsPipeline;
    VkResult result = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkGraphicsPipeline);
    
    vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
    
    if (result != VK_SUCCESS) {
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
        Window::ShowError("Failed to create graphics pipeline for loading animation!");
        return false;
    }
    m_graphicsPipeline = vkGraphicsPipeline;
    
    return true;
}

void LoadingAnimation::Render(CommandBufferHandle commandBuffer, Extent2D extent) {
    if (!m_initialized || m_graphicsPipeline == nullptr) return;
    
    // 将抽象句柄转换为Vulkan类型
    VkCommandBuffer vkCommandBuffer = static_cast<VkCommandBuffer>(commandBuffer);
    VkPipeline vkGraphicsPipeline = static_cast<VkPipeline>(m_graphicsPipeline);
    VkPipelineLayout vkPipelineLayout = static_cast<VkPipelineLayout>(m_pipelineLayout);
    
    // 绑定管线
    vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);
    
    // 渲染每个方块
    for (size_t i = 0; i < m_boxes.size(); i++) {
        const auto& box = m_boxes[i];
        
        // 绑定该方块的顶点缓冲区
        VkBuffer vkVertexBuffer = static_cast<VkBuffer>(m_vertexBuffers[i]);
        VkBuffer vertexBuffers[] = {vkVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
        
        // Push constants: position, size, screenSize
        // Position in window coordinates (Y down, origin at top-left)
        // Shader will handle Y-axis flip internally
        float pushConstants[6] = {
            m_posX + box.x,           // position.x (window coordinates)
            m_posY + box.y,           // position.y (window coordinates, Y down)
            BOX_SIZE,                 // size.x
            BOX_SIZE,                 // size.y
            (float)extent.width,      // screenSize.x
            (float)extent.height      // screenSize.y
        };
        
        vkCmdPushConstants(vkCommandBuffer, vkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 
                          0, sizeof(pushConstants), pushConstants);
        
        // 绘制方块（6个顶点）
        vkCmdDraw(vkCommandBuffer, 6, 1, 0, 0);
    }
}

void LoadingAnimation::SetBoxColor(float r, float g, float b, float a) {
    // 设置所有方块的颜色
    for (int i = 0; i < BOX_COUNT; i++) {
        SetBoxColor(i, r, g, b, a);
    }
}

void LoadingAnimation::SetBoxColor(int boxIndex, float r, float g, float b, float a) {
    if (boxIndex < 0 || boxIndex >= BOX_COUNT) return;
    
    // 更新颜色
    m_boxColors[boxIndex].r = r;
    m_boxColors[boxIndex].g = g;
    m_boxColors[boxIndex].b = b;
    m_boxColors[boxIndex].a = a;
    
    // 更新该方块的顶点缓冲区
    UpdateBoxColorBuffer(boxIndex);
}

void LoadingAnimation::UpdateBoxColorBuffer() {
    // 更新所有方块的缓冲区
    for (int i = 0; i < BOX_COUNT; i++) {
        UpdateBoxColorBuffer(i);
    }
}

void LoadingAnimation::UpdateBoxColorBuffer(int boxIndex) {
    if (!m_initialized || boxIndex < 0 || boxIndex >= BOX_COUNT) return;
    if (m_vertexBufferMemories[boxIndex] == nullptr) return;
    
    // 将抽象句柄转换为Vulkan类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    VkDeviceMemory vkMemory = static_cast<VkDeviceMemory>(m_vertexBufferMemories[boxIndex]);
    
    struct Vertex {
        float x, y;
        float r, g, b, a;
    };
    
    // 创建一个方块的顶点数据（单位矩形）
    Vertex vertices[] = {
        // 第一个三角形
        {0.0f, 0.0f, m_boxColors[boxIndex].r, m_boxColors[boxIndex].g, m_boxColors[boxIndex].b, m_boxColors[boxIndex].a},  // 左上
        {1.0f, 0.0f, m_boxColors[boxIndex].r, m_boxColors[boxIndex].g, m_boxColors[boxIndex].b, m_boxColors[boxIndex].a},  // 右上
        {1.0f, 1.0f, m_boxColors[boxIndex].r, m_boxColors[boxIndex].g, m_boxColors[boxIndex].b, m_boxColors[boxIndex].a},  // 右下
        // 第二个三角形
        {0.0f, 0.0f, m_boxColors[boxIndex].r, m_boxColors[boxIndex].g, m_boxColors[boxIndex].b, m_boxColors[boxIndex].a},  // 左上
        {1.0f, 1.0f, m_boxColors[boxIndex].r, m_boxColors[boxIndex].g, m_boxColors[boxIndex].b, m_boxColors[boxIndex].a},  // 右下
        {0.0f, 1.0f, m_boxColors[boxIndex].r, m_boxColors[boxIndex].g, m_boxColors[boxIndex].b, m_boxColors[boxIndex].a},  // 左下
    };
    
    VkDeviceSize bufferSize = sizeof(vertices);
    
    // 更新缓冲区数据（使用已转换的Vulkan类型）
    void* data;
    vkMapMemory(vkDevice, vkMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(vkDevice, vkMemory);
}


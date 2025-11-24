#include "loading/loading_animation.h"
#include "shader/shader_loader.h"
#include "window/window.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdio.h>
#include <windows.h>

using namespace renderer::shader;

LoadingAnimation::LoadingAnimation() {
}

LoadingAnimation::~LoadingAnimation() {
    Cleanup();
}

bool LoadingAnimation::Initialize(VkDevice device, VkPhysicalDevice physicalDevice, VkCommandPool commandPool, 
                                  VkQueue graphicsQueue, VkRenderPass renderPass, VkExtent2D swapchainExtent) {
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
    
    if (m_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        m_graphicsPipeline = VK_NULL_HANDLE;
    }
    
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    
    // 清理所有方块的顶点缓冲区
    for (size_t i = 0; i < m_vertexBuffers.size(); i++) {
        if (m_vertexBuffers[i] != VK_NULL_HANDLE) {
            vkDestroyBuffer(m_device, m_vertexBuffers[i], nullptr);
            m_vertexBuffers[i] = VK_NULL_HANDLE;
        }
        if (m_vertexBufferMemories[i] != VK_NULL_HANDLE) {
            vkFreeMemory(m_device, m_vertexBufferMemories[i], nullptr);
            m_vertexBufferMemories[i] = VK_NULL_HANDLE;
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
        
        if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_vertexBuffers[i]) != VK_SUCCESS) {
            Window::ShowError("Failed to create vertex buffer for loading animation!");
            // 清理已创建的缓冲区
            for (int j = 0; j < i; j++) {
                vkDestroyBuffer(m_device, m_vertexBuffers[j], nullptr);
                vkFreeMemory(m_device, m_vertexBufferMemories[j], nullptr);
            }
            return false;
        }
        
        // 分配内存
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_device, m_vertexBuffers[i], &memRequirements);
        
        VkMemoryAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, 
                                                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
        if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_vertexBufferMemories[i]) != VK_SUCCESS) {
            Window::ShowError("Failed to allocate vertex buffer memory for loading animation!");
            // 清理已创建的缓冲区
            vkDestroyBuffer(m_device, m_vertexBuffers[i], nullptr);
            for (int j = 0; j < i; j++) {
                vkDestroyBuffer(m_device, m_vertexBuffers[j], nullptr);
                vkFreeMemory(m_device, m_vertexBufferMemories[j], nullptr);
            }
            return false;
        }
        
        vkBindBufferMemory(m_device, m_vertexBuffers[i], m_vertexBufferMemories[i], 0);
        
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
        vkMapMemory(m_device, m_vertexBufferMemories[i], 0, bufferSize, 0, &data);
        memcpy(data, vertices, (size_t)bufferSize);
        vkUnmapMemory(m_device, m_vertexBufferMemories[i]);
    }
    
    return true;
}

uint32_t LoadingAnimation::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    Window::ShowError("Failed to find suitable memory type for loading animation!");
    return 0;
}

bool LoadingAnimation::CreatePipeline(VkRenderPass renderPass) {
    // 加载shader
    std::vector<char> vertCode = ShaderLoader::LoadSPIRV("renderer/loading/loading.vert.spv");
    std::vector<char> fragCode = ShaderLoader::LoadSPIRV("renderer/loading/loading.frag.spv");
    
    if (vertCode.empty() || fragCode.empty()) {
        // 如果SPIR-V文件不存在，尝试从源码编译
        #ifdef USE_SHADERC
        std::ifstream vertFile("renderer/loading/loading.vert");
        std::ifstream fragFile("renderer/loading/loading.frag");
        if (vertFile.is_open() && fragFile.is_open()) {
            std::string vertSource((std::istreambuf_iterator<char>(vertFile)), std::istreambuf_iterator<char>());
            std::string fragSource((std::istreambuf_iterator<char>(fragFile)), std::istreambuf_iterator<char>());
            vertCode = ShaderLoader::CompileGLSLFromSource(vertSource, VK_SHADER_STAGE_VERTEX_BIT, "loading.vert");
            fragCode = ShaderLoader::CompileGLSLFromSource(fragSource, VK_SHADER_STAGE_FRAGMENT_BIT, "loading.frag");
        }
        #endif
    }
    
    if (vertCode.empty() || fragCode.empty()) {
        Window::ShowError("Failed to load shaders for loading animation!");
        return false;
    }
    
    VkShaderModule vertShaderModule = ShaderLoader::CreateShaderModuleFromSPIRV(m_device, vertCode);
    VkShaderModule fragShaderModule = ShaderLoader::CreateShaderModuleFromSPIRV(m_device, fragCode);
    
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        Window::ShowError("Failed to create shader modules for loading animation!");
        return false;
    }
    
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
    
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        Window::ShowError("Failed to create pipeline layout for loading animation!");
        return false;
    }
    
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
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    
    VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);
    
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create graphics pipeline for loading animation!");
        return false;
    }
    
    return true;
}

void LoadingAnimation::Render(VkCommandBuffer commandBuffer, VkExtent2D extent) {
    if (!m_initialized || m_graphicsPipeline == VK_NULL_HANDLE) return;
    
    // 绑定管线
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    
    // 渲染每个方块
    for (size_t i = 0; i < m_boxes.size(); i++) {
        const auto& box = m_boxes[i];
        
        // 绑定该方块的顶点缓冲区
        VkBuffer vertexBuffers[] = {m_vertexBuffers[i]};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        
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
        
        vkCmdPushConstants(commandBuffer, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 
                          0, sizeof(pushConstants), pushConstants);
        
        // 绘制方块（6个顶点）
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
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
    if (m_vertexBufferMemories[boxIndex] == VK_NULL_HANDLE) return;
    
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
    
    // 更新缓冲区数据
    void* data;
    vkMapMemory(m_device, m_vertexBufferMemories[boxIndex], 0, bufferSize, 0, &data);
    memcpy(data, vertices, (size_t)bufferSize);
    vkUnmapMemory(m_device, m_vertexBufferMemories[boxIndex]);
}


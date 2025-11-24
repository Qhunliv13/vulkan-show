#include "ui/slider/slider.h"
#include "ui/button/button.h"
#include "shader/shader_loader.h"
#include "window/window.h"
#include "vulkan/vulkan_renderer.h"  // 包含 StretchParams 定义
#include <algorithm>
#include <cmath>
#include <fstream>

using namespace renderer::shader;

Slider::Slider() {
}

Slider::~Slider() {
    Cleanup();
}

bool Slider::Initialize(VkDevice device, VkPhysicalDevice physicalDevice, 
                       VkCommandPool commandPool, VkQueue graphicsQueue, 
                       VkRenderPass renderPass, VkExtent2D swapchainExtent,
                       const SliderConfig& config,
                       bool usePureShader) {
    m_device = device;
    m_physicalDevice = physicalDevice;
    m_commandPool = commandPool;
    m_graphicsQueue = graphicsQueue;
    m_renderPass = renderPass;
    m_swapchainExtent = swapchainExtent;
    m_usePureShader = usePureShader;
    
    // 从配置中设置滑块属性
    m_width = config.width;
    m_height = config.height;
    m_trackColorR = config.trackColorR;
    m_trackColorG = config.trackColorG;
    m_trackColorB = config.trackColorB;
    m_trackColorA = config.trackColorA;
    m_fillColorR = config.fillColorR;
    m_fillColorG = config.fillColorG;
    m_fillColorB = config.fillColorB;
    m_fillColorA = config.fillColorA;
    m_thumbWidth = config.thumbWidth;
    m_thumbHeight = config.thumbHeight;
    m_useRelativePosition = config.useRelativePosition;
    m_relativeX = config.relativeX;
    m_relativeY = config.relativeY;
    m_screenWidth = (float)swapchainExtent.width;
    m_screenHeight = (float)swapchainExtent.height;
    m_minValue = config.minValue;
    m_maxValue = config.maxValue;
    m_value = config.defaultValue;
    m_zIndex = config.zIndex;
    
    // 限制初始值在范围内
    if (m_value < m_minValue) m_value = m_minValue;
    if (m_value > m_maxValue) m_value = m_maxValue;
    
    // 根据配置设置位置
    if (m_useRelativePosition) {
        UpdateRelativePosition();
    } else {
        m_x = config.x;
        m_y = config.y;
    }
    
    // 创建拖动点按钮（复用Button组件）
    m_thumbButton = std::make_unique<Button>();
    ButtonConfig thumbConfig;
    thumbConfig.width = m_thumbWidth;
    thumbConfig.height = m_thumbHeight;
    thumbConfig.colorR = config.thumbColorR;
    thumbConfig.colorG = config.thumbColorG;
    thumbConfig.colorB = config.thumbColorB;
    thumbConfig.colorA = config.thumbColorA;
    thumbConfig.texturePath = config.thumbTexturePath;
    thumbConfig.zIndex = m_zIndex + 1;  // 拖动点在轨道上方
    thumbConfig.useRelativePosition = false;
    thumbConfig.shapeType = 1;  // 设置为圆形按钮
    
    if (!m_thumbButton->Initialize(device, physicalDevice, commandPool, graphicsQueue, 
                                   renderPass, swapchainExtent, thumbConfig, nullptr, usePureShader)) {
        Window::ShowError("Failed to initialize slider thumb button!");
        return false;
    }
    
    // 更新拖动点位置
    UpdateThumbPosition();
    
    // 根据选择的渲染方式创建相应的资源
    if (m_usePureShader) {
        // 纯shader方式：创建全屏四边形和纯shader管线
        if (!CreateFullscreenQuadBuffer()) {
            return false;
        }
        if (!CreatePureShaderPipeline(renderPass)) {
            return false;
        }
    } else {
        // 传统方式：创建轨道和填充顶点缓冲区以及传统管线
        if (!CreateTrackBuffer()) {
            return false;
        }
        if (!CreateFillBuffer()) {
            return false;
        }
        if (!CreatePipeline(renderPass)) {
            return false;
        }
    }
    
    m_initialized = true;
    printf("[SLIDER] Slider initialized: pos=(%.2f, %.2f), size=(%.2f, %.2f), visible=%s, zIndex=%d\n",
           m_x, m_y, m_width, m_height, m_visible ? "true" : "false", m_zIndex);
    return true;
}

void Slider::Cleanup() {
    if (!m_initialized) return;
    
    // 清空回调函数，防止事件泄露
    m_onValueChangedCallback = nullptr;
    
    // 清理Scaled模式的拉伸参数（智能指针自动管理）
    m_stretchParams.reset();
    
    // 清理拖动点按钮
    if (m_thumbButton) {
        m_thumbButton->Cleanup();
        m_thumbButton.reset();
    }
    
    // 清理传统渲染资源
    if (m_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        m_graphicsPipeline = VK_NULL_HANDLE;
    }
    
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    
    if (m_trackVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_trackVertexBuffer, nullptr);
        m_trackVertexBuffer = VK_NULL_HANDLE;
    }
    
    if (m_trackVertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_trackVertexBufferMemory, nullptr);
        m_trackVertexBufferMemory = VK_NULL_HANDLE;
    }
    
    if (m_fillVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_fillVertexBuffer, nullptr);
        m_fillVertexBuffer = VK_NULL_HANDLE;
    }
    
    if (m_fillVertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_fillVertexBufferMemory, nullptr);
        m_fillVertexBufferMemory = VK_NULL_HANDLE;
    }
    
    // 清理纯shader渲染资源
    if (m_pureShaderPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_pureShaderPipeline, nullptr);
        m_pureShaderPipeline = VK_NULL_HANDLE;
    }
    
    if (m_pureShaderPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pureShaderPipelineLayout, nullptr);
        m_pureShaderPipelineLayout = VK_NULL_HANDLE;
    }
    
    if (m_fullscreenQuadBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_device, m_fullscreenQuadBuffer, nullptr);
        m_fullscreenQuadBuffer = VK_NULL_HANDLE;
    }
    
    if (m_fullscreenQuadBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_fullscreenQuadBufferMemory, nullptr);
        m_fullscreenQuadBufferMemory = VK_NULL_HANDLE;
    }
    
    m_initialized = false;
}

bool Slider::CreateTrackBuffer() {
    // 创建轨道顶点缓冲区
    struct Vertex {
        float x, y;
        float r, g, b, a;
    };
    
    Vertex trackVertices[] = {
        // 第一个三角形
        {0.0f, 0.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},  // 左上
        {1.0f, 0.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},  // 右上
        {1.0f, 1.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},  // 右下
        // 第二个三角形
        {0.0f, 0.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},  // 左上
        {1.0f, 1.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},  // 右下
        {0.0f, 1.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},  // 左下
    };
    
    VkDeviceSize bufferSize = sizeof(trackVertices);
    
    // 创建缓冲区
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_trackVertexBuffer) != VK_SUCCESS) {
        Window::ShowError("Failed to create slider track vertex buffer!");
        return false;
    }
    
    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_trackVertexBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, 
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_trackVertexBufferMemory) != VK_SUCCESS) {
        Window::ShowError("Failed to allocate slider track vertex buffer memory!");
        return false;
    }
    
    vkBindBufferMemory(m_device, m_trackVertexBuffer, m_trackVertexBufferMemory, 0);
    
    // 填充数据
    void* data;
    vkMapMemory(m_device, m_trackVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, trackVertices, (size_t)bufferSize);
    vkUnmapMemory(m_device, m_trackVertexBufferMemory);
    
    return true;
}

bool Slider::CreateFillBuffer() {
    // 创建填充区域顶点缓冲区
    // 注意：填充区域的顶点缓冲区应该始终使用完整的归一化宽度（1.0）
    // 因为 push constants 中的 size.x 已经是 fillWidth（实际像素宽度）
    // shader 会将顶点坐标 inPosition.x（1.0）乘以 fillWidth，得到实际的像素宽度
    struct Vertex {
        float x, y;
        float r, g, b, a;
    };
    
    // 填充区域使用完整的归一化宽度（1.0），实际宽度由 push constants 中的 size.x 控制
    Vertex fillVertices[] = {
        // 第一个三角形
        {0.0f, 0.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 左上
        {1.0f, 0.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 右上
        {1.0f, 1.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 右下
        // 第二个三角形
        {0.0f, 0.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 左上
        {1.0f, 1.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 右下
        {0.0f, 1.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 左下
    };
    
    VkDeviceSize bufferSize = sizeof(fillVertices);
    
    // 创建缓冲区
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_fillVertexBuffer) != VK_SUCCESS) {
        Window::ShowError("Failed to create slider fill vertex buffer!");
        return false;
    }
    
    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_fillVertexBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, 
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_fillVertexBufferMemory) != VK_SUCCESS) {
        Window::ShowError("Failed to allocate slider fill vertex buffer memory!");
        return false;
    }
    
    vkBindBufferMemory(m_device, m_fillVertexBuffer, m_fillVertexBufferMemory, 0);
    
    // 填充数据
    void* data;
    vkMapMemory(m_device, m_fillVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, fillVertices, (size_t)bufferSize);
    vkUnmapMemory(m_device, m_fillVertexBufferMemory);
    
    return true;
}

void Slider::UpdateTrackBuffer() {
    if (!m_initialized || m_trackVertexBufferMemory == VK_NULL_HANDLE) return;
    
    struct Vertex {
        float x, y;
        float r, g, b, a;
    };
    
    Vertex trackVertices[] = {
        {0.0f, 0.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},
        {1.0f, 0.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},
        {1.0f, 1.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},
        {0.0f, 0.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},
        {1.0f, 1.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},
        {0.0f, 1.0f, m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA},
    };
    
    VkDeviceSize bufferSize = sizeof(trackVertices);
    void* data;
    vkMapMemory(m_device, m_trackVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, trackVertices, (size_t)bufferSize);
    vkUnmapMemory(m_device, m_trackVertexBufferMemory);
}

void Slider::UpdateFillBuffer() {
    if (!m_initialized || m_fillVertexBufferMemory == VK_NULL_HANDLE) return;
    
    struct Vertex {
        float x, y;
        float r, g, b, a;
    };
    
    // 填充区域的顶点缓冲区应该始终使用完整的归一化宽度（1.0）
    // 因为 push constants 中的 size.x 已经是 fillWidth（实际像素宽度）
    // shader 会将顶点坐标 inPosition.x（1.0）乘以 fillWidth，得到实际的像素宽度
    // 这样填充区域就会从 renderX 开始，宽度为 fillWidth
    Vertex fillVertices[] = {
        {0.0f, 0.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 左上
        {1.0f, 0.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 右上
        {1.0f, 1.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 右下
        {0.0f, 0.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 左上
        {1.0f, 1.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 右下
        {0.0f, 1.0f, m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA},  // 左下
    };
    
    VkDeviceSize bufferSize = sizeof(fillVertices);
    void* data;
    vkMapMemory(m_device, m_fillVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, fillVertices, (size_t)bufferSize);
    vkUnmapMemory(m_device, m_fillVertexBufferMemory);
}

uint32_t Slider::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    Window::ShowError("Failed to find suitable memory type for slider!");
    return 0;
}

bool Slider::CreatePipeline(VkRenderPass renderPass) {
    // 加载shader（复用按钮的shader）
    std::vector<char> vertCode = ShaderLoader::LoadSPIRV("renderer/ui/button/button.vert.spv");
    std::vector<char> fragCode = ShaderLoader::LoadSPIRV("renderer/ui/button/button.frag.spv");
    
    if (vertCode.empty() || fragCode.empty()) {
        // 如果SPIR-V文件不存在，尝试从源码编译
        #ifdef USE_SHADERC
        std::ifstream vertFile("renderer/ui/button/button.vert");
        std::ifstream fragFile("renderer/ui/button/button.frag");
        if (vertFile.is_open() && fragFile.is_open()) {
            std::string vertSource((std::istreambuf_iterator<char>(vertFile)), std::istreambuf_iterator<char>());
            std::string fragSource((std::istreambuf_iterator<char>(fragFile)), std::istreambuf_iterator<char>());
            vertCode = ShaderLoader::CompileGLSLFromSource(vertSource, VK_SHADER_STAGE_VERTEX_BIT, "button.vert");
            fragCode = ShaderLoader::CompileGLSLFromSource(fragSource, VK_SHADER_STAGE_FRAGMENT_BIT, "button.frag");
        }
        #endif
    }
    
    if (vertCode.empty() || fragCode.empty()) {
        Window::ShowError("Failed to load shaders for slider!");
        return false;
    }
    
    VkShaderModule vertShaderModule = ShaderLoader::CreateShaderModuleFromSPIRV(m_device, vertCode);
    VkShaderModule fragShaderModule = ShaderLoader::CreateShaderModuleFromSPIRV(m_device, fragCode);
    
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        Window::ShowError("Failed to create shader modules for slider!");
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
    
    // 视口（使用动态状态）
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
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // Dynamic states
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    
    // 深度测试状态
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    // Push constants: position(2) + size(2) + screenSize(2) + useTexture(1) = 7 floats
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 8; // position(2) + size(2) + screenSize(2) + useTexture(1) + shapeType(1)
    
    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        Window::ShowError("Failed to create pipeline layout for slider!");
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
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    
    VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);
    
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create graphics pipeline for slider!");
        return false;
    }
    
    return true;
}

bool Slider::CreateFullscreenQuadBuffer() {
    // 创建全屏四边形的顶点缓冲区（用于纯shader渲染）
    struct Vertex {
        float x, y;
    };
    
    Vertex quadVertices[] = {
        {0.0f, 0.0f},  // 左上
        {1.0f, 0.0f},  // 右上
        {1.0f, 1.0f},  // 右下
        {0.0f, 0.0f},  // 左上
        {1.0f, 1.0f},  // 右下
        {0.0f, 1.0f},  // 左下
    };
    
    VkDeviceSize bufferSize = sizeof(quadVertices);
    
    // 创建缓冲区
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    if (vkCreateBuffer(m_device, &bufferInfo, nullptr, &m_fullscreenQuadBuffer) != VK_SUCCESS) {
        Window::ShowError("Failed to create fullscreen quad vertex buffer!");
        return false;
    }
    
    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, m_fullscreenQuadBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, 
                                                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &m_fullscreenQuadBufferMemory) != VK_SUCCESS) {
        Window::ShowError("Failed to allocate fullscreen quad vertex buffer memory!");
        return false;
    }
    
    vkBindBufferMemory(m_device, m_fullscreenQuadBuffer, m_fullscreenQuadBufferMemory, 0);
    
    // 填充数据
    void* data;
    vkMapMemory(m_device, m_fullscreenQuadBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, quadVertices, (size_t)bufferSize);
    vkUnmapMemory(m_device, m_fullscreenQuadBufferMemory);
    
    return true;
}

bool Slider::CreatePureShaderPipeline(VkRenderPass renderPass) {
    // 加载纯shader（复用按钮的纯shader）
    std::vector<char> vertCode = ShaderLoader::LoadSPIRV("renderer/ui/button/button_pure.vert.spv");
    std::vector<char> fragCode = ShaderLoader::LoadSPIRV("renderer/ui/button/button_pure.frag.spv");
    
    if (vertCode.empty() || fragCode.empty()) {
        #ifdef USE_SHADERC
        std::ifstream vertFile("renderer/ui/button/button_pure.vert");
        std::ifstream fragFile("renderer/ui/button/button_pure.frag");
        if (vertFile.is_open() && fragFile.is_open()) {
            std::string vertSource((std::istreambuf_iterator<char>(vertFile)), std::istreambuf_iterator<char>());
            std::string fragSource((std::istreambuf_iterator<char>(fragFile)), std::istreambuf_iterator<char>());
            vertCode = ShaderLoader::CompileGLSLFromSource(vertSource, VK_SHADER_STAGE_VERTEX_BIT, "button_pure.vert");
            fragCode = ShaderLoader::CompileGLSLFromSource(fragSource, VK_SHADER_STAGE_FRAGMENT_BIT, "button_pure.frag");
        }
        #endif
    }
    
    if (vertCode.empty() || fragCode.empty()) {
        Window::ShowError("Failed to load pure shader shaders for slider!");
        return false;
    }
    
    VkShaderModule vertShaderModule = ShaderLoader::CreateShaderModuleFromSPIRV(m_device, vertCode);
    VkShaderModule fragShaderModule = ShaderLoader::CreateShaderModuleFromSPIRV(m_device, fragCode);
    
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        Window::ShowError("Failed to create pure shader modules for slider!");
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
    bindingDescription.stride = sizeof(float) * 2; // x, y
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    VkVertexInputAttributeDescription attributeDescription = {};
    attributeDescription.binding = 0;
    attributeDescription.location = 0;
    attributeDescription.format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescription.offset = 0;
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.vertexAttributeDescriptionCount = 1;
    vertexInputInfo.pVertexAttributeDescriptions = &attributeDescription;
    
    // 输入装配
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // 视口（使用动态状态）
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
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // Dynamic states
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    
    // 深度测试状态
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    // Push constants: position(2) + size(2) + screenSize(2) + color(4) = 10 floats
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 10;
    
    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pureShaderPipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
        vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
        Window::ShowError("Failed to create pure shader pipeline layout for slider!");
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
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pureShaderPipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    
    VkResult result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pureShaderPipeline);
    
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create pure shader graphics pipeline for slider!");
        return false;
    }
    
    return true;
}

void Slider::SetPosition(float x, float y) {
    m_x = x;
    m_y = y;
    m_useRelativePosition = false;
    UpdateThumbPosition();
}

void Slider::SetSize(float width, float height) {
    m_width = width;
    m_height = height;
    if (m_useRelativePosition) {
        UpdateRelativePosition();
    }
    UpdateThumbPosition();
}

void Slider::UpdateRelativePosition() {
    if (m_useRelativePosition) {
        // Scaled模式：使用逻辑坐标计算位置
        if (m_stretchParams) {
            m_x = m_relativeX * m_stretchParams->logicalWidth - m_width / 2.0f;
            m_y = m_relativeY * m_stretchParams->logicalHeight - m_height / 2.0f;
        } else if (m_screenWidth > 0.0f && m_screenHeight > 0.0f) {
            // 其他模式：使用屏幕坐标计算位置
            m_x = m_relativeX * m_screenWidth - m_width / 2.0f;
            m_y = m_relativeY * m_screenHeight - m_height / 2.0f;
        }
        UpdateThumbPosition();
    }
}

void Slider::UpdateThumbPosition() {
    if (!m_thumbButton) return;
    
    // 计算拖动点的X位置（根据归一化值）
    float normalizedValue = GetNormalizedValue();
    // 拖动点中心应该在归一化值对应的位置
    // 拖动点可以超出轨道范围，但中心对齐到值的位置
    float thumbCenterX = m_x + normalizedValue * m_width;
    float thumbCenterY = m_y + m_height / 2.0f;
    
    // 设置拖动点按钮的位置（按钮的左上角）
    m_thumbX = thumbCenterX - m_thumbWidth / 2.0f;
    m_thumbY = thumbCenterY - m_thumbHeight / 2.0f;
    
    m_thumbButton->SetPosition(m_thumbX, m_thumbY);
    
    // 调试输出
    static int thumbDebugCount = 0;
    if (thumbDebugCount % 60 == 0) {
        printf("[SLIDER] UpdateThumbPosition: sliderPos=(%.2f, %.2f), normalizedValue=%.2f, thumbPos=(%.2f, %.2f)\n",
               m_x, m_y, normalizedValue, m_thumbX, m_thumbY);
    }
    thumbDebugCount++;
}

void Slider::SetValue(float value) {
    float oldValue = m_value;
    m_value = (std::max)(m_minValue, (std::min)(m_maxValue, value));
    
    if (m_value != oldValue) {
        UpdateThumbPosition();
        UpdateFillBuffer();
        
        if (m_onValueChangedCallback) {
            m_onValueChangedCallback(m_value);
        }
    }
}

void Slider::SetThumbColor(float r, float g, float b, float a) {
    if (m_thumbButton) {
        m_thumbButton->SetColor(r, g, b, a);
    }
}

void Slider::SetThumbTexture(const std::string& texturePath) {
    if (m_thumbButton) {
        m_thumbButton->SetTexture(texturePath);
    }
}

void Slider::SetZIndex(int zIndex) {
    m_zIndex = zIndex;
    if (m_thumbButton) {
        m_thumbButton->SetZIndex(zIndex + 1);
    }
}

void Slider::SetVisible(bool visible) {
    m_visible = visible;
    if (m_thumbButton) {
        m_thumbButton->SetVisible(visible);
    }
}

bool Slider::IsPointInsideTrack(float px, float py) const {
    float checkX = px;
    float checkY = py;
    
    // Scaled模式：将屏幕坐标转换为逻辑坐标
    if (m_stretchParams) {
        checkX = (px - m_stretchParams->marginX) / m_stretchParams->stretchScaleX;
        checkY = (py - m_stretchParams->marginY) / m_stretchParams->stretchScaleY;
    }
    
    // 检查是否在轨道的矩形区域内（逻辑坐标）
    return checkX >= m_x && checkX <= m_x + m_width && 
           checkY >= m_y && checkY <= m_y + m_height;
}

bool Slider::IsPointInsideThumb(float px, float py) const {
    if (!m_thumbButton) return false;
    return m_thumbButton->IsPointInside(px, py);
}

void Slider::SetValueFromPosition(float px, float py) {
    float checkX = px;
    
    // Scaled模式：将屏幕坐标转换为逻辑坐标
    if (m_stretchParams) {
        checkX = (px - m_stretchParams->marginX) / m_stretchParams->stretchScaleX;
    }
    
    // 计算相对于轨道的位置（0.0-1.0）
    float relativeX = (checkX - m_x) / m_width;
    relativeX = (std::max)(0.0f, (std::min)(1.0f, relativeX));
    
    // 转换为实际值
    float newValue = m_minValue + relativeX * (m_maxValue - m_minValue);
    SetValue(newValue);
}

bool Slider::HandleMouseDown(float clickX, float clickY) {
    if (!m_visible) return false;
    
    // 先检查是否点击了拖动点
    if (IsPointInsideThumb(clickX, clickY)) {
        m_isDragging = true;
        return true;
    }
    
    // 检查是否点击了轨道
    if (IsPointInsideTrack(clickX, clickY)) {
        SetValueFromPosition(clickX, clickY);
        m_isDragging = true;
        return true;
    }
    
    return false;
}

bool Slider::HandleMouseMove(float mouseX, float mouseY) {
    if (!m_isDragging) return false;
    
    SetValueFromPosition(mouseX, mouseY);
    return true;
}

void Slider::HandleMouseUp() {
    m_isDragging = false;
}

void Slider::SetStretchParams(const struct StretchParams& params) {
    // 为StretchParams分配内存（如果还没有分配）
    if (!m_stretchParams) {
        m_stretchParams = std::make_unique<StretchParams>();
    }
    *m_stretchParams = params;
    
    // 如果使用相对位置，需要重新计算位置
    if (m_useRelativePosition) {
        UpdateRelativePosition();
    }
    
    // 更新拖动点按钮的拉伸参数
    if (m_thumbButton) {
        m_thumbButton->SetStretchParams(params);
    }
}

void Slider::Render(VkCommandBuffer commandBuffer, VkExtent2D extent) {
    // 如果滑块不可见，不渲染
    if (!m_visible) {
        static int debugCount = 0;
        if (debugCount % 60 == 0) {
            printf("[SLIDER RENDER] Slider is not visible\n");
        }
        debugCount++;
        return;
    }
    
    if (!m_initialized) {
        printf("[SLIDER RENDER] Slider is not initialized\n");
        return;
    }
    
    // 调试输出
    static int renderDebugCount = 0;
    if (renderDebugCount % 60 == 0) {
        printf("[SLIDER RENDER] Rendering slider: pos=(%.2f, %.2f), size=(%.2f, %.2f), extent=(%u, %u), value=%.2f\n",
               m_x, m_y, m_width, m_height, extent.width, extent.height, m_value);
    }
    renderDebugCount++;
    
    if (m_usePureShader) {
        // 纯shader方式渲染
        if (m_pureShaderPipeline == VK_NULL_HANDLE || m_fullscreenQuadBuffer == VK_NULL_HANDLE) return;
        
        // 绑定管线
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pureShaderPipeline);
        
        // 绑定全屏四边形顶点缓冲区
        VkBuffer vertexBuffers[] = {m_fullscreenQuadBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        
        // 设置视口和裁剪区域
        VkViewport viewport = {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)extent.width;
        viewport.height = (float)extent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        
        VkRect2D scissor = {};
        scissor.offset = {0, 0};
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        
        // 渲染轨道
        float renderX = m_x;
        float renderY = m_y;
        float renderWidth = m_width;
        float renderHeight = m_height;
        float renderScreenWidth = (float)extent.width;
        float renderScreenHeight = (float)extent.height;
        
        if (m_stretchParams) {
            renderX = m_x * m_stretchParams->stretchScaleX + m_stretchParams->marginX;
            renderY = m_y * m_stretchParams->stretchScaleY + m_stretchParams->marginY;
            renderWidth = m_width * m_stretchParams->stretchScaleX;
            renderHeight = m_height * m_stretchParams->stretchScaleY;
            renderScreenWidth = m_stretchParams->screenWidth;
            renderScreenHeight = m_stretchParams->screenHeight;
        }
        
        // 计算翻转的Y坐标
        float flippedY = renderScreenHeight - renderY - renderHeight;
        
        // Push constants for track
        float pushConstants[10] = {
            renderX, flippedY, renderWidth, renderHeight,
            renderScreenWidth, renderScreenHeight,
            m_trackColorR, m_trackColorG, m_trackColorB, m_trackColorA
        };
        vkCmdPushConstants(commandBuffer, m_pureShaderPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 
                          0, sizeof(pushConstants), pushConstants);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
        
        // 渲染填充区域
        float fillWidth = GetNormalizedValue() * renderWidth;
        float fillPushConstants[10] = {
            renderX, flippedY, fillWidth, renderHeight,
            renderScreenWidth, renderScreenHeight,
            m_fillColorR, m_fillColorG, m_fillColorB, m_fillColorA
        };
        vkCmdPushConstants(commandBuffer, m_pureShaderPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 
                          0, sizeof(fillPushConstants), fillPushConstants);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    } else {
        // 传统方式渲染
        if (m_graphicsPipeline == VK_NULL_HANDLE || 
            m_trackVertexBuffer == VK_NULL_HANDLE || 
            m_fillVertexBuffer == VK_NULL_HANDLE) return;
        
        // 绑定管线
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
        
        // Scaled模式：将逻辑坐标转换为屏幕坐标
        float renderX = m_x;
        float renderY = m_y;
        float renderWidth = m_width;
        float renderHeight = m_height;
        float renderScreenWidth = (float)extent.width;
        float renderScreenHeight = (float)extent.height;
        
        if (m_stretchParams) {
            renderX = m_x * m_stretchParams->stretchScaleX + m_stretchParams->marginX;
            renderY = m_y * m_stretchParams->stretchScaleY + m_stretchParams->marginY;
            renderWidth = m_width * m_stretchParams->stretchScaleX;
            renderHeight = m_height * m_stretchParams->stretchScaleY;
            renderScreenWidth = m_stretchParams->screenWidth;
            renderScreenHeight = m_stretchParams->screenHeight;
        }
        
        // 计算翻转的Y坐标
        float flippedY = renderScreenHeight - renderY - renderHeight;
        
        // 调试输出
        if (renderDebugCount % 60 == 0) {
            printf("[SLIDER RENDER] Traditional mode: renderX=%.2f, renderY=%.2f, flippedY=%.2f, renderWidth=%.2f, renderHeight=%.2f, screenSize=(%.2f, %.2f)\n",
                   renderX, renderY, flippedY, renderWidth, renderHeight, renderScreenWidth, renderScreenHeight);
        }
        
        // 注意：不设置 viewport 和 scissor，使用调用者已经设置的 viewport/scissor
        // 这样在 Fit 模式下可以正确工作
        
        // Push constants: position, size, screenSize, useTexture, shapeType
        float useTexture = 0.0f;  // 滑块不使用纹理
        float pushConstants[8] = {
            renderX, flippedY, renderWidth, renderHeight,
            renderScreenWidth, renderScreenHeight, useTexture,
            0.0f  // shapeType = 0 (矩形，轨道和填充区域都应该是矩形)
        };
        vkCmdPushConstants(commandBuffer, m_pipelineLayout, 
                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                          0, sizeof(pushConstants), pushConstants);
        
        // 渲染轨道
        VkBuffer trackBuffers[] = {m_trackVertexBuffer};
        VkDeviceSize trackOffsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, trackBuffers, trackOffsets);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
        
        // 渲染填充区域
        // 计算填充区域的宽度（根据归一化值）
        float normalizedValue = GetNormalizedValue();
        float fillWidth = normalizedValue * renderWidth;
        
        // 注意：填充区域的顶点缓冲区使用归一化坐标（0-1）
        // push constants 中的 size.x 应该是 fillWidth（实际像素宽度）
        // 这样 shader 会将顶点坐标 inPosition.x（归一化值）乘以 fillWidth，得到实际的像素宽度
        // 但是填充区域的顶点缓冲区中，X 坐标应该始终是 1.0（完整的归一化宽度）
        // 因为 push constants 中的 size.x 已经是 fillWidth，所以 1.0 * fillWidth = fillWidth
        
        // 使用填充颜色更新push constants
        // size.x = fillWidth（实际像素宽度），size.y = renderHeight
        float fillPushConstants[8] = {
            renderX, flippedY, fillWidth, renderHeight,
            renderScreenWidth, renderScreenHeight, useTexture,
            0.0f  // shapeType = 0 (矩形，填充区域应该是矩形)
        };
        vkCmdPushConstants(commandBuffer, m_pipelineLayout, 
                          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                          0, sizeof(fillPushConstants), fillPushConstants);
        
        // 绑定填充区域顶点缓冲区并绘制
        // 注意：填充区域的顶点缓冲区应该使用完整的归一化宽度（1.0），而不是 normalizedValue
        // 因为 push constants 中的 size.x 已经是 fillWidth
        VkBuffer fillBuffers[] = {m_fillVertexBuffer};
        VkDeviceSize fillOffsets[] = {0};
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, fillBuffers, fillOffsets);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    }
    
    // 渲染拖动点按钮（在所有模式下都需要）
    if (m_thumbButton) {
        m_thumbButton->Render(commandBuffer, extent);
    }
}

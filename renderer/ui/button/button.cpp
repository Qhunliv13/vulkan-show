#include "ui/button/button.h"  // 1. 对应头文件

#include <algorithm>           // 2. 系统头文件
#include <cmath>               // 2. 系统头文件
#include <fstream>             // 2. 系统头文件
#include <stdio.h>             // 2. 系统头文件

#include <vulkan/vulkan.h>     // 3. 第三方库头文件

/**
 * 注意：window.h已经包含了windows.h，所以LoadImage宏可能已经被定义
 * 在包含image_loader.h之前取消宏定义，避免与ImageLoader::LoadImage冲突
 * 
 * 原因：Windows API 定义了 LoadImage 宏，会与 ImageLoader::LoadImage 方法名冲突
 */
#ifdef LoadImage
#undef LoadImage
#endif
#include "core/interfaces/irender_context.h"  // 4. 项目头文件（接口）
#include "core/config/stretch_params.h"                    // 4. 项目头文件
#include "renderer/vulkan/vulkan_render_context_factory.h"  // 4. 项目头文件（工厂函数）
#include "core/types/render_types.h"                       // 4. 项目头文件
#include "image/image_loader.h"                            // 4. 项目头文件
#include "shader/shader_loader.h"                          // 4. 项目头文件
#include "text/text_renderer.h"                            // 4. 项目头文件
#include "texture/texture.h"                               // 4. 项目头文件
#include "window/window.h"                                 // 4. 项目头文件

Button::Button() {
}

Button::~Button() {
    Cleanup();
}

bool Button::Initialize(IRenderContext* renderContext,
                       const ButtonConfig& config,
                       TextRenderer* textRenderer,
                       bool usePureShader) {
    if (!renderContext) {
        return false;
    }
    
    m_renderContext = renderContext;
    // 将抽象句柄转换为 Vulkan 类型（存储为抽象类型，在需要时转换）
    m_device = renderContext->GetDevice();
    m_physicalDevice = renderContext->GetPhysicalDevice();
    m_commandPool = renderContext->GetCommandPool();
    m_graphicsQueue = renderContext->GetGraphicsQueue();
    m_renderPass = renderContext->GetRenderPass();
    m_swapchainExtent = renderContext->GetSwapchainExtent();
    m_usePureShader = usePureShader;
    
    // 从配置中设置按钮属性
    m_width = config.width;
    m_height = config.height;
    m_colorR = config.colorR;
    m_colorG = config.colorG;
    m_colorB = config.colorB;
    m_colorA = config.colorA;
    m_texturePath = config.texturePath;
    m_useRelativePosition = config.useRelativePosition;
    m_relativeX = config.relativeX;
    m_relativeY = config.relativeY;
    m_screenWidth = (float)m_swapchainExtent.width;
    m_screenHeight = (float)m_swapchainExtent.height;
    m_shapeType = config.shapeType;
    
    // 文本相关配置
    m_enableText = config.enableText;
    m_text = config.text;
    m_textColorR = config.textColorR;
    m_textColorG = config.textColorG;
    m_textColorB = config.textColorB;
    m_textColorA = config.textColorA;
    m_textRenderer = textRenderer;
    
    // 渲染层级配置
    m_zIndex = config.zIndex;
    
    // 悬停效果配置
    m_enableHoverEffect = config.enableHoverEffect;
    m_hoverEffectType = config.hoverEffectType;
    m_hoverEffectStrength = config.hoverEffectStrength;
    m_isHovering = false;
    
    // 如果启用了文本但没有提供TextRenderer，禁用文本
    if (m_enableText && !m_textRenderer) {
        m_enableText = false;
    }
    
    // 根据配置设置位置
    if (m_useRelativePosition) {
        UpdateRelativePosition();
    } else {
        m_x = config.x;
        m_y = config.y;
    }
    
    // 如果设置了纹理路径，加载纹理图像数据（仅用于点击判定）
    // 注意：纯shader方案目前只支持颜色渲染，纹理仅用于点击判定
    if (!config.texturePath.empty()) {
        m_texturePath = config.texturePath;
        
        // 加载纹理图像数据（用于点击判定）
        auto imageData = renderer::image::ImageLoader::LoadImage(config.texturePath);
        if (imageData.width > 0 && imageData.height > 0) {
            m_textureData.pixels = imageData.pixels;
            m_textureData.width = imageData.width;
            m_textureData.height = imageData.height;
            m_useTextureHitTest = true;  // 启用基于纹理的点击判定
            
            // 根据纹理宽高比调整按钮大小，保持纹理原始比例，避免拉伸
            float textureAspect = (float)imageData.width / (float)imageData.height;
            float buttonAspect = m_width / m_height;
            
            // 如果宽高比不匹配，调整按钮大小以保持纹理宽高比
            if (std::abs(textureAspect - buttonAspect) > 0.01f) {
                // 保持宽度不变，调整高度以匹配纹理宽高比
                m_height = m_width / textureAspect;
                printf("[BUTTON] Adjusted button size to match texture aspect ratio: %.2fx%.2f (texture: %dx%d, aspect: %.3f)\n",
                       m_width, m_height, imageData.width, imageData.height, textureAspect);
            }
        }
        
        // 纯shader方案不使用Vulkan纹理渲染，只使用颜色
        // 传统方案才需要加载Vulkan纹理资源
        if (!m_usePureShader) {
            printf("[BUTTON] Initializing with texture: %s (usePureShader=false)\n", config.texturePath.c_str());
            m_useTexture = true;
            if (!LoadTexture(config.texturePath)) {
                printf("[BUTTON] ERROR: Failed to load texture during initialization\n");
                return false;
            }
            printf("[BUTTON] Texture loaded successfully during initialization\n");
        } else {
            printf("[BUTTON] Texture path provided but usePureShader=true, skipping Vulkan texture load\n");
        }
    }
    
    // 如果使用纹理，先创建描述符集布局（传统渲染方式需要）
    if (m_useTexture && !m_usePureShader) {
        printf("[BUTTON] Creating descriptor set layout for texture (useTexture=true, usePureShader=false)\n");
        if (!CreateDescriptorSetLayout()) {
            printf("[BUTTON] ERROR: Failed to create descriptor set layout during initialization\n");
            return false;
        }
    } else {
        printf("[BUTTON] Skipping descriptor set layout creation (useTexture=%s, usePureShader=%s)\n",
               m_useTexture ? "true" : "false", m_usePureShader ? "true" : "false");
    }
    
    // 根据选择的渲染方式创建相应的资源
    if (m_usePureShader) {
        // 纯shader方式：创建全屏四边形和纯shader管线
        if (!CreateFullscreenQuadBuffer()) {
            return false;
        }
        if (!CreatePureShaderPipeline(m_renderPass)) {
            return false;
        }
    } else {
        // 传统方式：创建按钮顶点缓冲区和传统管线
        if (!CreateButtonBuffer()) {
            return false;
        }
        if (!CreatePipeline(m_renderPass)) {
            return false;
        }
    }
    
    m_initialized = true;
    return true;
}

void Button::Cleanup() {
    if (!m_initialized) return;
    
    // 将抽象类型转换为 Vulkan 类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    
    // 清空回调函数，防止事件泄露
    m_onClickCallback = nullptr;
    
    // 清理Scaled模式的拉伸参数（智能指针自动管理）
    m_stretchParams.reset();
    
    // 清理纹理资源（所有渲染方式都需要清理）
    CleanupTexture();
    
    // 清理描述符相关资源
    VkDescriptorPool vkDescriptorPool = static_cast<VkDescriptorPool>(m_descriptorPool);
    if (vkDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
        m_descriptorPool = nullptr;
    }
    
    VkDescriptorSetLayout vkDescriptorSetLayout = static_cast<VkDescriptorSetLayout>(m_descriptorSetLayout);
    if (vkDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(vkDevice, vkDescriptorSetLayout, nullptr);
        m_descriptorSetLayout = nullptr;
    }
    
    // 清理传统渲染资源
    VkPipeline vkGraphicsPipeline = static_cast<VkPipeline>(m_graphicsPipeline);
    if (vkGraphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(vkDevice, vkGraphicsPipeline, nullptr);
        m_graphicsPipeline = nullptr;
    }
    
    VkPipelineLayout vkPipelineLayout = static_cast<VkPipelineLayout>(m_pipelineLayout);
    if (vkPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(vkDevice, vkPipelineLayout, nullptr);
        m_pipelineLayout = nullptr;
    }
    
    VkBuffer vkVertexBuffer = static_cast<VkBuffer>(m_vertexBuffer);
    if (vkVertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(vkDevice, vkVertexBuffer, nullptr);
        m_vertexBuffer = nullptr;
    }
    
    VkDeviceMemory vkVertexBufferMemory = static_cast<VkDeviceMemory>(m_vertexBufferMemory);
    if (vkVertexBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(vkDevice, vkVertexBufferMemory, nullptr);
        m_vertexBufferMemory = nullptr;
    }
    
    // 清理纯shader渲染资源
    VkPipeline vkPureShaderPipeline = static_cast<VkPipeline>(m_pureShaderPipeline);
    if (vkPureShaderPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(vkDevice, vkPureShaderPipeline, nullptr);
        m_pureShaderPipeline = nullptr;
    }
    
    VkPipelineLayout vkPureShaderPipelineLayout = static_cast<VkPipelineLayout>(m_pureShaderPipelineLayout);
    if (vkPureShaderPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(vkDevice, vkPureShaderPipelineLayout, nullptr);
        m_pureShaderPipelineLayout = nullptr;
    }
    
    VkBuffer vkFullscreenQuadBuffer = static_cast<VkBuffer>(m_fullscreenQuadBuffer);
    if (vkFullscreenQuadBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(vkDevice, vkFullscreenQuadBuffer, nullptr);
        m_fullscreenQuadBuffer = nullptr;
    }
    
    VkDeviceMemory vkFullscreenQuadBufferMemory = static_cast<VkDeviceMemory>(m_fullscreenQuadBufferMemory);
    if (vkFullscreenQuadBufferMemory != VK_NULL_HANDLE) {
        vkFreeMemory(vkDevice, vkFullscreenQuadBufferMemory, nullptr);
        m_fullscreenQuadBufferMemory = nullptr;
    }
    
    m_initialized = false;
}

bool Button::CreateButtonBuffer() {
    // 将抽象类型转换为 Vulkan 类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    
    // 创建按钮专用的顶点缓冲区
    struct Vertex {
        float x, y;
        float r, g, b, a;
    };
    
    Vertex buttonVertices[] = {
        // 第一个三角形
        {0.0f, 0.0f, m_colorR, m_colorG, m_colorB, m_colorA},  // 左上
        {1.0f, 0.0f, m_colorR, m_colorG, m_colorB, m_colorA},  // 右上
        {1.0f, 1.0f, m_colorR, m_colorG, m_colorB, m_colorA},  // 右下
        // 第二个三角形
        {0.0f, 0.0f, m_colorR, m_colorG, m_colorB, m_colorA},  // 左上
        {1.0f, 1.0f, m_colorR, m_colorG, m_colorB, m_colorA},  // 右下
        {0.0f, 1.0f, m_colorR, m_colorG, m_colorB, m_colorA},  // 左下
    };
    
    VkDeviceSize bufferSize = sizeof(buttonVertices);
    
    // 创建缓冲区
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkBuffer vkVertexBuffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &vkVertexBuffer) != VK_SUCCESS) {
        Window::ShowError("Failed to create button vertex buffer!");
        return false;
    }
    m_vertexBuffer = vkVertexBuffer;
    
    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkDevice, vkVertexBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, 
                                                MemoryPropertyFlag::HostVisible | MemoryPropertyFlag::HostCoherent);
    
    VkDeviceMemory vkVertexBufferMemory = VK_NULL_HANDLE;
    if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &vkVertexBufferMemory) != VK_SUCCESS) {
        Window::ShowError("Failed to allocate button vertex buffer memory!");
        return false;
    }
    m_vertexBufferMemory = vkVertexBufferMemory;
    
    vkBindBufferMemory(vkDevice, vkVertexBuffer, vkVertexBufferMemory, 0);
    
    // 填充数据
    void* data;
    vkMapMemory(vkDevice, vkVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, buttonVertices, (size_t)bufferSize);
    vkUnmapMemory(vkDevice, vkVertexBufferMemory);
    
    return true;
}

void Button::UpdateButtonBuffer() {
    VkDeviceMemory vkVertexBufferMemory = static_cast<VkDeviceMemory>(m_vertexBufferMemory);
    if (!m_initialized || vkVertexBufferMemory == VK_NULL_HANDLE) return;
    
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    
    struct Vertex {
        float x, y;
        float r, g, b, a;
    };
    
    // 计算实际渲染颜色（考虑悬停效果）
    float renderR = m_colorR;
    float renderG = m_colorG;
    float renderB = m_colorB;
    float renderA = m_colorA;
    
    if (m_enableHoverEffect && m_isHovering) {
        if (m_hoverEffectType == 0) {
            // 变暗效果：将RGB值乘以(1 - strength)
            float darkenFactor = 1.0f - m_hoverEffectStrength;
            renderR *= darkenFactor;
            renderG *= darkenFactor;
            renderB *= darkenFactor;
        } else if (m_hoverEffectType == 1) {
            // 变淡效果：将Alpha值乘以(1 - strength)
            renderA *= (1.0f - m_hoverEffectStrength);
        }
    }
    
    Vertex buttonVertices[] = {
        {0.0f, 0.0f, renderR, renderG, renderB, renderA},
        {1.0f, 0.0f, renderR, renderG, renderB, renderA},
        {1.0f, 1.0f, renderR, renderG, renderB, renderA},
        {0.0f, 0.0f, renderR, renderG, renderB, renderA},
        {1.0f, 1.0f, renderR, renderG, renderB, renderA},
        {0.0f, 1.0f, renderR, renderG, renderB, renderA},
    };
    
    VkDeviceSize bufferSize = sizeof(buttonVertices);
    void* data;
    vkMapMemory(vkDevice, vkVertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, buttonVertices, (size_t)bufferSize);
    vkUnmapMemory(vkDevice, vkVertexBufferMemory);
}

uint32_t Button::FindMemoryType(uint32_t typeFilter, MemoryPropertyFlag properties) {
    if (m_renderContext) {
        // 使用抽象类型，直接调用渲染上下文的方法
        return m_renderContext->FindMemoryType(typeFilter, properties);
    }
    
    // 向后兼容：如果没有渲染上下文，使用旧方法（需要转换为 Vulkan 类型）
    VkPhysicalDevice vkPhysicalDevice = static_cast<VkPhysicalDevice>(m_physicalDevice);
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &memProperties);
    
    // 将抽象类型转换为 Vulkan 类型
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
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && 
            (memProperties.memoryTypes[i].propertyFlags & vkProperties) == vkProperties) {
            return i;
        }
    }
    
    Window::ShowError("Failed to find suitable memory type for button!");
    return 0;
}

bool Button::CreatePipeline(RenderPassHandle renderPass) {
    // 将抽象类型转换为 Vulkan 类型
    VkRenderPass vkRenderPass = static_cast<VkRenderPass>(renderPass);
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    // 加载shader（使用按钮专用的shader，支持纹理）
    std::vector<char> vertCode = renderer::shader::ShaderLoader::LoadSPIRV("renderer/ui/button/button.vert.spv");
    std::vector<char> fragCode = renderer::shader::ShaderLoader::LoadSPIRV("renderer/ui/button/button.frag.spv");
    
    if (vertCode.empty() || fragCode.empty()) {
        // 如果SPIR-V文件不存在，尝试从源码编译
        #ifdef USE_SHADERC
        std::ifstream vertFile("renderer/ui/button/button.vert");
        std::ifstream fragFile("renderer/ui/button/button.frag");
        if (vertFile.is_open() && fragFile.is_open()) {
            std::string vertSource((std::istreambuf_iterator<char>(vertFile)), std::istreambuf_iterator<char>());
            std::string fragSource((std::istreambuf_iterator<char>(fragFile)), std::istreambuf_iterator<char>());
            vertCode = renderer::shader::ShaderLoader::CompileGLSLFromSource(vertSource, VK_SHADER_STAGE_VERTEX_BIT, "button.vert");
            fragCode = renderer::shader::ShaderLoader::CompileGLSLFromSource(fragSource, VK_SHADER_STAGE_FRAGMENT_BIT, "button.frag");
        }
        #endif
    }
    
    if (vertCode.empty() || fragCode.empty()) {
        Window::ShowError("Failed to load shaders for button!");
        return false;
    }
    
    VkShaderModule vertShaderModule = renderer::shader::ShaderLoader::CreateShaderModuleFromSPIRV(vkDevice, vertCode);
    VkShaderModule fragShaderModule = renderer::shader::ShaderLoader::CreateShaderModuleFromSPIRV(vkDevice, fragCode);
    
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        Window::ShowError("Failed to create shader modules for button!");
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
    // Alpha混合：保持目标alpha不变，避免影响后续渲染
    // 使用ONE作为源alpha因子，ONE作为目标alpha因子，这样alpha值会累加
    // 但实际上我们想要的是：如果按钮不透明，alpha应该是1.0；如果透明，保持目标alpha
    // 使用ONE_MINUS_SRC_ALPHA作为目标alpha因子，这样：resultAlpha = srcAlpha * 1 + dstAlpha * (1 - srcAlpha)
    // 当srcAlpha=1时，resultAlpha=1；当srcAlpha=0时，resultAlpha=dstAlpha（保持不变）
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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
    
    // 深度测试状态（禁用深度测试，因为渲染通道没有深度附件）
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    // Push constants: position(2) + size(2) + screenSize(2) + useTexture(1) + shapeType(1) + hoverEffect(1) = 9 floats
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 9; // position(2) + size(2) + screenSize(2) + useTexture(1) + shapeType(1) + hoverEffect(1)
    
    // Pipeline layout（如果使用纹理，需要包含描述符集布局）
    VkDescriptorSetLayout vkDescriptorSetLayout = static_cast<VkDescriptorSetLayout>(m_descriptorSetLayout);
    VkDescriptorSetLayout setLayouts[] = {vkDescriptorSetLayout};
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = (vkDescriptorSetLayout != VK_NULL_HANDLE) ? 1 : 0;
    pipelineLayoutInfo.pSetLayouts = (vkDescriptorSetLayout != VK_NULL_HANDLE) ? setLayouts : nullptr;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    VkPipelineLayout vkPipelineLayout = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &vkPipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
        vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
        Window::ShowError("Failed to create pipeline layout for button!");
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
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = vkPipelineLayout;
    pipelineInfo.renderPass = vkRenderPass;
    pipelineInfo.subpass = 0;
    
    VkPipeline vkGraphicsPipeline = VK_NULL_HANDLE;
    VkResult result = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkGraphicsPipeline);
    m_graphicsPipeline = vkGraphicsPipeline;
    
    vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
    
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create graphics pipeline for button!");
        return false;
    }
    
    return true;
}

void Button::Render(CommandBufferHandle commandBuffer, Extent2D extent) {
    // 如果按钮不可见，不渲染
    if (!m_visible) return;
    
    // 将抽象类型转换为 Vulkan 类型
    VkCommandBuffer vkCommandBuffer = static_cast<VkCommandBuffer>(commandBuffer);
    VkExtent2D vkExtent = { extent.width, extent.height };
    
    // 将成员变量转换为 Vulkan 类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    VkPipeline vkGraphicsPipeline = static_cast<VkPipeline>(m_graphicsPipeline);
    VkBuffer vkVertexBuffer = static_cast<VkBuffer>(m_vertexBuffer);
    VkPipelineLayout vkPipelineLayout = static_cast<VkPipelineLayout>(m_pipelineLayout);
    VkDescriptorSet vkDescriptorSet = static_cast<VkDescriptorSet>(m_descriptorSet);
    
    // 根据选择的渲染方式调用相应的渲染方法
    if (m_usePureShader) {
        RenderPureShader(commandBuffer, extent);
    } else {
        // 传统渲染方式
        if (!m_initialized || vkGraphicsPipeline == VK_NULL_HANDLE || vkVertexBuffer == VK_NULL_HANDLE) return;
        
        // 调试输出：显示按钮信息（每60帧一次）
        static int renderDebugCount = 0;
        if (renderDebugCount % 60 == 0) {
            printf("[BUTTON RENDER] Button: pos=(%.2f, %.2f), size=(%.2f, %.2f), useTexture=%s, descriptorSet=%p, texturePath=%s\n",
                   m_x, m_y, m_width, m_height, m_useTexture ? "true" : "false", 
                   (void*)m_descriptorSet, m_texturePath.c_str());
        }
        renderDebugCount++;
        
        // 绑定管线
        vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkGraphicsPipeline);
        
        // 如果使用纹理，绑定描述符集
        if (m_useTexture && vkDescriptorSet != VK_NULL_HANDLE) {
            vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipelineLayout,
                                   0, 1, &vkDescriptorSet, 0, nullptr);
        }
        
        // 绑定顶点缓冲区
        VkBuffer vertexBuffers[] = {vkVertexBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
        
        // Scaled模式：将逻辑坐标转换为屏幕坐标
        float renderX = m_x;
        float renderY = m_y;
        float renderWidth = m_width;
        float renderHeight = m_height;
        float renderScreenWidth = (float)vkExtent.width;
        float renderScreenHeight = (float)vkExtent.height;
        
        if (m_stretchParams) {
            // 逻辑坐标 -> 屏幕坐标
            renderX = m_x * m_stretchParams->stretchScaleX + m_stretchParams->marginX;
            renderY = m_y * m_stretchParams->stretchScaleY + m_stretchParams->marginY;
            renderWidth = m_width * m_stretchParams->stretchScaleX;
            renderHeight = m_height * m_stretchParams->stretchScaleY;
            renderScreenWidth = m_stretchParams->screenWidth;
            renderScreenHeight = m_stretchParams->screenHeight;
        }
        
        // 计算翻转的Y坐标（与loading animation相同的逻辑）
        float flippedY = renderScreenHeight - renderY - renderHeight;
        
        // Push constants: position, size, screenSize, useTexture, shapeType, hoverEffect
        float useTexture = (m_useTexture && vkDescriptorSet != VK_NULL_HANDLE) ? 1.0f : 0.0f;
        // 计算悬停效果参数（用于shader中的纹理悬停效果）
        float hoverEffect = 0.0f;  // 0.0 = 无效果, >0.0 = 有悬停效果
        if (m_enableHoverEffect && m_isHovering) {
            hoverEffect = m_hoverEffectType == 0 ? m_hoverEffectStrength : -m_hoverEffectStrength;  // 正数=变暗, 负数=变淡
        }
        float pushConstants[9] = {
            renderX,                        // position.x (屏幕坐标)
            flippedY,                       // position.y (翻转的Y坐标)
            renderWidth,                    // size.x (屏幕尺寸)
            renderHeight,                   // size.y (屏幕尺寸)
            renderScreenWidth,              // screenSize.x (屏幕尺寸)
            renderScreenHeight,             // screenSize.y (屏幕尺寸)
            useTexture,                     // useTexture (1.0 = use texture, 0.0 = use color)
            (float)m_shapeType,            // shapeType (0.0=矩形, 1.0=圆形)
            hoverEffect                     // hoverEffect (0.0=无效果, >0.0=变暗, <0.0=变淡)
        };
        
        vkCmdPushConstants(vkCommandBuffer, vkPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 
                          0, sizeof(pushConstants), pushConstants);
        
        // 绘制按钮（6个顶点）
        vkCmdDraw(vkCommandBuffer, 6, 1, 0, 0);
    }
}

void Button::RenderText(CommandBufferHandle commandBuffer, Extent2D extent,
                        const void* viewport, const void* scissor) {
    // 如果按钮不可见，不渲染文本
    if (!m_visible) return;
    
    // 如果启用了文本且TextRenderer可用，渲染文本
    if (m_enableText && m_textRenderer && !m_text.empty()) {
        // 将抽象类型转换为 Vulkan 类型
        VkCommandBuffer vkCommandBuffer = static_cast<VkCommandBuffer>(commandBuffer);
        VkExtent2D vkExtent = { extent.width, extent.height };
        const VkViewport* vkViewport = static_cast<const VkViewport*>(viewport);
        const VkRect2D* vkScissor = static_cast<const VkRect2D*>(scissor);
        
        // 文本渲染始终使用全屏viewport和scissor，避免文本被裁剪
        // 即使提供了viewport和scissor（FIT模式），也不使用它们，确保文本完整显示
        VkViewport textViewport = {};
        textViewport.x = 0.0f;
        textViewport.y = 0.0f;
        textViewport.width = (float)vkExtent.width;
        textViewport.height = (float)vkExtent.height;
        textViewport.minDepth = 0.0f;
        textViewport.maxDepth = 1.0f;
        
        VkRect2D textScissor = {};
        textScissor.offset = {0, 0};
        textScissor.extent = vkExtent;
        
        vkCmdSetViewport(vkCommandBuffer, 0, 1, &textViewport);
        vkCmdSetScissor(vkCommandBuffer, 0, 1, &textScissor);
        
        // 计算按钮中心位置（逻辑坐标）
        float buttonCenterX = m_x + m_width / 2.0f;
        float buttonCenterY = m_y + m_height / 2.0f;
        
        // 计算屏幕坐标和屏幕尺寸
        float renderScreenWidth = (float)vkExtent.width;
        float renderScreenHeight = (float)vkExtent.height;
        
        if (m_stretchParams) {
            // Scaled模式：将逻辑坐标转换为屏幕坐标
            buttonCenterX = buttonCenterX * m_stretchParams->stretchScaleX + m_stretchParams->marginX;
            buttonCenterY = buttonCenterY * m_stretchParams->stretchScaleY + m_stretchParams->marginY;
            renderScreenWidth = m_stretchParams->screenWidth;
            renderScreenHeight = m_stretchParams->screenHeight;
        } else if (vkViewport && vkScissor) {
            // Fit/Disabled模式：将UI坐标转换为实际窗口坐标
            // UI坐标基于extent（UI基准大小），需要映射到viewport区域
            float uiToViewportScaleX = vkViewport->width / (float)vkExtent.width;
            float uiToViewportScaleY = vkViewport->height / (float)vkExtent.height;
            buttonCenterX = buttonCenterX * uiToViewportScaleX + vkViewport->x;
            buttonCenterY = buttonCenterY * uiToViewportScaleY + vkViewport->y;
            // 文本渲染使用全屏viewport，所以屏幕尺寸是实际窗口大小
            renderScreenWidth = (float)vkScissor->extent.width;
            renderScreenHeight = (float)vkScissor->extent.height;
        }
        
        // 检查textRenderer是否有效
        if (!m_textRenderer) {
            return;
        }
        
        // 使用中心坐标渲染文本（屏幕坐标）
        m_textRenderer->RenderTextCentered(commandBuffer, m_text,
                                          buttonCenterX, buttonCenterY,
                                          renderScreenWidth, renderScreenHeight,
                                          m_textColorR, m_textColorG, m_textColorB, m_textColorA);
    }
}

void Button::SetTexture(const std::string& texturePath) {
    // 只有传统渲染方式才需要清理Vulkan纹理资源
    if (!m_usePureShader) {
        CleanupTexture();
    }
    
    m_texturePath = texturePath;
    m_useTextureHitTest = false;  // 默认不使用纹理点击判定
    
    if (!texturePath.empty()) {
        // 加载纹理图像数据（用于点击判定）
        auto imageData = renderer::image::ImageLoader::LoadImage(texturePath);
        if (imageData.width > 0 && imageData.height > 0) {
            m_textureData.pixels = imageData.pixels;
            m_textureData.width = imageData.width;
            m_textureData.height = imageData.height;
            m_useTextureHitTest = true;  // 启用基于纹理的点击判定
            
            // 如果按钮大小未设置，使用纹理大小
            if (m_width <= 0 || m_height <= 0) {
                m_width = (float)imageData.width;
                m_height = (float)imageData.height;
            }
        }
        
        // 只有传统渲染方式才需要设置m_useTexture和加载Vulkan纹理
        if (!m_usePureShader) {
            printf("[BUTTON] SetTexture: Loading texture for traditional rendering mode\n");
            m_useTexture = true;
            if (!LoadTexture(texturePath)) {
                // 加载失败，回退到颜色模式
                printf("[BUTTON] SetTexture: Texture loading failed, falling back to color mode\n");
                m_useTexture = false;
            } else {
                printf("[BUTTON] SetTexture: Texture loaded successfully\n");
            }
        } else {
            printf("[BUTTON] SetTexture: usePureShader=true, skipping Vulkan texture load\n");
        }
    } else {
        m_textureData.pixels.clear();
        m_textureData.width = 0;
        m_textureData.height = 0;
        m_useTextureHitTest = false;
        m_useTexture = false;
        // 只有传统渲染方式才需要更新缓冲区
        if (!m_usePureShader) {
            UpdateButtonBuffer(); // 切换回颜色模式
        }
    }
}

bool Button::LoadTexture(const std::string& texturePath) {
    // 将抽象类型转换为 Vulkan 类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    VkPhysicalDevice vkPhysicalDevice = static_cast<VkPhysicalDevice>(m_physicalDevice);
    VkCommandPool vkCommandPool = static_cast<VkCommandPool>(m_commandPool);
    VkQueue vkGraphicsQueue = static_cast<VkQueue>(m_graphicsQueue);
    
    // 清理旧的纹理
    CleanupTexture();
    
    if (texturePath.empty()) {
        return true;
    }
    
    // 创建纹理对象
    m_texture = std::make_unique<renderer::texture::Texture>();
    
    // 加载纹理
    if (!m_texture->LoadFromFile(vkDevice, vkPhysicalDevice, vkCommandPool, vkGraphicsQueue, texturePath)) {
        m_texture.reset();
        Window::ShowError("Failed to load button texture: " + texturePath);
        return false;
    }
    
    // 创建描述符集布局（如果还没有创建）
    VkDescriptorSetLayout vkDescriptorSetLayout = static_cast<VkDescriptorSetLayout>(m_descriptorSetLayout);
    if (vkDescriptorSetLayout == VK_NULL_HANDLE) {
        if (!CreateDescriptorSetLayout()) {
            CleanupTexture();
            m_useTexture = false;  // 创建失败，设置为false
            return false;
        }
    }
    
    // 创建描述符集
    if (!CreateDescriptorSet()) {
        CleanupTexture();
        m_useTexture = false;  // 创建失败，设置为false
        return false;
    }
    
    // 确保设置useTexture标志（因为CleanupTexture可能会重置它）
    m_useTexture = true;
    
    return true;
}

void Button::CleanupTexture() {
    if (m_texture) {
        VkDevice vkDevice = static_cast<VkDevice>(m_device);
        m_texture->Cleanup(vkDevice);
        m_texture.reset();
    }
    // 注意：不在这里设置m_useTexture=false
    // 因为LoadTexture会先调用CleanupTexture清理旧纹理，然后加载新纹理
    // m_useTexture会在LoadTexture成功后设置为true，失败时保持原值或由调用者设置
}

bool Button::CreateDescriptorSetLayout() {
    // 将抽象类型转换为 Vulkan 类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    
    // 创建描述符集布局，用于绑定纹理采样器
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;
    
    VkDescriptorSetLayout vkDescriptorSetLayout = VK_NULL_HANDLE;
    VkResult result = vkCreateDescriptorSetLayout(vkDevice, &layoutInfo, nullptr, &vkDescriptorSetLayout);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create descriptor set layout for button texture!");
        return false;
    }
    m_descriptorSetLayout = vkDescriptorSetLayout;
    
    return true;
}

bool Button::HasTexture() const {
    return m_useTexture && m_texture != nullptr && m_texture->IsValid();
}

bool Button::CreateDescriptorSet() {
    // 将抽象类型转换为 Vulkan 类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    
    // 如果纹理不存在，不需要创建描述符集
    if (!m_texture || !m_texture->IsValid()) {
        return true;
    }
    
    // 创建描述符池
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1;
    
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;
    
    // 如果描述符池已存在，先清理
    VkDescriptorPool vkDescriptorPool = static_cast<VkDescriptorPool>(m_descriptorPool);
    if (vkDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(vkDevice, vkDescriptorPool, nullptr);
        m_descriptorPool = nullptr;
    }
    
    VkResult result = vkCreateDescriptorPool(vkDevice, &poolInfo, nullptr, &vkDescriptorPool);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create descriptor pool for button texture!");
        return false;
    }
    m_descriptorPool = vkDescriptorPool;
    
    // 分配描述符集
    VkDescriptorSetLayout vkDescriptorSetLayout = static_cast<VkDescriptorSetLayout>(m_descriptorSetLayout);
    VkDescriptorSetLayout setLayouts[] = {vkDescriptorSetLayout};
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vkDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = setLayouts;
    
    VkDescriptorSet vkDescriptorSet = VK_NULL_HANDLE;
    result = vkAllocateDescriptorSets(vkDevice, &allocInfo, &vkDescriptorSet);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to allocate descriptor set for button texture!");
        return false;
    }
    m_descriptorSet = vkDescriptorSet;
    
    // 更新描述符集，绑定纹理
    VkDescriptorImageInfo imageInfo = m_texture->GetDescriptorInfo();
    
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = vkDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;
    
    vkUpdateDescriptorSets(vkDevice, 1, &descriptorWrite, 0, nullptr);
    
    // 确保设置useTexture标志
    m_useTexture = true;
    
    return true;
}

void Button::SetSize(float width, float height) {
    m_width = width;
    m_height = height;
    if (m_useRelativePosition) {
        UpdateRelativePosition();
    }
}

void Button::SetColor(float r, float g, float b, float a) {
    m_colorR = r;
    m_colorG = g;
    m_colorB = b;
    m_colorA = a;
    if (m_texturePath.empty()) {
        UpdateButtonBuffer(); // 更新缓冲区颜色
    }
}

void Button::UpdateScreenSize(float screenWidth, float screenHeight) {
    if (m_fixedScreenSize) {
        // FIT模式：使用固定的screenSize，不更新
        return;
    }
    if (m_stretchParams) {
        // Scaled模式：不通过UpdateScreenSize更新，而是通过SetStretchParams更新
        return;
    }
    m_screenWidth = screenWidth;
    m_screenHeight = screenHeight;
    if (m_useRelativePosition) {
        UpdateRelativePosition();
    }
}

void Button::UpdateRelativePosition() {
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
    }
}

void Button::SetStretchParams(const struct StretchParams& params) {
    // 为StretchParams分配内存（如果还没有分配）
    if (!m_stretchParams) {
        m_stretchParams = std::make_unique<StretchParams>();
    }
    *m_stretchParams = params;
    
    // 如果使用相对位置，需要重新计算位置
    if (m_useRelativePosition) {
        UpdateRelativePosition();
    }
}

bool Button::IsPointInside(float px, float py) const {
    float checkX = px;
    float checkY = py;
    
    // Scaled模式：将屏幕坐标转换为逻辑坐标
    if (m_stretchParams) {
        checkX = (px - m_stretchParams->marginX) / m_stretchParams->stretchScaleX;
        checkY = (py - m_stretchParams->marginY) / m_stretchParams->stretchScaleY;
    }
    
    // 根据形状类型进行判断
    if (m_shapeType == 1) {
        // 圆形按钮：检查是否在圆形区域内
        float centerX = m_x + m_width * 0.5f;
        float centerY = m_y + m_height * 0.5f;
        float radius = std::min(m_width, m_height) * 0.5f;  // 使用较小的尺寸作为半径
        
        float dx = checkX - centerX;
        float dy = checkY - centerY;
        float distSquared = dx * dx + dy * dy;
        float radiusSquared = radius * radius;
        
        if (distSquared > radiusSquared) {
            return false;
        }
        
        // 如果使用纹理且启用了纹理点击判定，检查alpha通道
        if (m_useTextureHitTest && m_textureData.width > 0 && m_textureData.height > 0) {
            // 将窗口坐标转换为纹理坐标（使用逻辑坐标checkX和checkY）
            float localX = checkX - m_x;
            float localY = checkY - m_y;
            
            // 归一化到 [0, 1] 范围
            float normalizedX = localX / m_width;
            float normalizedY = localY / m_height;
            
            // 转换为纹理像素坐标
            uint32_t texX = (uint32_t)(normalizedX * m_textureData.width);
            uint32_t texY = (uint32_t)(normalizedY * m_textureData.height);
            
            // 检查该位置的alpha值（只有不透明区域才能点击）
            return m_textureData.IsOpaque(texX, texY, 128);
        }
        
        return true;
    } else {
        // 矩形按钮：检查是否在矩形区域内（逻辑坐标）
        if (checkX < m_x || checkX > m_x + m_width || checkY < m_y || checkY > m_y + m_height) {
            return false;
        }
        
        // 如果使用纹理且启用了纹理点击判定，检查alpha通道
        if (m_useTextureHitTest && m_textureData.width > 0 && m_textureData.height > 0) {
            // 将窗口坐标转换为纹理坐标（使用逻辑坐标checkX和checkY）
            float localX = checkX - m_x;
            float localY = checkY - m_y;
            
            // 归一化到 [0, 1] 范围
            float normalizedX = localX / m_width;
            float normalizedY = localY / m_height;
            
            // 直接使用归一化坐标映射到纹理坐标 [0, 1]
            // 注意：shader已经移除了纹理缩放，所以这里也直接使用完整范围
            float texCoordX = normalizedX;
            float texCoordY = normalizedY;
            
            // 转换为纹理像素坐标
            uint32_t texX = (uint32_t)(texCoordX * m_textureData.width);
            uint32_t texY = (uint32_t)(texCoordY * m_textureData.height);
            
            // 检查该位置的alpha值（只有不透明区域才能点击）
            return m_textureData.IsOpaque(texX, texY, 128);
        }
        
        // 默认：矩形区域判定
        return true;
    }
}

bool Button::CreateFullscreenQuadBuffer() {
    // 将抽象类型转换为 Vulkan 类型
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    
    // 创建全屏四边形的顶点缓冲区（用于纯shader渲染）
    // 顶点只包含位置信息（归一化坐标 0-1）
    struct Vertex {
        float x, y;
    };
    
    // 全屏四边形：两个三角形覆盖整个屏幕
    Vertex quadVertices[] = {
        // 第一个三角形
        {0.0f, 0.0f},  // 左上
        {1.0f, 0.0f},  // 右上
        {1.0f, 1.0f},  // 右下
        // 第二个三角形
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
    
    VkBuffer vkFullscreenQuadBuffer = VK_NULL_HANDLE;
    if (vkCreateBuffer(vkDevice, &bufferInfo, nullptr, &vkFullscreenQuadBuffer) != VK_SUCCESS) {
        Window::ShowError("Failed to create fullscreen quad vertex buffer!");
        return false;
    }
    m_fullscreenQuadBuffer = vkFullscreenQuadBuffer;
    
    // 分配内存
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(vkDevice, vkFullscreenQuadBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, 
                                                MemoryPropertyFlag::HostVisible | MemoryPropertyFlag::HostCoherent);
    
    VkDeviceMemory vkFullscreenQuadBufferMemory = VK_NULL_HANDLE;
    if (vkAllocateMemory(vkDevice, &allocInfo, nullptr, &vkFullscreenQuadBufferMemory) != VK_SUCCESS) {
        Window::ShowError("Failed to allocate fullscreen quad vertex buffer memory!");
        return false;
    }
    m_fullscreenQuadBufferMemory = vkFullscreenQuadBufferMemory;
    
    vkBindBufferMemory(vkDevice, vkFullscreenQuadBuffer, vkFullscreenQuadBufferMemory, 0);
    
    // 填充数据
    void* data;
    vkMapMemory(vkDevice, vkFullscreenQuadBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, quadVertices, (size_t)bufferSize);
    vkUnmapMemory(vkDevice, vkFullscreenQuadBufferMemory);
    
    return true;
}

bool Button::CreatePureShaderPipeline(RenderPassHandle renderPass) {
    // 将抽象类型转换为 Vulkan 类型
    VkRenderPass vkRenderPass = static_cast<VkRenderPass>(renderPass);
    VkDevice vkDevice = static_cast<VkDevice>(m_device);
    // 加载纯shader（使用新创建的shader文件）
    std::vector<char> vertCode = renderer::shader::ShaderLoader::LoadSPIRV("renderer/ui/button/button_pure.vert.spv");
    std::vector<char> fragCode = renderer::shader::ShaderLoader::LoadSPIRV("renderer/ui/button/button_pure.frag.spv");
    
    if (vertCode.empty() || fragCode.empty()) {
        // 如果SPIR-V文件不存在，尝试从源码编译
        #ifdef USE_SHADERC
        std::ifstream vertFile("renderer/ui/button/button_pure.vert");
        std::ifstream fragFile("renderer/ui/button/button_pure.frag");
        if (vertFile.is_open() && fragFile.is_open()) {
            std::string vertSource((std::istreambuf_iterator<char>(vertFile)), std::istreambuf_iterator<char>());
            std::string fragSource((std::istreambuf_iterator<char>(fragFile)), std::istreambuf_iterator<char>());
            vertCode = renderer::shader::ShaderLoader::CompileGLSLFromSource(vertSource, VK_SHADER_STAGE_VERTEX_BIT, "button_pure.vert");
            fragCode = renderer::shader::ShaderLoader::CompileGLSLFromSource(fragSource, VK_SHADER_STAGE_FRAGMENT_BIT, "button_pure.frag");
        }
        #endif
    }
    
    if (vertCode.empty() || fragCode.empty()) {
        Window::ShowError("Failed to load pure shader shaders for button!");
        return false;
    }
    
    VkShaderModule vertShaderModule = renderer::shader::ShaderLoader::CreateShaderModuleFromSPIRV(vkDevice, vertCode);
    VkShaderModule fragShaderModule = renderer::shader::ShaderLoader::CreateShaderModuleFromSPIRV(vkDevice, fragCode);
    
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        Window::ShowError("Failed to create pure shader modules for button!");
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
    
    // 顶点输入（只有位置，没有颜色）
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
    // Alpha混合：保持目标alpha不变，避免影响后续渲染
    // 使用ONE作为源alpha因子，ONE作为目标alpha因子，这样alpha值会累加
    // 但实际上我们想要的是：如果按钮不透明，alpha应该是1.0；如果透明，保持目标alpha
    // 使用ONE_MINUS_SRC_ALPHA作为目标alpha因子，这样：resultAlpha = srcAlpha * 1 + dstAlpha * (1 - srcAlpha)
    // 当srcAlpha=1时，resultAlpha=1；当srcAlpha=0时，resultAlpha=dstAlpha（保持不变）
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
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
    
    // 深度测试状态（禁用深度测试，因为渲染通道没有深度附件）
    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_ALWAYS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    // Push constants: position(2) + size(2) + screenSize(2) + color(4) + shapeType(1) = 11 floats
    // 注意：vec4 color需要4个float，shapeType需要1个float，所以总共是11个float
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 11;
    
    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    VkPipelineLayout vkPureShaderPipelineLayout = VK_NULL_HANDLE;
    if (vkCreatePipelineLayout(vkDevice, &pipelineLayoutInfo, nullptr, &vkPureShaderPipelineLayout) != VK_SUCCESS) {
        vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
        vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
        Window::ShowError("Failed to create pure shader pipeline layout for button!");
        return false;
    }
    m_pureShaderPipelineLayout = vkPureShaderPipelineLayout;
    
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
    pipelineInfo.layout = vkPureShaderPipelineLayout;
    pipelineInfo.renderPass = vkRenderPass;
    pipelineInfo.subpass = 0;
    
    VkPipeline vkPureShaderPipeline = VK_NULL_HANDLE;
    VkResult result = vkCreateGraphicsPipelines(vkDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vkPureShaderPipeline);
    m_pureShaderPipeline = vkPureShaderPipeline;
    
    vkDestroyShaderModule(vkDevice, vertShaderModule, nullptr);
    vkDestroyShaderModule(vkDevice, fragShaderModule, nullptr);
    
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create pure shader graphics pipeline for button!");
        return false;
    }
    
    return true;
}

void Button::RenderPureShader(CommandBufferHandle commandBuffer, Extent2D extent) {
    // 如果按钮不可见，不渲染
    if (!m_visible) return;
    
    // 将抽象类型转换为 Vulkan 类型
    VkCommandBuffer vkCommandBuffer = static_cast<VkCommandBuffer>(commandBuffer);
    VkExtent2D vkExtent = { extent.width, extent.height };
    VkPipeline vkPureShaderPipeline = static_cast<VkPipeline>(m_pureShaderPipeline);
    VkBuffer vkFullscreenQuadBuffer = static_cast<VkBuffer>(m_fullscreenQuadBuffer);
    VkPipelineLayout vkPureShaderPipelineLayout = static_cast<VkPipelineLayout>(m_pureShaderPipelineLayout);
    
    if (!m_initialized || vkPureShaderPipeline == VK_NULL_HANDLE || vkFullscreenQuadBuffer == VK_NULL_HANDLE) return;
    
    // 绑定管线
    vkCmdBindPipeline(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPureShaderPipeline);
    
    // 绑定全屏四边形顶点缓冲区
    VkBuffer vertexBuffers[] = {vkFullscreenQuadBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
    
    // 设置视口和裁剪区域
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)vkExtent.width;
    viewport.height = (float)vkExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(vkCommandBuffer, 0, 1, &viewport);
    
    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = vkExtent;
    vkCmdSetScissor(vkCommandBuffer, 0, 1, &scissor);
    
    // 计算实际渲染颜色（考虑悬停效果）
    float renderR = m_colorR;
    float renderG = m_colorG;
    float renderB = m_colorB;
    float renderA = m_colorA;
    
    if (m_enableHoverEffect && m_isHovering) {
        if (m_hoverEffectType == 0) {
            // 变暗效果：将RGB值乘以(1 - strength)
            float darkenFactor = 1.0f - m_hoverEffectStrength;
            renderR *= darkenFactor;
            renderG *= darkenFactor;
            renderB *= darkenFactor;
        } else if (m_hoverEffectType == 1) {
            // 变淡效果：将Alpha值乘以(1 - strength)
            renderA *= (1.0f - m_hoverEffectStrength);
        }
    }
    
    // Push constants: position(2) + size(2) + screenSize(2) + color(4) + shapeType(1) = 11 floats
    // 注意：按钮位置使用窗口坐标（Y向下），片段着色器会直接使用
    // 布局必须与shader中的PushConstants结构匹配：
    // vec2 position (2 floats)
    // vec2 size (2 floats)  
    // vec2 screenSize (2 floats)
    // vec4 color (4 floats)
    // float shapeType (1 float)
    float pushConstants[11] = {
        m_x,                            // position.x (window coordinates)
        m_y,                            // position.y (window coordinates, Y down)
        m_width,                        // size.x
        m_height,                       // size.y
        (float)vkExtent.width,            // screenSize.x
        (float)vkExtent.height,           // screenSize.y
        renderR,                       // color.r (应用悬停效果后)
        renderG,                       // color.g (应用悬停效果后)
        renderB,                       // color.b (应用悬停效果后)
        renderA,                       // color.a (应用悬停效果后)
        (float)m_shapeType              // shapeType (0.0=矩形, 1.0=圆形)
    };
    
    vkCmdPushConstants(vkCommandBuffer, vkPureShaderPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 
                      0, sizeof(pushConstants), pushConstants);
    
    // 绘制全屏四边形（6个顶点）
    vkCmdDraw(vkCommandBuffer, 6, 1, 0, 0);
}


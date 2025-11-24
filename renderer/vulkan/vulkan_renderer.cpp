#include "vulkan/vulkan_renderer.h"
#include "ui/slider/slider.h"
#include "shader/shader_loader.h"
#include "window/window.h"
#include "core/constants.h"
#include "loading/loading_animation.h"
#include "text/text_renderer.h"
#include "ui/button/button.h"
#include "texture/texture.h"
#include "image/image_loader.h"
#include <set>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdio.h>
#include <windows.h>
#include <memory>

using namespace renderer::shader;

VulkanRenderer::VulkanRenderer() {
}

VulkanRenderer::~VulkanRenderer() {
    Cleanup();
}

bool VulkanRenderer::Initialize(HWND hwnd, HINSTANCE hInstance) {
    m_hwnd = hwnd;
    
    if (!CreateInstance()) return false;
    if (!CreateSurface(hwnd, hInstance)) return false;
    if (!SelectPhysicalDevice()) return false;
    if (!CreateLogicalDevice()) return false;
    if (!CreateSwapchain()) return false;
    if (!CreateImageViews()) return false;
    if (!CreateRenderPass()) return false;
    if (!CreateFramebuffers()) return false;
    if (!CreateCommandPool()) return false;
    if (!CreateCommandBuffers()) return false;
    if (!CreateSyncObjects()) return false;
    
    return true;
}

void VulkanRenderer::Cleanup() {
    if (m_device != VK_NULL_HANDLE) {
        VkResult result = vkDeviceWaitIdle(m_device);
        if (result != VK_SUCCESS) {
            printf("[WARNING] vkDeviceWaitIdle failed during cleanup: %d\n", result);
        }
    }
    
    // 清理同步对象
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (m_imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
        }
        if (m_renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
            vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
        }
        if (m_inFlightFences[i] != VK_NULL_HANDLE) {
            vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
        }
    }
    
    // 清理命令池
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
    
    // 清理背景纹理
    CleanupBackgroundTexture();
    
    // 清理图形管线
    if (m_graphicsPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
        m_graphicsPipeline = VK_NULL_HANDLE;
    }
    
    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    
    // 清理loading_cubes管线
    if (m_loadingCubesPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_loadingCubesPipeline, nullptr);
        m_loadingCubesPipeline = VK_NULL_HANDLE;
    }
    
    if (m_loadingCubesPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_loadingCubesPipelineLayout, nullptr);
        m_loadingCubesPipelineLayout = VK_NULL_HANDLE;
    }
    
    // 清理光线追踪管线
    if (m_rayTracingPipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_rayTracingPipeline, nullptr);
        m_rayTracingPipeline = VK_NULL_HANDLE;
    }
    
    if (m_rayTracingPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_rayTracingPipelineLayout, nullptr);
        m_rayTracingPipelineLayout = VK_NULL_HANDLE;
    }
    
    if (m_rayTracingDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_rayTracingDescriptorSetLayout, nullptr);
        m_rayTracingDescriptorSetLayout = VK_NULL_HANDLE;
    }
    
    CleanupSwapchain();
    
    // 清理设备
    if (m_device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_device, nullptr);
        m_device = VK_NULL_HANDLE;
    }
    
    // 清理表面
    if (m_surface != VK_NULL_HANDLE) {
        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        m_surface = VK_NULL_HANDLE;
    }
    
    // 清理实例
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

bool VulkanRenderer::CreateInstance() {
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Shader Gouyu";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;
    
    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME
    };
    
    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();
    
    VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create Vulkan instance!");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::CreateSurface(HWND hwnd, HINSTANCE hInstance) {
    VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.hwnd = hwnd;
    surfaceCreateInfo.hinstance = hInstance;
    
    VkResult result = vkCreateWin32SurfaceKHR(m_instance, &surfaceCreateInfo, nullptr, &m_surface);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create surface!");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::SelectPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        Window::ShowError("No Vulkan devices found!");
        return false;
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    m_physicalDevice = devices[0]; // 简单选择第一个设备
    
    return true;
}

bool VulkanRenderer::CreateLogicalDevice() {
    // 查找队列族
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &queueFamilyCount, queueFamilies.data());
    
    m_graphicsQueueFamily = UINT32_MAX;
    m_presentQueueFamily = UINT32_MAX;
    
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            m_graphicsQueueFamily = i;
        }
        
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_physicalDevice, i, m_surface, &presentSupport);
        if (presentSupport) {
            m_presentQueueFamily = i;
        }
        
        if (m_graphicsQueueFamily != UINT32_MAX && m_presentQueueFamily != UINT32_MAX) {
            break;
        }
    }
    
    if (m_graphicsQueueFamily == UINT32_MAX || m_presentQueueFamily == UINT32_MAX) {
        Window::ShowError("No suitable queue family found!");
        return false;
    }
    
    // 创建逻辑设备
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {m_graphicsQueueFamily, m_presentQueueFamily};
    
    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
    
    // 检查并启用光线追踪扩展
    std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    
    // 检查光线追踪支持
    m_rayTracingSupported = CheckRayTracingSupport();
    if (m_rayTracingSupported) {
        // 添加光线追踪相关扩展
        deviceExtensions.push_back(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME);
        deviceExtensions.push_back(VK_KHR_RAY_QUERY_EXTENSION_NAME);
        printf("[RAYTRACING] Ray tracing extensions enabled\n");
    } else {
        printf("[RAYTRACING] Ray tracing not supported, falling back to software ray casting\n");
    }
    
    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.queueCreateInfoCount = queueCreateInfos.size();
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions.data();
    deviceCreateInfo.enabledExtensionCount = deviceExtensions.size();
    
    VkResult result = vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create logical device!");
        return false;
    }
    
    vkGetDeviceQueue(m_device, m_graphicsQueueFamily, 0, &m_graphicsQueue);
    vkGetDeviceQueue(m_device, m_presentQueueFamily, 0, &m_presentQueue);
    
    return true;
}

bool VulkanRenderer::CreateSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_physicalDevice, m_surface, &capabilities);
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, &formatCount, formats.data());
    
    m_swapchainImageFormat = formats[0].format;
    m_swapchainExtent = capabilities.currentExtent;
    if (m_swapchainExtent.width == UINT32_MAX) {
        m_swapchainExtent = { WINDOW_WIDTH, WINDOW_HEIGHT };
    }
    
    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = m_surface;
    swapchainCreateInfo.minImageCount = imageCount;
    swapchainCreateInfo.imageFormat = m_swapchainImageFormat;
    swapchainCreateInfo.imageColorSpace = formats[0].colorSpace;
    swapchainCreateInfo.imageExtent = m_swapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    uint32_t queueFamilyIndices[] = {m_graphicsQueueFamily, m_presentQueueFamily};
    if (m_graphicsQueueFamily != m_presentQueueFamily) {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    
    swapchainCreateInfo.preTransform = capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;
    
    VkResult result = vkCreateSwapchainKHR(m_device, &swapchainCreateInfo, nullptr, &m_swapchain);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create swap chain!");
        return false;
    }
    
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchainImageCount, nullptr);
    m_swapchainImages.resize(m_swapchainImageCount);
    vkGetSwapchainImagesKHR(m_device, m_swapchain, &m_swapchainImageCount, m_swapchainImages.data());
    
    return true;
}

bool VulkanRenderer::CreateImageViews() {
    m_swapchainImageViews.resize(m_swapchainImageCount);
    for (uint32_t i = 0; i < m_swapchainImageCount; i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        VkResult result = vkCreateImageView(m_device, &createInfo, nullptr, &m_swapchainImageViews[i]);
        if (result != VK_SUCCESS) {
            Window::ShowError("Failed to create image views!");
            return false;
        }
    }
    
    return true;
}

bool VulkanRenderer::CreateRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_swapchainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    
    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    
    VkResult result = vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create render pass!");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::CreateGraphicsPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath) {
    // 创建管道布局
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 2; // time + aspect
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create pipeline layout!");
        return false;
    }
    
    // 加载shader（支持SPIR-V文件或GLSL文件）
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    
    // 尝试加载SPIR-V文件或编译GLSL文件
    size_t vertExtPos = vertShaderPath.find_last_of('.');
    if (vertExtPos != std::string::npos && vertShaderPath.substr(vertExtPos) == ".spv") {
        vertShaderCode = ShaderLoader::LoadSPIRV(vertShaderPath);
    } else {
        // 尝试从GLSL编译
        vertShaderCode = ShaderLoader::CompileGLSLFromFile(vertShaderPath, VK_SHADER_STAGE_VERTEX_BIT);
    }
    
    size_t fragExtPos = fragShaderPath.find_last_of('.');
    if (fragExtPos != std::string::npos && fragShaderPath.substr(fragExtPos) == ".spv") {
        fragShaderCode = ShaderLoader::LoadSPIRV(fragShaderPath);
    } else {
        fragShaderCode = ShaderLoader::CompileGLSLFromFile(fragShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    
    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        Window::ShowError("Failed to load shaders! Make sure " + vertShaderPath + " and " + fragShaderPath + " exist.");
        return false;
    }
    
    VkShaderModule vertShaderModule = ShaderLoader::CreateShaderModuleFromSPIRV(m_device, vertShaderCode);
    VkShaderModule fragShaderModule = ShaderLoader::CreateShaderModuleFromSPIRV(m_device, fragShaderCode);
    
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        Window::ShowError("Failed to create shader modules!");
        return false;
    }
    
    // 创建shader阶段
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
    
    // 顶点输入状态（使用内置顶点）
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    
    // 输入装配状态
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // 视口和裁剪
    // Use dynamic viewport and scissor (like Godot) for aspect ratio scaling
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;  // Dynamic
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;    // Dynamic
    
    // 光栅化状态
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // 颜色混合
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // Dynamic states (like Godot) for aspect ratio scaling
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineDynamicStateCreateInfo dynamicState = {};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    
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
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    
    result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline);
    
    // 清理shader模块
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create graphics pipeline!");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::CreateLoadingCubesPipeline(const std::string& vertShaderPath, const std::string& fragShaderPath) {
    // 创建管道布局（与主pipeline相同）
    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(float) * 7; // time + aspect + cameraYaw + cameraPitch + cameraPosX + cameraPosY + cameraPosZ
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    VkResult result = vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_loadingCubesPipelineLayout);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create loading cubes pipeline layout!");
        return false;
    }
    
    // 加载shader（支持SPIR-V文件或GLSL文件）
    std::vector<char> vertShaderCode;
    std::vector<char> fragShaderCode;
    
    // 尝试加载SPIR-V文件或编译GLSL文件
    size_t vertExtPos = vertShaderPath.find_last_of('.');
    if (vertExtPos != std::string::npos && vertShaderPath.substr(vertExtPos) == ".spv") {
        vertShaderCode = ShaderLoader::LoadSPIRV(vertShaderPath);
    } else {
        // 尝试从GLSL编译
        vertShaderCode = ShaderLoader::CompileGLSLFromFile(vertShaderPath, VK_SHADER_STAGE_VERTEX_BIT);
    }
    
    size_t fragExtPos = fragShaderPath.find_last_of('.');
    if (fragExtPos != std::string::npos && fragShaderPath.substr(fragExtPos) == ".spv") {
        fragShaderCode = ShaderLoader::LoadSPIRV(fragShaderPath);
    } else {
        fragShaderCode = ShaderLoader::CompileGLSLFromFile(fragShaderPath, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    
    if (vertShaderCode.empty() || fragShaderCode.empty()) {
        Window::ShowError("Failed to load loading cubes shaders! Make sure " + vertShaderPath + " and " + fragShaderPath + " exist.");
        return false;
    }
    
    VkShaderModule vertShaderModule = ShaderLoader::CreateShaderModuleFromSPIRV(m_device, vertShaderCode);
    VkShaderModule fragShaderModule = ShaderLoader::CreateShaderModuleFromSPIRV(m_device, fragShaderCode);
    
    if (vertShaderModule == VK_NULL_HANDLE || fragShaderModule == VK_NULL_HANDLE) {
        Window::ShowError("Failed to create loading cubes shader modules!");
        return false;
    }
    
    // 创建shader阶段
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
    
    // 顶点输入状态（使用内置顶点）
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    
    // 输入装配状态
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // 视口和裁剪
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr;  // Dynamic
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr;    // Dynamic
    
    // 光栅化状态
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    // 多重采样
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // 颜色混合
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
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
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_loadingCubesPipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    
    result = vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_loadingCubesPipeline);
    
    // 清理shader模块
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create loading cubes graphics pipeline!");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::CreateFramebuffers() {
    m_swapchainFramebuffers.resize(m_swapchainImageCount);
    for (size_t i = 0; i < m_swapchainImageCount; i++) {
        VkImageView attachments[] = { m_swapchainImageViews[i] };
        
        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_swapchainExtent.width;
        framebufferInfo.height = m_swapchainExtent.height;
        framebufferInfo.layers = 1;
        
        VkResult result = vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapchainFramebuffers[i]);
        if (result != VK_SUCCESS) {
            Window::ShowError("Failed to create framebuffer!");
            return false;
        }
    }
    
    return true;
}

bool VulkanRenderer::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_graphicsQueueFamily;
    
    VkResult result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create command pool!");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::CreateCommandBuffers() {
    m_commandBuffers.resize(m_swapchainImageCount);
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = m_swapchainImageCount;
    
    VkResult result = vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data());
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to allocate command buffers!");
        return false;
    }
    
    return true;
}

bool VulkanRenderer::CreateSyncObjects() {
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult result1 = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
        VkResult result2 = vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
        VkResult result3 = vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]);
        
        if (result1 != VK_SUCCESS || result2 != VK_SUCCESS || result3 != VK_SUCCESS) {
            Window::ShowError("Failed to create synchronization objects!");
            return false;
        }
    }
    
    return true;
}

void VulkanRenderer::CleanupSwapchain() {
    for (auto framebuffer : m_swapchainFramebuffers) {
        vkDestroyFramebuffer(m_device, framebuffer, nullptr);
    }
    m_swapchainFramebuffers.clear();
    
    for (auto imageView : m_swapchainImageViews) {
        vkDestroyImageView(m_device, imageView, nullptr);
    }
    m_swapchainImageViews.clear();
    
    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
}

void VulkanRenderer::RecreateSwapchain() {
    VkResult result = vkDeviceWaitIdle(m_device);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to wait for device idle during swapchain recreation!");
        return;
    }
    
    CleanupSwapchain();
    
    if (!CreateSwapchain()) {
        Window::ShowError("Failed to recreate swapchain!");
        return;
    }
    
    if (!CreateImageViews()) {
        Window::ShowError("Failed to recreate swapchain image views!");
        CleanupSwapchain();
        return;
    }
    
    if (!CreateFramebuffers()) {
        Window::ShowError("Failed to recreate swapchain framebuffers!");
        CleanupSwapchain();
        return;
    }
}

void VulkanRenderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, float time, bool useLoadingCubes, TextRenderer* textRenderer, float fps) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to begin recording command buffer!");
        return;
    }
    
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchainExtent;
    
    // 根据状态设置背景色：loading_cubes使用淡棕色，其他使用黑色
    VkClearValue clearColor;
    if (useLoadingCubes) {
        // 淡棕色：RGB(210, 180, 140) / 255.0
        clearColor.color.float32[0] = 210.0f / 255.0f;  // R
        clearColor.color.float32[1] = 180.0f / 255.0f;  // G
        clearColor.color.float32[2] = 140.0f / 255.0f;  // B
        clearColor.color.float32[3] = 1.0f;              // A
    } else {
        clearColor.color.float32[0] = 0.0f;
        clearColor.color.float32[1] = 0.0f;
        clearColor.color.float32[2] = 0.0f;
        clearColor.color.float32[3] = 1.0f;
    }
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // 根据状态选择pipeline
    if (useLoadingCubes) {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_loadingCubesPipeline);
    } else {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
    }
    
    // Calculate viewport and scissor with stretch mode and aspect ratio scaling (like Godot)
    // Reference: Godot's window.cpp _update_viewport_size() and viewport.cpp _set_size()
    const float baseWidth = (float)WINDOW_WIDTH;   // desired_res (target resolution)
    const float baseHeight = (float)WINDOW_HEIGHT;
    const float targetAspect = baseWidth / baseHeight;  // 800/800 = 1.0
    const float videoModeAspect = (float)m_swapchainExtent.width / (float)m_swapchainExtent.height;
    
    VkViewport viewport = {};
    VkRect2D scissor = {};
    float aspect = 1.0f;  // Aspect ratio for shader
    
    // loading_cubes shader全屏显示，忽略stretch mode
    if (useLoadingCubes) {
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float)m_swapchainExtent.width;
        viewport.height = (float)m_swapchainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        
        scissor.offset = {0, 0};
        scissor.extent = m_swapchainExtent;
        
        aspect = (float)m_swapchainExtent.width / (float)m_swapchainExtent.height;
    } else {
        // 其他shader使用原来的stretch mode逻辑
        switch (m_stretchMode) {
        case StretchMode::Disabled: {
            // Disabled: no stretching, keep original size (800x800), center it
            float offsetX = ((float)m_swapchainExtent.width - baseWidth) * 0.5f;
            float offsetY = ((float)m_swapchainExtent.height - baseHeight) * 0.5f;
            
            viewport.x = offsetX;
            viewport.y = offsetY;
            viewport.width = baseWidth;
            viewport.height = baseHeight;
            
            scissor.offset.x = (int32_t)offsetX;
            scissor.offset.y = (int32_t)offsetY;
            scissor.extent.width = (uint32_t)baseWidth;
            scissor.extent.height = (uint32_t)baseHeight;
            aspect = targetAspect;
            break;
        }
        case StretchMode::Scaled: {
            // Canvas items mode: 保持 800x800 视口，如果窗口大小不同，居中显示并留黑边
            // UI位置在逻辑空间（800x800）中定义，渲染时转换为屏幕坐标
            float viewportWidth = baseWidth;   // 逻辑宽度 800
            float viewportHeight = baseHeight; // 逻辑高度 800
            float screenWidth = (float)m_swapchainExtent.width;
            float screenHeight = (float)m_swapchainExtent.height;
            float offsetX = (screenWidth - viewportWidth) * 0.5f;
            float offsetY = (screenHeight - viewportHeight) * 0.5f;
            
            // 计算拉伸比例（逻辑坐标到屏幕坐标的缩放）
            float stretchScaleX = screenWidth / viewportWidth;
            float stretchScaleY = screenHeight / viewportHeight;
            
            // 保存拉伸参数（用于UI渲染）
            m_stretchParams.stretchScaleX = stretchScaleX;
            m_stretchParams.stretchScaleY = stretchScaleY;
            m_stretchParams.logicalWidth = viewportWidth;
            m_stretchParams.logicalHeight = viewportHeight;
            m_stretchParams.screenWidth = screenWidth;
            m_stretchParams.screenHeight = screenHeight;
            m_stretchParams.marginX = offsetX;
            m_stretchParams.marginY = offsetY;
            
            viewport.x = offsetX;
            viewport.y = offsetY;
            viewport.width = viewportWidth;
            viewport.height = viewportHeight;
            
            scissor.offset.x = (int32_t)offsetX;
            scissor.offset.y = (int32_t)offsetY;
            scissor.extent.width = (uint32_t)viewportWidth;
            scissor.extent.height = (uint32_t)viewportHeight;
            
            // aspect 设置为目标宽高比（1.0），与 Disabled 模式保持一致
            aspect = targetAspect;
            break;
        }
        case StretchMode::Fit: {
            // Fit mode: 完全照抄 example/vulkan 的 AspectRatioMode::Keep 实现
            // Keep aspect ratio, add black bars (letterbox/pillarbox)
            const float currentAspect = (float)m_swapchainExtent.width / (float)m_swapchainExtent.height;
            
            if (currentAspect > targetAspect) {
                // Window is wider - add pillarbox
                float viewportHeight = (float)m_swapchainExtent.height;
                float viewportWidth = viewportHeight * targetAspect;
                float offsetX = ((float)m_swapchainExtent.width - viewportWidth) * 0.5f;
                
                viewport.x = offsetX;
                viewport.y = 0.0f;
                viewport.width = viewportWidth;
                viewport.height = viewportHeight;
                
                scissor.offset.x = (int32_t)offsetX;
                scissor.offset.y = 0;
                scissor.extent.width = (uint32_t)viewportWidth;
                scissor.extent.height = (uint32_t)viewportHeight;
            } else {
                // Window is taller - add letterbox
                float viewportWidth = (float)m_swapchainExtent.width;
                float viewportHeight = viewportWidth / targetAspect;
                float offsetY = ((float)m_swapchainExtent.height - viewportHeight) * 0.5f;
                
                viewport.x = 0.0f;
                viewport.y = offsetY;
                viewport.width = viewportWidth;
                viewport.height = viewportHeight;
                
                scissor.offset.x = 0;
                scissor.offset.y = (int32_t)offsetY;
                scissor.extent.width = (uint32_t)viewportWidth;
                scissor.extent.height = (uint32_t)viewportHeight;
            }
            aspect = targetAspect;
            break;
        }
        }  // 结束 switch 语句
        
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
    }
    
    // Set dynamic viewport and scissor (like Godot)
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    
    // 推送常量
    VkPipelineLayout currentPipelineLayout = useLoadingCubes ? m_loadingCubesPipelineLayout : m_pipelineLayout;
    if (useLoadingCubes) {
        // loading_cubes shader需要7个参数：time + aspect + cameraYaw + cameraPitch + cameraPosX + cameraPosY + cameraPosZ
        float pushConstants[7] = {
            time, aspect,
            m_cameraYaw, m_cameraPitch,
            m_cameraPosX, m_cameraPosY, m_cameraPosZ
        };
        vkCmdPushConstants(commandBuffer, currentPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) * 7, pushConstants);
    } else {
        // 普通shader只需要2个参数：time + aspect
        float pushConstants[2] = {time, aspect};
        vkCmdPushConstants(commandBuffer, currentPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float) * 2, pushConstants);
    }
    
    // 如果支持硬件光线追踪且pipeline已创建，使用硬件光追
    // 否则使用软件ray casting（当前实现）
    if (useLoadingCubes && m_rayTracingSupported && m_rayTracingPipeline != VK_NULL_HANDLE) {
        // 硬件光线追踪渲染路径
        // 注意：这需要完整的实现，包括：
        // - Acceleration structures已构建（BVH树等）
        // - Shader binding table已创建
        // - Ray tracing pipeline已创建
        // 
        // 硬件光追的优势：
        // 1. 使用专用RT Core硬件加速
        // 2. BVH加速结构，O(log n)复杂度而非O(n)
        // 3. 并行处理多条射线
        // 4. 性能比软件实现快10-100倍
        //
        // 示例代码（需要完整实现后才能使用）：
        // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rayTracingPipeline);
        // vkCmdTraceRaysKHR(commandBuffer, 
        //                   &raygenShaderBindingTableRegion,
        //                   &missShaderBindingTableRegion,
        //                   &hitShaderBindingTableRegion,
        //                   &callableShaderBindingTableRegion,
        //                   m_swapchainExtent.width, m_swapchainExtent.height, 1);
        
        // 目前回退到软件实现
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_loadingCubesPipeline);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    } else {
        // 使用传统的图形管线（软件ray casting）
        // 
        // 软件光追性能问题分析：
        // 1. 每个像素都要遍历所有立方体（8x8=64个）
        // 2. 计算复杂度：像素数 × 立方体数 × 抗锯齿采样数
        //    例如：1920x1080 × 64 × 9 = 1,194,393,600次计算/帧
        // 3. 在fragment shader中执行，受限于shader执行单元
        // 4. 没有空间加速结构（BVH），必须线性遍历
        // 5. 复杂的分支和循环降低GPU并行效率
        //
        // 硬件光追的优势：
        // - 使用BVH树，复杂度从O(n)降到O(log n)
        // - 专用RT Core硬件，比通用shader快得多
        // - 自动处理并行射线追踪
        // - 支持动态场景更新
        //
        // 绘制全屏三角形（6个顶点，2个三角形）
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    }
    
    // 渲染帧率文本（左上角）
    if (textRenderer && fps > 0.0f) {
        char fpsText[32];
        sprintf_s(fpsText, "FPS: %.1f", fps);
        
        // 计算左上角位置（在视口坐标系中）
        float textX = 10.0f;  // 距离左边10像素
        float textY = 10.0f;  // 距离顶部10像素
        
        // 根据不同的stretch mode调整文本位置
        if (useLoadingCubes) {
            // loading_cubes模式：使用全屏坐标系
            textRenderer->RenderText(commandBuffer, fpsText, textX, textY, 
                                    (float)m_swapchainExtent.width, (float)m_swapchainExtent.height,
                                    1.0f, 1.0f, 0.0f, 1.0f);  // 黄色文本
        } else {
            // 其他模式：需要考虑视口偏移
            switch (m_stretchMode) {
            case StretchMode::Disabled: {
                // Disabled模式：视口居中，需要加上偏移
                float offsetX = ((float)m_swapchainExtent.width - baseWidth) * 0.5f;
                float offsetY = ((float)m_swapchainExtent.height - baseHeight) * 0.5f;
                textRenderer->RenderText(commandBuffer, fpsText, textX + offsetX, textY + offsetY,
                                        (float)m_swapchainExtent.width, (float)m_swapchainExtent.height,
                                        1.0f, 1.0f, 0.0f, 1.0f);  // 黄色文本
                break;
            }
            case StretchMode::Scaled: {
                // Scaled模式：视口居中，需要加上偏移
                float offsetX = ((float)m_swapchainExtent.width - baseWidth) * 0.5f;
                float offsetY = ((float)m_swapchainExtent.height - baseHeight) * 0.5f;
                textRenderer->RenderText(commandBuffer, fpsText, textX + offsetX, textY + offsetY,
                                        (float)m_swapchainExtent.width, (float)m_swapchainExtent.height,
                                        1.0f, 1.0f, 0.0f, 1.0f);  // 黄色文本
                break;
            }
            case StretchMode::Fit: {
                // Fit模式：视口可能居中，需要加上偏移
                const float currentAspect = (float)m_swapchainExtent.width / (float)m_swapchainExtent.height;
                float offsetX = 0.0f, offsetY = 0.0f;
                if (currentAspect > targetAspect) {
                    // 窗口更宽 - 添加左右黑边
                    float viewportHeight = (float)m_swapchainExtent.height;
                    float viewportWidth = viewportHeight * targetAspect;
                    offsetX = ((float)m_swapchainExtent.width - viewportWidth) * 0.5f;
                } else {
                    // 窗口更高 - 添加上下黑边
                    float viewportWidth = (float)m_swapchainExtent.width;
                    float viewportHeight = viewportWidth / targetAspect;
                    offsetY = ((float)m_swapchainExtent.height - viewportHeight) * 0.5f;
                }
                textRenderer->RenderText(commandBuffer, fpsText, textX + offsetX, textY + offsetY,
                                        (float)m_swapchainExtent.width, (float)m_swapchainExtent.height,
                                        1.0f, 1.0f, 0.0f, 1.0f);  // 黄色文本
                break;
            }
            }
        }
    }
    
    vkCmdEndRenderPass(commandBuffer);
    
    result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to record command buffer!");
    }
}

bool VulkanRenderer::DrawFrame(float time, bool useLoadingCubes, TextRenderer* textRenderer, float fps) {
    VkResult result = vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to wait for fences!");
        return false;
    }
    
    uint32_t imageIndex;
    result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapchain();
        // 更新swapchain extent
        RECT clientRect;
        GetClientRect(m_hwnd, &clientRect);
        m_swapchainExtent.width = clientRect.right - clientRect.left;
        m_swapchainExtent.height = clientRect.bottom - clientRect.top;
        return false;
    } else if (result != VK_SUCCESS) {
        Window::ShowError("Failed to acquire swap chain image!");
        return false;
    }
    
    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
    
    vkResetCommandBuffer(m_commandBuffers[imageIndex], 0);
    RecordCommandBuffer(m_commandBuffers[imageIndex], imageIndex, time, useLoadingCubes, textRenderer, fps);
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];
    
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to submit draw command buffer!");
        return false;
    }
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {m_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapchain();
    } else if (result != VK_SUCCESS) {
        Window::ShowError("Failed to present swap chain image!");
        return false;
    }
    
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    
    return true;
}

bool VulkanRenderer::DrawFrameWithLoading(float time, LoadingAnimation* loadingAnim,
                                          Button* button,
                                          TextRenderer* textRenderer,
                                          Button* colorButton,
                                          Button* leftButton,
                                          const std::vector<Button*>* additionalButtons,
                                          Slider* slider,
                                          const std::vector<Slider*>* additionalSliders,
                                          float fps) {
    VkResult result = vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to wait for fences!");
        return false;
    }
    
    uint32_t imageIndex;
    result = vkAcquireNextImageKHR(m_device, m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapchain();
        // 更新swapchain extent
        RECT clientRect;
        GetClientRect(m_hwnd, &clientRect);
        m_swapchainExtent.width = clientRect.right - clientRect.left;
        m_swapchainExtent.height = clientRect.bottom - clientRect.top;
        return false;
    } else if (result != VK_SUCCESS) {
        Window::ShowError("Failed to acquire swap chain image!");
        return false;
    }
    
    vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);
    
    // 记录命令缓冲区（只渲染加载动画）
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    result = vkBeginCommandBuffer(m_commandBuffers[imageIndex], &beginInfo);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to begin recording command buffer!");
        return false;
    }
    
    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = m_swapchainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_swapchainExtent;
    
    // 深灰色背景
    VkClearValue clearColor = {{{0.2f, 0.2f, 0.2f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;
    
    vkCmdBeginRenderPass(m_commandBuffers[imageIndex], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // 根据不同的拉伸模式设置视口和坐标系
    // UI基准使用背景纹理的原始尺寸（唯一的耦合点）
    float baseWidth = (float)WINDOW_WIDTH;   // 默认宽度 800（如果没有背景）
    float baseHeight = (float)WINDOW_HEIGHT; // 默认高度 800（如果没有背景）
    if (m_backgroundTextureWidth > 0 && m_backgroundTextureHeight > 0) {
        // 使用背景纹理的原始尺寸作为UI基准
        baseWidth = (float)m_backgroundTextureWidth;
        baseHeight = (float)m_backgroundTextureHeight;
    }
    const float targetAspect = baseWidth / baseHeight;
    const float currentAspect = (float)m_swapchainExtent.width / (float)m_swapchainExtent.height;
    
    VkViewport viewport = {};
    VkRect2D scissor = {};
    VkExtent2D uiExtent = {};  // UI使用的坐标系尺寸（基于背景纹理大小）
    
    switch (m_stretchMode) {
        case StretchMode::Scaled: {
            // Canvas Items 模式：和example一致，UI使用实际窗口大小，通过UpdateForWindowResize更新
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)m_swapchainExtent.width;
            viewport.height = (float)m_swapchainExtent.height;
            
            scissor.offset = {0, 0};
            scissor.extent = m_swapchainExtent;
            
            // UI使用实际窗口大小（和example一致）
            uiExtent = m_swapchainExtent;
            break;
        }
        case StretchMode::Fit: {
            // Fit模式：在窗口内建立保持背景纹理比例的最大NDC坐标系
            // NDC坐标系保持比例重建，不整个窗口重建
    if (currentAspect > targetAspect) {
                // 窗口更宽 - 添加左右黑边（pillarbox）
        float viewportHeight = (float)m_swapchainExtent.height;
        float viewportWidth = viewportHeight * targetAspect;
        float offsetX = ((float)m_swapchainExtent.width - viewportWidth) * 0.5f;
        
        viewport.x = offsetX;
        viewport.y = 0.0f;
        viewport.width = viewportWidth;
        viewport.height = viewportHeight;
        
        scissor.offset.x = (int32_t)offsetX;
        scissor.offset.y = 0;
        scissor.extent.width = (uint32_t)viewportWidth;
        scissor.extent.height = (uint32_t)viewportHeight;
    } else {
                // 窗口更高或相等 - 添加上下黑边（letterbox）
        float viewportWidth = (float)m_swapchainExtent.width;
        float viewportHeight = viewportWidth / targetAspect;
        float offsetY = ((float)m_swapchainExtent.height - viewportHeight) * 0.5f;
        
        viewport.x = 0.0f;
        viewport.y = offsetY;
        viewport.width = viewportWidth;
        viewport.height = viewportHeight;
        
        scissor.offset.x = 0;
        scissor.offset.y = (int32_t)offsetY;
        scissor.extent.width = (uint32_t)viewportWidth;
        scissor.extent.height = (uint32_t)viewportHeight;
            }
            
            // FIT模式：UI使用背景纹理大小作为坐标系基准
            uiExtent.width = (uint32_t)baseWidth;
            uiExtent.height = (uint32_t)baseHeight;
            break;
        }
        case StretchMode::Disabled: {
            // Disabled模式：保持固定的背景纹理大小，UI完全和窗口无关
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = baseWidth;   // 背景纹理宽度
            viewport.height = baseHeight; // 背景纹理高度
            
            scissor.offset = {0, 0};
            scissor.extent.width = (uint32_t)baseWidth;
            scissor.extent.height = (uint32_t)baseHeight;
            
            // UI使用背景纹理大小作为坐标系基准
            uiExtent.width = (uint32_t)baseWidth;
            uiExtent.height = (uint32_t)baseHeight;
            break;
        }
        default: {
            // 其他模式：使用实际窗口大小
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float)m_swapchainExtent.width;
            viewport.height = (float)m_swapchainExtent.height;
            
            scissor.offset = {0, 0};
            scissor.extent = m_swapchainExtent;
            
            uiExtent = m_swapchainExtent;
            break;
        }
    }
    
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    // 先绘制背景纹理（在UI元素之前）
    // 背景使用独立的viewport和scissor（全屏），完全独立于UI模式
    if (HasBackgroundTexture()) {
        // 背景使用全屏viewport和scissor，不受UI拉伸模式影响
        VkViewport bgViewport = {};
        bgViewport.x = 0.0f;
        bgViewport.y = 0.0f;
        bgViewport.width = (float)m_swapchainExtent.width;
        bgViewport.height = (float)m_swapchainExtent.height;
        bgViewport.minDepth = 0.0f;
        bgViewport.maxDepth = 1.0f;
        
        VkRect2D bgScissor = {};
        bgScissor.offset = {0, 0};
        bgScissor.extent = m_swapchainExtent;
        
        vkCmdSetViewport(m_commandBuffers[imageIndex], 0, 1, &bgViewport);
        vkCmdSetScissor(m_commandBuffers[imageIndex], 0, 1, &bgScissor);
        
        RenderBackgroundTexture(m_commandBuffers[imageIndex], m_swapchainExtent);
    }
    
    // 设置UI的viewport和scissor（独立于背景）
    vkCmdSetViewport(m_commandBuffers[imageIndex], 0, 1, &viewport);
    vkCmdSetScissor(m_commandBuffers[imageIndex], 0, 1, &scissor);
    
    // 渲染加载动画（方块动画）
    // UI使用固定的坐标系尺寸（uiExtent），这样UI位置始终正确
    if (loadingAnim) {
        loadingAnim->Render(m_commandBuffers[imageIndex], uiExtent);
    }
    
    // 收集所有按钮并按zIndex排序（数值越大越在上层）
    std::vector<Button*> buttons;
    if (button) buttons.push_back(button);
    if (colorButton) buttons.push_back(colorButton);
    if (leftButton) buttons.push_back(leftButton);
    // 添加额外的按钮（如果有）
    if (additionalButtons) {
        for (Button* btn : *additionalButtons) {
            if (btn) buttons.push_back(btn);
        }
    }
    
    // 按zIndex排序（升序，先渲染低层级的按钮，后渲染高层级的按钮）
    std::sort(buttons.begin(), buttons.end(), 
              [](Button* a, Button* b) {
                  return a->GetZIndex() < b->GetZIndex();
              });
    
    // 为文本渲染收集所有可见按钮
    // 文本统一在最后渲染一遍，按降序排序（高层级先）
    // 这样高层级文本会最后渲染，显示在最上层
    std::vector<Button*> textButtons;
    for (Button* btn : buttons) {
        if (btn->IsVisible()) {
            textButtons.push_back(btn);
        }
    }
    // 按降序排序（高层级先），这样在渲染时高层级文本会最后渲染，显示在最上层
    std::sort(textButtons.begin(), textButtons.end(),
              [](Button* a, Button* b) {
                  return a->GetZIndex() > b->GetZIndex();
              });
    
    // 根据不同的拉伸模式设置按钮渲染的 viewport
    if (m_stretchMode == StretchMode::Fit || m_stretchMode == StretchMode::Disabled) {
        // Fit模式：使用与上面相同的 viewport 和 scissor
        VkViewport buttonViewport = viewport;
        VkRect2D buttonScissor = scissor;
        
        // Set viewport and scissor BEFORE calling Render
        vkCmdSetViewport(m_commandBuffers[imageIndex], 0, 1, &buttonViewport);
        vkCmdSetScissor(m_commandBuffers[imageIndex], 0, 1, &buttonScissor);
        
        // 按层级顺序渲染所有按钮（先渲染按钮本身）
        for (Button* btn : buttons) {
            btn->Render(m_commandBuffers[imageIndex], uiExtent);
        }
        
        // 渲染滑块（如果有）
        if (slider) {
            static int sliderDebugCount = 0;
            if (sliderDebugCount % 60 == 0) {
                printf("[DRAWFRAME] Fit mode: Rendering slider: visible=%s, uiExtent=(%u, %u)\n",
                       slider->IsVisible() ? "true" : "false", uiExtent.width, uiExtent.height);
            }
            sliderDebugCount++;
            if (slider->IsVisible()) {
                slider->Render(m_commandBuffers[imageIndex], uiExtent);
            }
        }
        // 渲染额外的滑块（如果有）
        if (additionalSliders) {
            for (Slider* sld : *additionalSliders) {
                if (sld && sld->IsVisible()) {
                    sld->Render(m_commandBuffers[imageIndex], uiExtent);
                }
            }
        }
    
        // 统一渲染所有按钮的文本（在所有按钮之后，只渲染一遍）
        // 使用批量渲染API，避免文本相互覆盖
        // 注意：由于Button类没有暴露文本相关的getter方法，这里暂时保留原有的逐个渲染方式
        // 但使用批量渲染API来避免覆盖问题
        if (textRenderer) {
            textRenderer->BeginTextBatch();
            
            VkViewport textViewport = {};
            textViewport.x = 0.0f;
            textViewport.y = 0.0f;
            textViewport.width = (float)m_swapchainExtent.width;
            textViewport.height = (float)m_swapchainExtent.height;
            textViewport.minDepth = 0.0f;
            textViewport.maxDepth = 1.0f;
            
            VkRect2D textScissor = {};
            textScissor.offset = {0, 0};
            textScissor.extent = m_swapchainExtent;
            
            // 使用Button的RenderText方法，但使用批量渲染模式
            // 注意：需要修改Button::RenderText以支持批量模式，或者直接在这里实现文本收集逻辑
            // 为了快速修复bug，暂时保留原有逻辑，但确保所有文本都使用批量渲染
            for (Button* btn : textButtons) {
                // 暂时使用原有方法，但会在TextRenderer中累积顶点
                btn->RenderText(m_commandBuffers[imageIndex], uiExtent, &buttonViewport, &textScissor);
            }
            
            // 添加FPS文本到批次
            if (fps > 0.0f) {
                char fpsText[32];
                sprintf_s(fpsText, "FPS: %.1f", fps);
                
                float textX = 10.0f;
                float textY = 10.0f;
                float offsetX = 0.0f, offsetY = 0.0f;
                
                // Fit模式：需要考虑视口偏移
                if (currentAspect > targetAspect) {
                    float viewportHeight = (float)m_swapchainExtent.height;
                    float viewportWidth = viewportHeight * targetAspect;
                    offsetX = ((float)m_swapchainExtent.width - viewportWidth) * 0.5f;
                } else {
                    float viewportWidth = (float)m_swapchainExtent.width;
                    float viewportHeight = viewportWidth / targetAspect;
                    offsetY = ((float)m_swapchainExtent.height - viewportHeight) * 0.5f;
                }
                
                float flippedY = (float)m_swapchainExtent.height - (textY + offsetY);
                textRenderer->AddTextToBatch(fpsText, textX + offsetX, flippedY,
                                            1.0f, 1.0f, 0.0f, 1.0f);
            }
            
            // 一次性渲染所有累积的文本（包括按钮文本和FPS文本）
            // Fit模式：完全参考Button::RenderText的做法
            // Button::RenderText使用窗口大小作为screenSize，坐标已经是窗口坐标（已转换）
            // 但字符大小（像素值）需要根据uiToViewportScale缩放
            float textScreenWidth = (float)m_swapchainExtent.width;
            float textScreenHeight = (float)m_swapchainExtent.height;
            
            // 计算UI到Viewport的缩放比例（和Button::RenderText中的uiToViewportScale一致）
            float uiToViewportScaleX = buttonViewport.width / (float)uiExtent.width;
            float uiToViewportScaleY = buttonViewport.height / (float)uiExtent.height;
            
            textRenderer->EndTextBatch(m_commandBuffers[imageIndex], 
                                      textScreenWidth, 
                                      textScreenHeight,
                                      0.0f, 0.0f,  // viewport偏移为0（使用全屏）
                                      uiToViewportScaleX,  // 字符大小需要按这个比例缩放
                                      uiToViewportScaleY); // 和Button坐标转换的比例一致
        }
    } else {
        // Scaled 或其他模式：使用全屏 viewport（和example一致）
        // Scaled模式下，UI使用实际窗口大小，不需要坐标转换
        VkViewport buttonViewport = {};
        buttonViewport.x = 0.0f;
        buttonViewport.y = 0.0f;
        buttonViewport.width = (float)m_swapchainExtent.width;
        buttonViewport.height = (float)m_swapchainExtent.height;
        buttonViewport.minDepth = 0.0f;
        buttonViewport.maxDepth = 1.0f;
        
        VkRect2D buttonScissor = {};
        buttonScissor.offset = {0, 0};
        buttonScissor.extent = m_swapchainExtent;
        
        vkCmdSetViewport(m_commandBuffers[imageIndex], 0, 1, &buttonViewport);
        vkCmdSetScissor(m_commandBuffers[imageIndex], 0, 1, &buttonScissor);
        
        // 按层级顺序渲染所有按钮（先渲染按钮本身，使用实际窗口大小）
        for (Button* btn : buttons) {
            btn->Render(m_commandBuffers[imageIndex], uiExtent);
        }
        
        // 渲染滑块（如果有）
        if (slider) {
            static int sliderDebugCount2 = 0;
            if (sliderDebugCount2 % 60 == 0) {
                printf("[DRAWFRAME] Scaled mode: Rendering slider: visible=%s, uiExtent=(%u, %u)\n",
                       slider->IsVisible() ? "true" : "false", uiExtent.width, uiExtent.height);
            }
            sliderDebugCount2++;
            if (slider->IsVisible()) {
                slider->Render(m_commandBuffers[imageIndex], uiExtent);
            }
        }
        // 渲染额外的滑块（如果有）
        if (additionalSliders) {
            for (Slider* sld : *additionalSliders) {
                if (sld && sld->IsVisible()) {
                    sld->Render(m_commandBuffers[imageIndex], uiExtent);
                }
            }
        }
        
        // 统一渲染所有按钮的文本（在所有按钮之后，只渲染一遍）
        // 使用批量渲染API，避免文本相互覆盖
        if (textRenderer) {
            textRenderer->BeginTextBatch();
            
            // 使用Button的RenderText方法，但会在TextRenderer中累积顶点
            for (Button* btn : textButtons) {
                btn->RenderText(m_commandBuffers[imageIndex], uiExtent);
            }
            
            // 添加FPS文本到批次
            if (fps > 0.0f) {
                char fpsText[32];
                sprintf_s(fpsText, "FPS: %.1f", fps);
                float textX = 10.0f;
                float textY = 10.0f;
                float flippedY = (float)m_swapchainExtent.height - textY;
                textRenderer->AddTextToBatch(fpsText, textX, flippedY,
                                            1.0f, 1.0f, 0.0f, 1.0f);
            }
            
            // 一次性渲染所有累积的文本（包括按钮文本和FPS文本）
            // Scaled模式：使用整个窗口大小，viewport无偏移
            textRenderer->EndTextBatch(m_commandBuffers[imageIndex], 
                                      (float)m_swapchainExtent.width, 
                                      (float)m_swapchainExtent.height,
                                      0.0f, 0.0f);
        }
    }
    
    // FPS文本已经在批量渲染批次中处理，这里不再单独渲染
    
    vkCmdEndRenderPass(m_commandBuffers[imageIndex]);
    
    result = vkEndCommandBuffer(m_commandBuffers[imageIndex]);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to record command buffer!");
        return false;
    }
    
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {m_imageAvailableSemaphores[m_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_commandBuffers[imageIndex];
    
    VkSemaphore signalSemaphores[] = {m_renderFinishedSemaphores[m_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    result = vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]);
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to submit draw command buffer!");
        return false;
    }
    
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {m_swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(m_presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        RecreateSwapchain();
        return false;
    } else if (result != VK_SUCCESS) {
        Window::ShowError("Failed to present swap chain image!");
        return false;
    }
    
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    
    return true;
}

// 背景纹理实现
// 使用Button类来绘制全屏背景（简化实现）
static std::unique_ptr<Button> s_backgroundButton = nullptr;

bool VulkanRenderer::LoadBackgroundTexture(const std::string& filepath) {
    printf("[BACKGROUND] Loading background texture: %s\n", filepath.c_str());
    
    CleanupBackgroundTexture();
    
    // 先加载图像数据获取原始尺寸
    renderer::image::ImageData imageData = renderer::image::ImageLoader::LoadImage(filepath);
    if (imageData.width == 0 || imageData.height == 0) {
        printf("[BACKGROUND] ERROR: Failed to load image data from %s\n", filepath.c_str());
        return false;
    }
    
    // 存储背景纹理原始尺寸（用于计算宽高比）
    m_backgroundTextureWidth = imageData.width;
    m_backgroundTextureHeight = imageData.height;
    float textureAspect = (float)m_backgroundTextureWidth / (float)m_backgroundTextureHeight;
    
    printf("[BACKGROUND] Background texture dimensions: %dx%d, aspect=%.3f\n", 
           m_backgroundTextureWidth, m_backgroundTextureHeight, textureAspect);
    
    // 使用Button来绘制全屏背景（简化实现）
    s_backgroundButton = std::make_unique<Button>();
    
    // 获取窗口尺寸
    RECT clientRect;
    GetClientRect(m_hwnd, &clientRect);
    float windowWidth = (float)(clientRect.right - clientRect.left);
    float windowHeight = (float)(clientRect.bottom - clientRect.top);
    float windowAspect = windowWidth / windowHeight;
    
    // 根据背景模式计算背景按钮的大小（保持纹理宽高比）
    float bgWidth, bgHeight;
    if (m_backgroundStretchMode == BackgroundStretchMode::Fit) {
        // Fit模式：保持比例，完全在窗口内的最大大小
        if (windowAspect > textureAspect) {
            // 窗口更宽，以高度为准
            bgHeight = windowHeight;
            bgWidth = bgHeight * textureAspect;
        } else {
            // 窗口更高，以宽度为准
            bgWidth = windowWidth;
            bgHeight = bgWidth / textureAspect;
        }
    } else {
        // Scaled模式：保持比例，覆盖整个窗口的最小大小（没有空隙）
        if (windowAspect > textureAspect) {
            // 窗口更宽，以宽度为准（覆盖整个宽度）
            bgWidth = windowWidth;
            bgHeight = bgWidth / textureAspect;
        } else {
            // 窗口更高，以高度为准（覆盖整个高度）
            bgHeight = windowHeight;
            bgWidth = bgHeight * textureAspect;
        }
    }
    
    // 背景使用独立的坐标系（始终使用实际窗口大小，不受UI拉伸模式影响）
    VkExtent2D bgExtent = m_swapchainExtent;
    
    // 创建背景按钮配置
    // 使用相对位置(0.5, 0.5)，大小根据背景模式计算（基于实际窗口大小）
    ButtonConfig bgConfig = ButtonConfig::CreateRelativeWithTexture(
        0.5f, 0.5f,  // 相对位置(0.5, 0.5) = 屏幕中心
        bgWidth, bgHeight,  // 根据背景模式计算的大小（基于实际窗口大小）
        filepath
    );
    bgConfig.zIndex = 0;  // zIndex = 0（最底层）
    
    if (s_backgroundButton->Initialize(
            m_device,
            m_physicalDevice,
            m_commandPool,
            m_graphicsQueue,
            m_renderPass,
            bgExtent,  // 背景使用实际窗口大小，独立于UI坐标系
            bgConfig,
            nullptr,
            false)) {  // 使用传统渲染方式，支持纹理
        // 背景始终响应窗口变化（不受UI拉伸模式影响）
        // 不设置SetFixedScreenSize，让背景按钮独立管理自己的行为
        
        if (s_backgroundButton->HasTexture()) {
            printf("[BACKGROUND] Background texture loaded successfully, size=%.0fx%.0f\n", bgWidth, bgHeight);
        } else {
            printf("[BACKGROUND] WARNING: Background button initialized but has no texture!\n");
        }
        return true;
    } else {
        printf("[BACKGROUND] ERROR: Failed to initialize background button\n");
        s_backgroundButton.reset();
        return false;
    }
}

void VulkanRenderer::CleanupBackgroundTexture() {
    if (s_backgroundButton) {
        s_backgroundButton.reset();
        printf("[BACKGROUND] Background texture cleaned up\n");
    }
}

void VulkanRenderer::RenderBackgroundTexture(VkCommandBuffer commandBuffer, VkExtent2D extent) {
    if (!s_backgroundButton) {
        printf("[DEBUG] RenderBackgroundTexture: s_backgroundButton is null\n");
        return;
    }
    
    if (!s_backgroundButton->HasTexture() || m_backgroundTextureWidth == 0 || m_backgroundTextureHeight == 0) {
        printf("[DEBUG] RenderBackgroundTexture: Button has no texture or invalid dimensions\n");
        return;
    }
    
    // 获取窗口尺寸
    float windowWidth = (float)extent.width;
    float windowHeight = (float)extent.height;
    float windowAspect = windowWidth / windowHeight;
    float textureAspect = (float)m_backgroundTextureWidth / (float)m_backgroundTextureHeight;
    
    // 根据背景模式重新计算背景按钮的大小（保持纹理宽高比）
    float bgWidth, bgHeight;
    if (m_backgroundStretchMode == BackgroundStretchMode::Fit) {
        // Fit模式：保持比例，完全在窗口内的最大大小
        if (windowAspect > textureAspect) {
            // 窗口更宽，以高度为准
            bgHeight = windowHeight;
            bgWidth = bgHeight * textureAspect;
        } else {
            // 窗口更高，以宽度为准
            bgWidth = windowWidth;
            bgHeight = bgWidth / textureAspect;
        }
    } else {
        // Scaled模式：保持比例，覆盖整个窗口的最小大小（没有空隙）
        if (windowAspect > textureAspect) {
            // 窗口更宽，以宽度为准（覆盖整个宽度）
            bgWidth = windowWidth;
            bgHeight = bgWidth / textureAspect;
        } else {
            // 窗口更高，以高度为准（覆盖整个高度）
            bgHeight = windowHeight;
            bgWidth = bgHeight * textureAspect;
        }
    }
    
    // 背景使用独立的坐标系（始终使用实际窗口大小，完全独立于UI模式）
    VkExtent2D bgExtent = extent;
    
    // 更新背景按钮大小和位置（完全独立于UI模式，只根据背景模式计算）
    s_backgroundButton->SetSize(bgWidth, bgHeight);
    s_backgroundButton->UpdateForWindowResize(windowWidth, windowHeight);
    
    // 绘制背景（使用背景自己的坐标系，独立于UI坐标系）
    s_backgroundButton->Render(commandBuffer, bgExtent);
}

bool VulkanRenderer::HasBackgroundTexture() const {
    if (!s_backgroundButton) {
        return false;
    }
    return s_backgroundButton->HasTexture();
}

VkExtent2D VulkanRenderer::GetUIBaseSize() const {
    VkExtent2D baseSize = {};
    // UI基准使用背景纹理的原始尺寸（唯一的耦合点）
    if (m_backgroundTextureWidth > 0 && m_backgroundTextureHeight > 0) {
        // 使用背景纹理的原始尺寸作为UI基准
        baseSize.width = m_backgroundTextureWidth;
        baseSize.height = m_backgroundTextureHeight;
    } else {
        // 如果没有背景，使用默认的800x800
        baseSize.width = WINDOW_WIDTH;
        baseSize.height = WINDOW_HEIGHT;
    }
    return baseSize;
}

void VulkanRenderer::SetMouseInput(float deltaX, float deltaY, bool buttonDown) {
    // 累积鼠标增量（每帧重置）
    m_mouseDeltaX += deltaX;
    m_mouseDeltaY += deltaY;
    m_mouseButtonDown = buttonDown;
}

void VulkanRenderer::SetKeyInput(bool w, bool a, bool s, bool d) {
    m_keyW = w;
    m_keyA = a;
    m_keyS = s;
    m_keyD = d;
}

void VulkanRenderer::UpdateCamera(float deltaTime) {
    const float rotationSensitivity = 0.005f;
    const float moveSpeed = 2.0f;
    const float maxPitch = 1.57f; // PI/2
    
    // 更新相机旋转（基于鼠标输入）
    if (m_mouseButtonDown) {
        m_cameraYaw += m_mouseDeltaX * rotationSensitivity;
        m_cameraPitch -= m_mouseDeltaY * rotationSensitivity; // 注意取反
        
        // 限制pitch角度
        if (m_cameraPitch > maxPitch) m_cameraPitch = maxPitch;
        if (m_cameraPitch < -maxPitch) m_cameraPitch = -maxPitch;
    }
    
    // 更新相机位置（基于WASD输入）
    float cosYaw = cosf(m_cameraYaw);
    float sinYaw = sinf(m_cameraYaw);
    float cosPitch = cosf(m_cameraPitch);
    float sinPitch = sinf(m_cameraPitch);
    
    // 前方向量（相机朝向）
    float forwardX = sinYaw * cosPitch;
    float forwardY = -sinPitch;
    float forwardZ = -cosYaw * cosPitch;
    
    // 右方向量（相机右侧）
    float rightX = cosYaw;
    float rightY = 0.0f;
    float rightZ = sinYaw;
    
    // 计算移动距离
    float moveDistance = moveSpeed * deltaTime;
    
    // 根据按键更新位置
    if (m_keyW) {
        m_cameraPosX += forwardX * moveDistance;
        m_cameraPosY += forwardY * moveDistance;
        m_cameraPosZ += forwardZ * moveDistance;
    }
    if (m_keyS) {
        m_cameraPosX -= forwardX * moveDistance;
        m_cameraPosY -= forwardY * moveDistance;
        m_cameraPosZ -= forwardZ * moveDistance;
    }
    if (m_keyA) {
        m_cameraPosX -= rightX * moveDistance;
        m_cameraPosY -= rightY * moveDistance;
        m_cameraPosZ -= rightZ * moveDistance;
    }
    if (m_keyD) {
        m_cameraPosX += rightX * moveDistance;
        m_cameraPosY += rightY * moveDistance;
        m_cameraPosZ += rightZ * moveDistance;
    }
    
    // 重置输入增量（准备下一帧）
    m_mouseDeltaX = 0.0f;
    m_mouseDeltaY = 0.0f;
}

void VulkanRenderer::ResetCamera() {
    m_cameraYaw = 0.0f;
    m_cameraPitch = 0.0f;
    m_cameraPosX = 0.0f;
    m_cameraPosY = 0.0f;
    m_cameraPosZ = 2.2f;
    m_mouseDeltaX = 0.0f;
    m_mouseDeltaY = 0.0f;
    m_mouseButtonDown = false;
    m_keyW = false;
    m_keyA = false;
    m_keyS = false;
    m_keyD = false;
}

bool VulkanRenderer::CheckRayTracingSupport() {
    if (m_instance == VK_NULL_HANDLE || m_physicalDevice == VK_NULL_HANDLE) {
        return false;
    }
    
    // 检查设备扩展支持
    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(m_physicalDevice, nullptr, &extensionCount, availableExtensions.data());
    
    // 需要的扩展列表
    const char* requiredExtensions[] = {
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME
    };
    
    // 检查所有必需的扩展是否可用
    for (const char* extName : requiredExtensions) {
        bool found = false;
        for (const auto& ext : availableExtensions) {
            if (strcmp(ext.extensionName, extName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            printf("[RAYTRACING] Required extension not found: %s\n", extName);
            return false;
        }
    }
    
    // 检查物理设备特性
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties = {};
    rayTracingProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
    
    VkPhysicalDeviceProperties2 deviceProperties2 = {};
    deviceProperties2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties2.pNext = &rayTracingProperties;
    vkGetPhysicalDeviceProperties2(m_physicalDevice, &deviceProperties2);
    
    printf("[RAYTRACING] Ray tracing supported! Max recursion depth: %u\n", 
           rayTracingProperties.maxRayRecursionDepth);
    
    return true;
}

bool VulkanRenderer::CreateRayTracingPipeline() {
    if (!m_rayTracingSupported) {
        printf("[RAYTRACING] Ray tracing not supported, cannot create pipeline\n");
        return false;
    }
    
    // 注意：完整的硬件光线追踪实现需要：
    // 1. Ray tracing shaders (raygen, closest hit, miss)
    // 2. Shader binding table (SBT)
    // 3. Acceleration structures (BLAS和TLAS)
    // 4. Ray tracing pipeline
    
    // 由于实现复杂度较高，这里先创建基本框架
    // 实际使用时需要：
    // - 将立方体数据转换为几何体
    // - 构建BLAS（Bottom Level Acceleration Structure）
    // - 构建TLAS（Top Level Acceleration Structure）
    // - 创建raygen/closest hit/miss shaders
    // - 创建shader binding table
    // - 使用vkCmdTraceRaysKHR进行渲染
    
    printf("[RAYTRACING] Ray tracing pipeline framework ready\n");
    printf("[RAYTRACING] Full implementation requires:\n");
    printf("[RAYTRACING]   - Ray tracing shaders (raygen/closest hit/miss)\n");
    printf("[RAYTRACING]   - Acceleration structures (BLAS/TLAS)\n");
    printf("[RAYTRACING]   - Shader binding table\n");
    printf("[RAYTRACING]   - Ray tracing pipeline creation\n");
    printf("[RAYTRACING] Currently using software ray casting as fallback\n");
    
    // 标记为未完全实现，但框架已就绪
    // 在实际项目中，可以逐步完善这些组件
    return false;  // 返回false表示未完全实现，会使用软件fallback
}

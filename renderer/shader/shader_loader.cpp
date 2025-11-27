#include "shader/shader_loader.h"  // 1. 对应头文件

#include <cstring>  // 2. 系统头文件
#include <fstream>  // 2. 系统头文件
#include <string>   // 2. 系统头文件

#include <vulkan/vulkan.h>  // 3. 第三方库头文件

// Shaderc库包含（如果可用）
#ifdef USE_SHADERC
#include <shaderc/shaderc.hpp>  // 3. 第三方库头文件
#endif

// 注意：直接包含window/window.h是因为需要使用Window::ShowError静态方法
// 根据开发标准第15.1节，应优先使用接口或前向声明，但静态方法需要完整定义
// 未来可考虑创建IErrorHandler接口以符合依赖注入原则
#include "window/window.h"  // 4. 项目头文件

namespace renderer {
namespace shader {

std::vector<char> ShaderLoader::LoadSPIRV(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    
    if (!file.is_open()) {
        Window::ShowError("Failed to open shader file: " + filename);
        return std::vector<char>();
    }
    
    size_t fileSize = (size_t)file.tellg();
    if (fileSize == 0) {
        Window::ShowError("Shader file is empty: " + filename);
        return std::vector<char>();
    }
    
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    
    // 验证SPIR-V格式
    if (!ValidateSPIRV(buffer)) {
        Window::ShowError("Invalid SPIR-V file: " + filename);
        return std::vector<char>();
    }
    
    return buffer;
}

ShaderModuleHandle ShaderLoader::CreateShaderModuleFromSPIRV(DeviceHandle device, const std::vector<char>& spirvCode) {
    if (spirvCode.empty()) {
        Window::ShowError("Cannot create shader module: SPIR-V code is empty");
        return nullptr;
    }
    
    if (!ValidateSPIRV(spirvCode)) {
        Window::ShowError("Cannot create shader module: Invalid SPIR-V format");
        return nullptr;
    }
    
    // 将抽象句柄转换为Vulkan类型
    VkDevice vkDevice = static_cast<VkDevice>(device);
    
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(spirvCode.data());
    
    VkShaderModule vkShaderModule = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(vkDevice, &createInfo, nullptr, &vkShaderModule);
    
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create shader module");
        return nullptr;
    }
    
    // 返回抽象句柄
    return static_cast<ShaderModuleHandle>(vkShaderModule);
}

std::vector<char> ShaderLoader::CompileGLSLFromFile(const std::string& filename, ShaderStage stage) {
    // 读取GLSL源码
    std::ifstream file(filename);
    if (!file.is_open()) {
        Window::ShowError("Failed to open GLSL file: " + filename);
        return std::vector<char>();
    }
    
    std::string glslSource((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    return CompileGLSLFromSource(glslSource, stage, filename);
}

std::vector<char> ShaderLoader::CompileGLSLFromSource(const std::string& glslSource, ShaderStage stage, const std::string& filename) {
#ifdef USE_SHADERC
    // 使用Shaderc库进行运行时编译
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    
    // 设置编译选项
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    
    // 将抽象类型转换为shaderc的shader类型
    shaderc_shader_kind kind;
    switch (stage) {
        case ShaderStage::Vertex:
            kind = shaderc_vertex_shader;
            break;
        case ShaderStage::Fragment:
            kind = shaderc_fragment_shader;
            break;
        case ShaderStage::Compute:
            kind = shaderc_compute_shader;
            break;
        default:
            Window::ShowError("Unsupported shader stage for runtime compilation");
            return std::vector<char>();
    }
    
    // 编译GLSL为SPIR-V
    shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(
        glslSource.c_str(), glslSource.length(),
        kind,
        filename.empty() ? "shader.glsl" : filename.c_str(),
        "main",
        options
    );
    
    if (module.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::string errorMsg = "Shader compilation failed: " + std::string(module.GetErrorMessage());
        Window::ShowError(errorMsg);
        return std::vector<char>();
    }
    
    // 转换为vector<char>
    const std::vector<uint32_t>& spirv = {module.cbegin(), module.cend()};
    std::vector<char> result(spirv.size() * sizeof(uint32_t));
    std::memcpy(result.data(), spirv.data(), result.size());
    
    return result;
#else
    // 如果没有shaderc库，提示用户使用离线编译
    Window::ShowError(
        "Runtime GLSL compilation requires Shaderc library.\n"
        "Please either:\n"
        "1. Define USE_SHADERC and link against shaderc library, or\n"
        "2. Use glslangValidator to compile shaders offline to SPIR-V format."
    );
    return std::vector<char>();
#endif
}

ShaderModuleHandle ShaderLoader::CreateShaderModuleFromSource(DeviceHandle device, const std::string& glslSource, ShaderStage stage, const std::string& filename) {
    // 先编译GLSL为SPIR-V
    std::vector<char> spirvCode = CompileGLSLFromSource(glslSource, stage, filename);
    
    if (spirvCode.empty()) {
        return nullptr;
    }
    
    // 然后创建shader模块
    return CreateShaderModuleFromSPIRV(device, spirvCode);
}

bool ShaderLoader::ValidateSPIRV(const std::vector<char>& spirvCode) {
    if (spirvCode.size() < 5 * sizeof(uint32_t)) {
        return false; // 文件太小，不可能是有效的SPIR-V
    }
    
    // 检查SPIR-V魔数（前4字节）
    uint32_t magic = *reinterpret_cast<const uint32_t*>(spirvCode.data());
    return magic == SPIRV_MAGIC;
}

} // namespace shader
} // namespace renderer

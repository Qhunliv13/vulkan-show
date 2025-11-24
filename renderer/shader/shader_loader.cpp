#include "shader/shader_loader.h"
#include "window/window.h"
#include <fstream>
#include <cstring>

// Shaderc库包含（如果可用）
#ifdef USE_SHADERC
#include <shaderc/shaderc.hpp>
#endif
#include <string>

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

VkShaderModule ShaderLoader::CreateShaderModuleFromSPIRV(VkDevice device, const std::vector<char>& spirvCode) {
    if (spirvCode.empty()) {
        Window::ShowError("Cannot create shader module: SPIR-V code is empty");
        return VK_NULL_HANDLE;
    }
    
    if (!ValidateSPIRV(spirvCode)) {
        Window::ShowError("Cannot create shader module: Invalid SPIR-V format");
        return VK_NULL_HANDLE;
    }
    
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirvCode.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(spirvCode.data());
    
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    
    if (result != VK_SUCCESS) {
        Window::ShowError("Failed to create shader module");
        return VK_NULL_HANDLE;
    }
    
    return shaderModule;
}

std::vector<char> ShaderLoader::CompileGLSLFromFile(const std::string& filename, VkShaderStageFlagBits stage) {
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

std::vector<char> ShaderLoader::CompileGLSLFromSource(const std::string& glslSource, VkShaderStageFlagBits stage, const std::string& filename) {
#ifdef USE_SHADERC
    // 使用Shaderc库进行运行时编译
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
    
    // 设置编译选项
    options.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_0);
    options.SetOptimizationLevel(shaderc_optimization_level_performance);
    
    // 转换为shaderc的shader类型
    shaderc_shader_kind kind;
    switch (stage) {
        case VK_SHADER_STAGE_VERTEX_BIT:
            kind = shaderc_vertex_shader;
            break;
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            kind = shaderc_fragment_shader;
            break;
        case VK_SHADER_STAGE_COMPUTE_BIT:
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

VkShaderModule ShaderLoader::CreateShaderModuleFromSource(VkDevice device, const std::string& glslSource, VkShaderStageFlagBits stage, const std::string& filename) {
    // 先编译GLSL为SPIR-V
    std::vector<char> spirvCode = CompileGLSLFromSource(glslSource, stage, filename);
    
    if (spirvCode.empty()) {
        return VK_NULL_HANDLE;
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

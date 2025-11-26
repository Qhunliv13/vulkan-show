#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace renderer {
namespace shader {

class ShaderLoader {
public:
    // 从文件读取SPIR-V字节码
    static std::vector<char> LoadSPIRV(const std::string& filename);
    
    // 从GLSL源码文件编译并加载shader
    static std::vector<char> CompileGLSLFromFile(const std::string& filename, VkShaderStageFlagBits stage);
    
    // 从内存中的GLSL源码编译为SPIR-V
    static std::vector<char> CompileGLSLFromSource(const std::string& glslSource, VkShaderStageFlagBits stage, const std::string& filename = "");
    
    // 从SPIR-V字节码创建shader模块
    static VkShaderModule CreateShaderModuleFromSPIRV(VkDevice device, const std::vector<char>& spirvCode);
    
    // 从GLSL源码直接创建shader模块（运行时编译）
    static VkShaderModule CreateShaderModuleFromSource(VkDevice device, const std::string& glslSource, VkShaderStageFlagBits stage, const std::string& filename = "");
    
    // 验证SPIR-V字节码格式
    static bool ValidateSPIRV(const std::vector<char>& spirvCode);
    
private:
    // SPIR-V魔数验证
    static const uint32_t SPIRV_MAGIC = 0x07230203;
};

} // namespace shader
} // namespace renderer

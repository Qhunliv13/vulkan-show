#pragma once

#include <string>            // 2. 系统头文件
#include <vector>            // 2. 系统头文件

#include <vulkan/vulkan.h>  // 3. 第三方库头文件

namespace renderer {
namespace shader {

// Shader加载器 - 负责加载和编译GLSL着色器，支持SPIR-V格式和运行时编译
// 提供统一的着色器资源管理接口，自动处理文件读取和格式验证
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

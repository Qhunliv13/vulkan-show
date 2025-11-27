#pragma once

#include <string>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件

#include "core/types/render_types.h"  // 4. 项目头文件（抽象类型）

namespace renderer {
namespace shader {

// Shader加载器 - 负责加载和编译GLSL着色器，支持SPIR-V格式和运行时编译
// 提供统一的着色器资源管理接口，自动处理文件读取和格式验证
// 使用抽象类型以支持多种渲染后端，在实现层进行类型转换
class ShaderLoader {
public:
    // 从文件读取SPIR-V字节码
    static std::vector<char> LoadSPIRV(const std::string& filename);
    
    // 从GLSL源码文件编译并加载shader
    // 使用抽象类型以支持多种渲染后端
    static std::vector<char> CompileGLSLFromFile(const std::string& filename, ShaderStage stage);
    
    // 从内存中的GLSL源码编译为SPIR-V
    // 使用抽象类型以支持多种渲染后端
    static std::vector<char> CompileGLSLFromSource(const std::string& glslSource, ShaderStage stage, const std::string& filename = "");
    
    // 从SPIR-V字节码创建shader模块
    // 使用抽象类型以支持多种渲染后端
    static ShaderModuleHandle CreateShaderModuleFromSPIRV(DeviceHandle device, const std::vector<char>& spirvCode);
    
    // 从GLSL源码直接创建shader模块（运行时编译）
    // 使用抽象类型以支持多种渲染后端
    static ShaderModuleHandle CreateShaderModuleFromSource(DeviceHandle device, const std::string& glslSource, ShaderStage stage, const std::string& filename = "");
    
    // 验证SPIR-V字节码格式
    static bool ValidateSPIRV(const std::vector<char>& spirvCode);
    
private:
    // SPIR-V魔数验证
    static const uint32_t SPIRV_MAGIC = 0x07230203;
};

} // namespace shader
} // namespace renderer

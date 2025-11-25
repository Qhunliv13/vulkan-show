# 使用MSVC编译器（Visual Studio）
env = Environment()

# 设置C++标准为C++17，优化级别O2，UTF-8编码
env.Append(CXXFLAGS=['/std:c++17', '/O2', '/EHsc', '/utf-8'])

# 添加include路径
env.Append(CPPPATH=['.', 'renderer'])

# 配置Vulkan SDK路径
import os
vulkan_sdk = os.environ.get('VULKAN_SDK')
vulkan_found = False

# 如果设置了环境变量，优先使用
if vulkan_sdk:
    vulkan_include = os.path.join(vulkan_sdk, 'Include')
    vulkan_lib = os.path.join(vulkan_sdk, 'Lib')
    if os.path.exists(vulkan_include) and os.path.exists(os.path.join(vulkan_include, 'vulkan', 'vulkan.h')):
        env.Append(CPPPATH=[vulkan_include])
        print("Vulkan SDK include path found: " + vulkan_include)
        if os.path.exists(vulkan_lib):
            env.Append(LIBPATH=[vulkan_lib])
            print("Vulkan SDK library path found: " + vulkan_lib)
        vulkan_found = True

# 如果未找到，尝试常见安装路径
if not vulkan_found:
    common_paths = [
        'F:/A',  # 用户指定的安装路径
        'C:/VulkanSDK',
        'C:/Program Files/Vulkan SDK',
        os.path.expanduser('~/VulkanSDK')
    ]
    for base_path in common_paths:
        if os.path.exists(base_path):
            # 首先检查是否直接是SDK根目录
            direct_include = os.path.join(base_path, 'Include')
            direct_lib = os.path.join(base_path, 'Lib')
            if os.path.exists(direct_include) and os.path.exists(os.path.join(direct_include, 'vulkan', 'vulkan.h')):
                env.Append(CPPPATH=[direct_include])
                print("Vulkan SDK include path found: " + direct_include)
                if os.path.exists(direct_lib):
                    env.Append(LIBPATH=[direct_lib])
                    print("Vulkan SDK library path found: " + direct_lib)
                vulkan_found = True
                break
            # 如果不是，查找版本子目录
            try:
                versions = [d for d in os.listdir(base_path) if os.path.isdir(os.path.join(base_path, d)) and (d.replace('.', '').isdigit() or d.startswith('1.'))]
                if versions:
                    latest_version = sorted(versions, key=lambda x: [int(i) for i in x.split('.')] if x.replace('.', '').isdigit() else [0], reverse=True)[0]
                    vulkan_include = os.path.join(base_path, latest_version, 'Include')
                    vulkan_lib = os.path.join(base_path, latest_version, 'Lib')
                    if os.path.exists(vulkan_include) and os.path.exists(os.path.join(vulkan_include, 'vulkan', 'vulkan.h')):
                        env.Append(CPPPATH=[vulkan_include])
                        print("Vulkan SDK include path found: " + vulkan_include)
                        if os.path.exists(vulkan_lib):
                            env.Append(LIBPATH=[vulkan_lib])
                            print("Vulkan SDK library path found: " + vulkan_lib)
                        vulkan_found = True
                        break
            except:
                pass

if not vulkan_found:
    print("Warning: Vulkan SDK not found. Please install Vulkan SDK or set VULKAN_SDK environment variable.")

# 链接Windows API库和Vulkan库
env.Append(LIBS=['user32', 'gdi32', 'vulkan-1', 'gdiplus', 'ole32'])

# Shaderc库支持（如果已构建）
shadercSourceDir = 'shaderc-main'
shadercBuildDir = 'shaderc-main/build'
shadercIncludeDir = shadercSourceDir + '/libshaderc/include'
shadercLibPath = shadercBuildDir + '/Release'

# 检查shaderc是否已构建
shadercLibFile = shadercLibPath + '/shaderc_combined.lib'
if os.path.exists(shadercLibFile):
    env.Append(CPPPATH=[shadercIncludeDir])
    env.Append(LIBPATH=[shadercLibPath])
    env.Append(LIBS=['shaderc_combined'])
    env.Append(CCFLAGS=['/DUSE_SHADERC'])
    print("Shaderc library found - runtime GLSL compilation enabled")
elif os.path.exists(shadercIncludeDir):
    print("Shaderc source found but library not built.")
    print("Run build_shaderc.bat or build_shaderc.ps1 to build the library.")
else:
    print("Shaderc not found - only SPIR-V file loading will be available")

# stb_image库支持（用于WebP等格式）
stbImagePath = 'renderer/thirdparty/stb_image.h'
if os.path.exists(stbImagePath):
    env.Append(CCFLAGS=['/DUSE_STB_IMAGE'])
    print("stb_image.h found - WebP support enabled")
else:
    print("Warning: stb_image.h not found at " + stbImagePath)
    print("         WebP support will be disabled. Download stb_image.h from:")
    print("         https://github.com/nothings/stb/blob/master/stb_image.h")
    print("         and place it in renderer/thirdparty/ directory")

# 构建可执行文件
sources = [
    'main.cpp',
    'renderer/core/managers/application.cpp',
    'renderer/core/managers/app_initializer.cpp',
    'renderer/core/managers/config_manager.cpp',
    'renderer/core/managers/window_manager.cpp',
    'renderer/core/utils/input_handler.cpp',
    'renderer/core/ui/ui_manager.cpp',
    'renderer/core/ui/ui_manager_getters.cpp',
    'renderer/core/managers/event_manager.cpp',
    'renderer/core/managers/scene_manager.cpp',
    'renderer/core/managers/render_scheduler.cpp',
    'renderer/core/handlers/window_message_handler.cpp',
    'renderer/core/utils/fps_monitor.cpp',
    'renderer/core/utils/logger.cpp',
    'renderer/core/utils/event_bus.cpp',
    'renderer/core/ui/button_ui_manager.cpp',
    'renderer/core/ui/color_ui_manager.cpp',
    'renderer/core/ui/slider_ui_manager.cpp',
    'renderer/window/window.cpp',
    'renderer/vulkan/vulkan_renderer.cpp',
    'renderer/vulkan/vulkan_renderer_factory.cpp',
    'renderer/shader/shader_loader.cpp',
    'renderer/loading/loading_animation.cpp',
    'renderer/text/text_renderer.cpp',
    'renderer/ui/button/button.cpp',
    'renderer/ui/slider/slider.cpp',
    'renderer/ui/color_controller/color_controller.cpp',
    'renderer/ui/text/text.cpp',
    'renderer/image/image_loader.cpp',
    'renderer/texture/texture.cpp'
]

# 编译资源文件（如果存在）
if os.path.exists('app_icon.rc') and os.path.exists('app_icon.ico'):
    # 编译.rc文件为.res文件
    rc_builder = Builder(action='rc /fo $TARGET $SOURCE', suffix='.res', src_suffix='.rc')
    env.Append(BUILDERS={'ResourceCompiler': rc_builder})
    res_file = env.ResourceCompiler('app_icon.res', 'app_icon.rc')
    # 将.res文件添加到源文件列表
    sources.append(res_file[0])
    print("Resource file found - icon will be embedded in executable")
else:
    print("Warning: app_icon.rc or app_icon.ico not found. Run convert_icon.py to create icon file.")

env.Program('shader_app.exe', sources)

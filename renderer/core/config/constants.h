#pragma once

// 统一包含所有配置常量和枚举
// 为了向后兼容，保留此文件作为统一入口，避免修改大量现有代码
// 新代码应直接包含具体的配置头文件（window_constants.h、render_constants.h、enums.h）以提高模块化

#include "core/config/window_constants.h"
#include "core/config/render_constants.h"
#include "core/config/enums.h"

// 为了向后兼容，在全局命名空间中也提供这些常量
// 允许现有代码继续使用 WINDOW_WIDTH、AspectRatioMode 等名称而无需修改
using namespace config;

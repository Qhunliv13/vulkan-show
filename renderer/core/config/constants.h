#pragma once

/**
 * 统一包含所有配置常量和枚举
 * 
 * 此文件作为统一入口，包含所有配置相关的头文件
 * 新代码应直接包含具体的配置头文件（window_constants.h、render_constants.h、enums.h）以提高模块化
 * 
 * 注意：所有常量都在 config 命名空间中，必须使用完整限定名（如 config::WINDOW_WIDTH）
 * 禁止使用 using namespace config
 */
#include "core/config/window_constants.h"
#include "core/config/render_constants.h"
#include "core/config/enums.h"

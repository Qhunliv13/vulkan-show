#pragma once

/**
 * 渲染相关常量配置
 * 定义渲染系统使用的全局常量，包括数学常量和 Vulkan 渲染参数
 */
namespace config {

/**
 * 数学常量：圆周率，用于角度计算和几何变换
 */
constexpr double PI = 3.14159265358979323846;

/**
 * Vulkan 渲染常量：最大并发帧数，用于双缓冲或三缓冲机制
 */
const int MAX_FRAMES_IN_FLIGHT = 2;

} // namespace config


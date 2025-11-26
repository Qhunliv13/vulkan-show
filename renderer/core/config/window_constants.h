#pragma once

/**
 * 窗口相关常量配置
 * 定义窗口系统的默认尺寸和最小尺寸限制，用于窗口初始化和尺寸约束
 */
namespace config {

/**
 * 默认窗口尺寸：应用启动时的窗口宽度和高度
 */
const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 500;

/**
 * 最小窗口尺寸：窗口可调整的最小宽度和高度，防止窗口过小导致 UI 无法正常显示
 */
const int WINDOW_MIN_WIDTH = 400;
const int WINDOW_MIN_HEIGHT = 400;

} // namespace config


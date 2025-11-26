#pragma once

/**
 * 渲染和窗口相关的枚举类型定义
 */

/**
 * 拉伸模式（类似 Godot 引擎）
 * 控制 UI 元素如何响应窗口尺寸变化
 */
enum class StretchMode {
    Disabled,    // 禁用拉伸，保持原始尺寸（800x800），固定像素模式
    Scaled,      // 缩放模式：UI 随窗口尺寸缩放，使用实际窗口尺寸
    Fit          // 适应模式：缩放以适应窗口，保持宽高比，必要时添加黑边
};

/**
 * 背景拉伸模式
 * 控制背景纹理的显示方式
 */
enum class BackgroundStretchMode {
    Fit,         // 适应模式：保持宽高比，完全适应窗口内（最大适应尺寸）
    Scaled       // 缩放模式：保持宽高比，填充整个窗口无间隙（最小覆盖尺寸）
};

/**
 * 应用状态
 * 表示应用程序当前所处的状态阶段
 */
enum class AppState {
    LoadingCubes,  // 显示 loading_cubes shader（启动界面）
    Loading,       // 显示加载动画
    Shader         // 显示 shader
};


# Shader Renderer

基于 Vulkan 的实时着色器渲染应用，支持自定义着色器、UI 系统和多种渲染模式。

## 特性

- 🎨 实时着色器渲染（GLSL/SPIR-V）
- 🖼️ 灵活的拉伸模式和背景模式
- 🎮 交互式 UI 系统（按钮、滑块、颜色控制器）
- 📝 文本渲染支持
- ⚡ 固定时间步逻辑更新 + 可变时间步渲染插值

## 构建要求

- Windows 10/11
- Visual Studio（MSVC 编译器）
- Vulkan SDK
- Python 3.x
- SCons

## 构建

```bash
# 完整编译（着色器 + SCons）
python compile_shaders.py

# 只编译着色器
python compile_shaders.py --shaders-only

# 只运行 SCons（不编译着色器）
python compile_shaders.py --build-only
```

## 使用

### 拉伸模式

```bash
.\shader_app.exe --stretch=scaled    # UI随窗口缩放
.\shader_app.exe --stretch=disabled  # UI固定，不随窗口变化
.\shader_app.exe --stretch=fit       # 保持宽高比，按最小边等比缩小
```

### 背景模式

```bash
.\shader_app.exe --stretch=disabled -b Fit    # 背景Fit模式
.\shader_app.exe --stretch=disabled -b Scaled # 背景Scaled模式
```

### 组合使用

```bash
.\shader_app.exe --stretch=scaled -b Fit
```
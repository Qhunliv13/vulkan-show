cd F:\shader ; F:\A\Bin\glslangValidator.exe -V renderer\shader\shader.frag -o renderer\shader\shader.frag.spv
cd F:\shader ; F:\A\Bin\glslangValidator.exe -V renderer\loading\loading.vert -o renderer\loading\loading.vert.spv
cd F:\shader ; F:\A\Bin\glslangValidator.exe -V renderer\ui\button\button.frag -o renderer\ui\button\button.frag.spv
cd F:\shader ; F:\A\Bin\glslangValidator.exe -V renderer\ui\button\button_pure.vert -o renderer\ui\button\button_pure.vert.spv
cd F:\shader ; F:\A\Bin\glslangValidator.exe -V renderer\ui\button\button_pure.frag -o renderer\ui\button\button_pure.frag.spv

# 默认：完整编译（着色器 + SCons）
python compile_shaders.py

# 只编译着色器
python compile_shaders.py --shaders-only

# 只运行 SCons（不编译着色器）
python compile_shaders.py --build-only

.\shader_app.exe --aspect=keep      # 保持宽高比（默认）
.\shader_app.exe --aspect=expand    # 拉伸填满
.\shader_app.exe -a keepwidth        # 保持宽度
.\shader_app.exe -a keepheight       # 保持高度
.\shader_app.exe -a center           # 居中显示

# 使用Scaled模式（UI随窗口缩放）
.\shader_app.exe --stretch=scaled

# 使用禁用拉伸模式（UI固定，不随窗口变化）
.\shader_app.exe --stretch=disabled

# 使用 Fit 模式（保持宽高比，按最小边等比缩小）
.\shader_app.exe --stretch=fit

.\shader_app.exe -a keep -s scaled

# 组合使用拉伸模式和宽高比模式
.\shader_app.exe --stretch=scaled --aspect=keep

.\shader_app.exe -a keep -s disabled -b Fit    # 背景Fit模式
.\shader_app.exe -a keep -s disabled -b Scaled # 背景Scaled模式
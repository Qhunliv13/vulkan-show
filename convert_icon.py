#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
将PNG图像转换为ICO图标文件
用于Windows应用程序图标资源
"""

import sys
import os
from PIL import Image

def png_to_ico(png_path, ico_path, sizes=None):
    """
    将PNG图像转换为ICO文件
    
    Args:
        png_path: 输入的PNG文件路径
        ico_path: 输出的ICO文件路径
        sizes: 图标尺寸列表，默认为[16, 32, 48, 64, 128, 256]
    """
    if sizes is None:
        sizes = [16, 32, 48, 64, 128, 256]
    
    try:
        # 打开PNG图像
        img = Image.open(png_path)
        
        # 确保图像有alpha通道
        if img.mode != 'RGBA':
            img = img.convert('RGBA')
        
        # 创建不同尺寸的图标
        icons = []
        for size in sizes:
            # 高质量缩放
            resized = img.resize((size, size), Image.Resampling.LANCZOS)
            icons.append(resized)
        
        # 保存为ICO文件
        icons[0].save(ico_path, format='ICO', sizes=[(s, s) for s in sizes])
        print(f"成功将 {png_path} 转换为 {ico_path}")
        print(f"包含尺寸: {', '.join(map(str, sizes))}x{', '.join(map(str, sizes))}")
        return True
        
    except Exception as e:
        print(f"错误: 无法转换图标 - {e}")
        return False

if __name__ == '__main__':
    # 默认使用 assets/test.png 作为输入
    input_png = 'assets/test.png'
    output_ico = 'app_icon.ico'
    
    # 如果提供了命令行参数，使用它们
    if len(sys.argv) > 1:
        input_png = sys.argv[1]
    if len(sys.argv) > 2:
        output_ico = sys.argv[2]
    
    # 检查输入文件是否存在
    if not os.path.exists(input_png):
        print(f"错误: 输入文件不存在: {input_png}")
        sys.exit(1)
    
    # 转换图标
    if png_to_ico(input_png, output_ico):
        print(f"\n图标已创建: {output_ico}")
        print("现在可以重新编译程序，图标将嵌入到可执行文件中。")
        sys.exit(0)
    else:
        sys.exit(1)


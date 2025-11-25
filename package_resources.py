#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
资源文件打包脚本
将基于相对路径的资源文件打包成压缩包
"""

import os
import zipfile
import shutil
from pathlib import Path
from datetime import datetime

def get_resource_files():
    """获取所有需要打包的资源文件"""
    resources = []
    
    # 资源文件根目录
    base_dir = Path(__file__).parent
    
    # 1. assets 目录下的所有文件
    assets_dir = base_dir / "assets"
    if assets_dir.exists():
        for file in assets_dir.rglob("*"):
            if file.is_file():
                resources.append(("assets", file))
    
    # 2. renderer 目录下的着色器文件（.spv, .vert, .frag）
    renderer_dir = base_dir / "renderer"
    if renderer_dir.exists():
        # 着色器文件
        shader_extensions = [".spv", ".vert", ".frag", ".glsl"]
        for ext in shader_extensions:
            for file in renderer_dir.rglob(f"*{ext}"):
                if file.is_file():
                    # 保持相对路径结构
                    rel_path = file.relative_to(base_dir)
                    resources.append(("renderer", file))
    
    # 3. HTML 文件（如果有）
    html_dir = base_dir / "HTML"
    if html_dir.exists():
        for file in html_dir.rglob("*"):
            if file.is_file():
                resources.append(("HTML", file))
    
    return resources

def create_package(output_name=None):
    """创建资源文件压缩包"""
    if output_name is None:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_name = f"shader_resources_{timestamp}.zip"
    
    base_dir = Path(__file__).parent
    output_path = base_dir / output_name
    
    resources = get_resource_files()
    
    if not resources:
        print("未找到需要打包的资源文件！")
        return None
    
    print(f"找到 {len(resources)} 个资源文件，开始打包...")
    print(f"输出文件: {output_path}")
    print()
    
    with zipfile.ZipFile(output_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
        for category, file_path in resources:
            # 保持相对路径结构
            arcname = file_path.relative_to(base_dir)
            zipf.write(file_path, arcname)
            print(f"  [+] {arcname}")
    
    file_size = output_path.stat().st_size / (1024 * 1024)  # MB
    print()
    print(f"打包完成！")
    print(f"文件: {output_path}")
    print(f"大小: {file_size:.2f} MB")
    print(f"包含 {len(resources)} 个文件")
    
    return output_path

def list_resources():
    """列出所有资源文件"""
    resources = get_resource_files()
    
    print("基于相对路径的资源文件列表：")
    print("=" * 60)
    
    categories = {}
    for category, file_path in resources:
        if category not in categories:
            categories[category] = []
        rel_path = file_path.relative_to(Path(__file__).parent)
        categories[category].append(rel_path)
    
    for category, files in sorted(categories.items()):
        print(f"\n[{category}] ({len(files)} 个文件)")
        for file in sorted(files):
            size = file.stat().st_size / 1024  # KB
            print(f"  - {file} ({size:.2f} KB)")
    
    print("\n" + "=" * 60)
    print(f"总计: {len(resources)} 个文件")

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) > 1 and sys.argv[1] == "list":
        list_resources()
    else:
        output_name = sys.argv[1] if len(sys.argv) > 1 else None
        create_package(output_name)


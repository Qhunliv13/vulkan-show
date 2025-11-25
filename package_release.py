#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
发布包打包脚本
打包可执行文件和所有运行时需要的资源文件
"""

import os
import zipfile
import shutil
from pathlib import Path
from datetime import datetime

def get_executable():
    """查找可执行文件"""
    base_dir = Path(__file__).parent
    exe_files = list(base_dir.glob("*.exe"))
    
    # 优先选择 shader_app.exe
    for exe in exe_files:
        if exe.name == "shader_app.exe":
            return exe
    
    # 如果没有找到，返回第一个 .exe 文件
    if exe_files:
        return exe_files[0]
    
    return None

def get_resource_files():
    """获取所有运行时需要的资源文件"""
    resources = []
    base_dir = Path(__file__).parent
    
    # 1. assets 目录下的所有文件（图片资源）
    assets_dir = base_dir / "assets"
    if assets_dir.exists():
        for file in assets_dir.rglob("*"):
            if file.is_file():
                resources.append(file)
    
    # 2. renderer 目录下的着色器文件（只打包预编译的 .spv 文件）
    renderer_dir = base_dir / "renderer"
    if renderer_dir.exists():
        # 只打包 .spv 文件（已编译的着色器）
        for file in renderer_dir.rglob("*.spv"):
            if file.is_file():
                resources.append(file)
    
    # 不打包 HTML 文件夹和 .md 文件
    
    return resources

def get_dependencies():
    """查找可能的依赖文件"""
    base_dir = Path(__file__).parent
    deps = []
    
    # Vulkan DLL（如果存在）
    vulkan_dlls = [
        "vulkan-1.dll",
        "VkLayer_*.dll",
    ]
    
    # 查找常见的 DLL 依赖
    for dll_pattern in ["*.dll", "*.dylib", "*.so"]:
        for dll in base_dir.glob(dll_pattern):
            deps.append(dll)
    
    return deps

def create_release_package(output_name=None):
    """创建发布包"""
    base_dir = Path(__file__).parent
    
    if output_name is None:
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        output_name = f"shader_app_release_{timestamp}.zip"
    
    output_path = base_dir / output_name
    
    # 获取可执行文件
    exe_file = get_executable()
    if not exe_file:
        print("错误：未找到可执行文件！")
        print("请先编译项目生成 .exe 文件")
        return None
    
    print(f"找到可执行文件: {exe_file.name}")
    print()
    
    # 获取资源文件
    resources = get_resource_files()
    print(f"找到 {len(resources)} 个资源文件")
    
    # 获取依赖文件
    deps = get_dependencies()
    if deps:
        print(f"找到 {len(deps)} 个依赖文件")
    
    print()
    print(f"开始打包到: {output_path}")
    print()
    
    with zipfile.ZipFile(output_path, 'w', zipfile.ZIP_DEFLATED) as zipf:
        # 1. 添加可执行文件
        zipf.write(exe_file, exe_file.name)
        print(f"  [+] {exe_file.name} ({exe_file.stat().st_size / (1024*1024):.2f} MB)")
        
        # 2. 添加资源文件
        for file_path in sorted(resources):
            arcname = file_path.relative_to(base_dir)
            zipf.write(file_path, arcname)
            size_kb = file_path.stat().st_size / 1024
            print(f"  [+] {arcname} ({size_kb:.2f} KB)")
        
        # 3. 添加依赖文件
        for dep_file in sorted(deps):
            arcname = dep_file.relative_to(base_dir)
            zipf.write(dep_file, arcname)
            size_kb = dep_file.stat().st_size / 1024
            print(f"  [+] {arcname} ({size_kb:.2f} KB)")
    
    file_size = output_path.stat().st_size / (1024 * 1024)  # MB
    total_files = 1 + len(resources) + len(deps)
    
    print()
    print("=" * 60)
    print("打包完成！")
    print(f"文件: {output_path}")
    print(f"大小: {file_size:.2f} MB")
    print(f"包含: {total_files} 个文件")
    print("  - 1 个可执行文件")
    print(f"  - {len(resources)} 个资源文件")
    if deps:
        print(f"  - {len(deps)} 个依赖文件")
    print("=" * 60)
    
    return output_path

def list_package_contents():
    """列出打包内容"""
    exe_file = get_executable()
    resources = get_resource_files()
    deps = get_dependencies()
    
    print("发布包内容清单：")
    print("=" * 60)
    
    if exe_file:
        print(f"\n[可执行文件]")
        print(f"  - {exe_file.name} ({exe_file.stat().st_size / (1024*1024):.2f} MB)")
    else:
        print("\n[可执行文件]")
        print("  - 未找到！请先编译项目")
    
    if resources:
        print(f"\n[资源文件] ({len(resources)} 个)")
        categories = {}
        for file in resources:
            category = file.parts[0] if len(file.parts) > 0 else "root"
            if category not in categories:
                categories[category] = []
            rel_path = file.relative_to(Path(__file__).parent)
            categories[category].append(rel_path)
        
        for category, files in sorted(categories.items()):
            print(f"\n  [{category}] ({len(files)} 个文件)")
            for file in sorted(files):
                size_kb = file.stat().st_size / 1024
                print(f"    - {file} ({size_kb:.2f} KB)")
    
    if deps:
        print(f"\n[依赖文件] ({len(deps)} 个)")
        for dep in sorted(deps):
            rel_path = dep.relative_to(Path(__file__).parent)
            size_kb = dep.stat().st_size / 1024
            print(f"  - {rel_path} ({size_kb:.2f} KB)")
    
    print("\n" + "=" * 60)
    total = (1 if exe_file else 0) + len(resources) + len(deps)
    print(f"总计: {total} 个文件")

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) > 1 and sys.argv[1] == "list":
        list_package_contents()
    else:
        output_name = sys.argv[1] if len(sys.argv) > 1 else None
        create_release_package(output_name)


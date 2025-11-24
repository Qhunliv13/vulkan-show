#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
着色器预编译脚本
自动查找项目中的所有GLSL着色器文件并编译为SPIR-V格式
"""

import os
import sys
import subprocess
import argparse
from pathlib import Path

# 支持的着色器文件扩展名
SHADER_EXTENSIONS = ['.vert', '.frag', '.comp', '.geom', '.tesc', '.tese', '.mesh', '.task']

# 默认的glslangValidator路径（Vulkan SDK）
DEFAULT_GLSLANG_VALIDATOR = 'glslangValidator'

def find_glslang_validator():
    """查找glslangValidator可执行文件"""
    # 尝试从环境变量获取Vulkan SDK路径
    vulkan_sdk = os.environ.get('VULKAN_SDK')
    if vulkan_sdk:
        # Windows路径
        windows_path = os.path.join(vulkan_sdk, 'Bin', 'glslangValidator.exe')
        if os.path.exists(windows_path):
            return windows_path
        # Linux/Mac路径
        unix_path = os.path.join(vulkan_sdk, 'bin', 'glslangValidator')
        if os.path.exists(unix_path):
            return unix_path
    
    # 尝试从README.MD中读取路径（如果存在）
    readme_path = Path('README.MD')
    if readme_path.exists():
        try:
            with open(readme_path, 'r', encoding='utf-8') as f:
                content = f.read()
                # 查找 glslangValidator.exe 路径
                import re
                match = re.search(r'([A-Z]:[^\\s]+glslangValidator\.exe)', content)
                if match:
                    path = match.group(1)
                    if os.path.exists(path):
                        return path
        except:
            pass
    
    # 尝试使用系统PATH中的glslangValidator
    if sys.platform == 'win32':
        validator = 'glslangValidator.exe'
    else:
        validator = 'glslangValidator'
    
    # 检查是否在PATH中
    try:
        result = subprocess.run(['which', validator] if sys.platform != 'win32' else ['where', validator],
                              capture_output=True, text=True)
        if result.returncode == 0:
            return validator
    except:
        pass
    
    return DEFAULT_GLSLANG_VALIDATOR

def find_shader_files(root_dir):
    """递归查找所有着色器文件"""
    shader_files = []
    root_path = Path(root_dir)
    
    # 需要忽略的文件夹
    ignore_dirs = {'test', 'example', '__pycache__', '.git'}
    
    for ext in SHADER_EXTENSIONS:
        for shader_file in root_path.rglob(f'*{ext}'):
            # 跳过已编译的.spv文件所在目录（如果有）
            if '.spv' in str(shader_file):
                continue
            
            # 跳过忽略的文件夹
            parts = shader_file.parts
            if any(ignore_dir in parts for ignore_dir in ignore_dirs):
                continue
            
            # 只扫描renderer文件夹
            if 'renderer' not in parts:
                continue
            
            shader_files.append(shader_file)
    
    return sorted(shader_files)

def compile_shader(validator_path, shader_file, output_dir=None, verbose=False):
    """编译单个着色器文件"""
    shader_path = Path(shader_file)
    
    # 确定输出文件路径
    # 保留完整的文件名（包括中间的点），然后添加.spv扩展名
    # 例如：button.vert -> button.vert.spv
    # shader_path.name 是完整文件名（如 "button.vert"）
    output_filename = shader_path.name + '.spv'
    
    if output_dir:
        output_path = Path(output_dir) / output_filename
        output_path.parent.mkdir(parents=True, exist_ok=True)
    else:
        # 输出到源文件同目录
        output_path = shader_path.parent / output_filename
    
    # 构建编译命令
    cmd = [
        validator_path,
        '-V',  # 编译为SPIR-V
        str(shader_path),
        '-o',   # 输出文件
        str(output_path)
    ]
    
    if verbose:
        print(f'编译: {shader_path} -> {output_path}')
    
    try:
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=os.getcwd())
        
        if result.returncode == 0:
            if verbose:
                print(f'  ✓ 成功')
            return True, None
        else:
            error_msg = result.stderr or result.stdout
            if verbose:
                print(f'  ✗ 失败: {error_msg}')
            return False, error_msg
    except FileNotFoundError:
        error_msg = f'找不到编译器: {validator_path}'
        if verbose:
            print(f'  ✗ {error_msg}')
        return False, error_msg
    except Exception as e:
        error_msg = f'编译错误: {str(e)}'
        if verbose:
            print(f'  ✗ {error_msg}')
        return False, error_msg

def run_scons(verbose=False):
    """运行SCons编译"""
    cmd = ['python', '-m', 'SCons']
    
    if verbose:
        print('\n运行 SCons 编译...')
        print('=' * 60)
    
    try:
        result = subprocess.run(cmd, cwd=os.getcwd())
        return result.returncode == 0
    except Exception as e:
        if verbose:
            print(f'错误: {str(e)}')
        return False

def main():
    parser = argparse.ArgumentParser(description='编译项目中的所有GLSL着色器文件为SPIR-V格式')
    parser.add_argument('--root', '-r', default='.', help='搜索根目录（默认：当前目录）')
    parser.add_argument('--validator', '-v', help='glslangValidator路径（默认：自动检测）')
    parser.add_argument('--output', '-o', help='输出目录（默认：与源文件同目录）')
    parser.add_argument('--verbose', action='store_true', help='显示详细信息')
    parser.add_argument('--quiet', '-q', action='store_true', help='静默模式（只显示错误）')
    parser.add_argument('--shaders-only', action='store_true', help='只编译着色器，不运行SCons')
    parser.add_argument('--build-only', action='store_true', help='只运行SCons，不编译着色器')
    
    args = parser.parse_args()
    
    shader_compile_success = True
    
    # 编译着色器（除非指定只编译项目）
    if not args.build_only:
        # 查找编译器
        if args.validator:
            validator_path = args.validator
            if not os.path.exists(validator_path):
                print(f'错误: 找不到编译器: {validator_path}', file=sys.stderr)
                return 1
        else:
            validator_path = find_glslang_validator()
            if not args.quiet:
                print(f'使用编译器: {validator_path}')
        
        # 查找所有着色器文件
        if not args.quiet:
            print(f'搜索着色器文件: {args.root}')
        
        shader_files = find_shader_files(args.root)
        
        if not shader_files:
            if not args.quiet:
                print('未找到着色器文件')
        else:
            if not args.quiet:
                print(f'找到 {len(shader_files)} 个着色器文件\n')
            
            # 编译所有着色器
            success_count = 0
            fail_count = 0
            failed_files = []
            
            for shader_file in shader_files:
                if not args.quiet or args.verbose:
                    print(f'[{success_count + fail_count + 1}/{len(shader_files)}] ', end='')
                
                success, error = compile_shader(validator_path, shader_file, args.output, args.verbose or not args.quiet)
                
                if success:
                    success_count += 1
                else:
                    fail_count += 1
                    failed_files.append((shader_file, error))
            
            # 显示总结
            print()  # 空行
            print('=' * 60)
            print(f'着色器编译完成: 成功 {success_count}, 失败 {fail_count}')
            
            if failed_files:
                print('\n失败的文件:')
                for shader_file, error in failed_files:
                    print(f'  - {shader_file}')
                    if args.verbose:
                        print(f'    错误: {error}')
                shader_compile_success = False
    
    # 运行SCons编译（除非指定只编译着色器，或着色器编译失败）
    if not args.shaders_only and shader_compile_success:
        build_success = run_scons(args.verbose or not args.quiet)
        if not build_success:
            return 1
    
    return 0 if shader_compile_success else 1

if __name__ == '__main__':
    sys.exit(main())


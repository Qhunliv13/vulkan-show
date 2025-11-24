#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
统计renderer目录中的源码行数
跳过编译结果、注释行、空行、单符号行、无有效字符行
"""

import os
import re
import argparse
from pathlib import Path

# 源码文件扩展名
SOURCE_EXTENSIONS = {'.cpp', '.h', '.glsl', '.frag', '.vert'}

# 编译结果扩展名（跳过）
BUILD_EXTENSIONS = {'.obj', '.spv', '.o', '.exe', '.dll', '.lib', '.a'}

def is_comment_line(line, in_multiline_comment):
    """
    检查是否为注释行
    返回: (is_comment, new_in_multiline_comment)
    """
    stripped = line.strip()
    
    # 检查多行注释
    if in_multiline_comment:
        if '*/' in line:
            # 多行注释结束，检查结束符后是否有代码
            after_comment = line.split('*/', 1)[1].strip()
            if after_comment and not after_comment.startswith('//'):
                # 注释结束后有代码，不算纯注释行
                return (False, False)
            return (True, False)
        return (True, True)
    
    # 检查多行注释开始
    if '/*' in line:
        comment_start_idx = line.find('/*')
        # 检查/*之前是否有有效代码
        before_comment = line[:comment_start_idx].strip()
        has_code_before = bool(before_comment and not before_comment.startswith('//'))
        
        if '*/' in line:
            # 单行多行注释，检查注释后是否有代码
            comment_end_idx = line.find('*/') + 2
            after_comment = line[comment_end_idx:].strip()
            has_code_after = bool(after_comment and not after_comment.startswith('//'))
            
            if has_code_before or has_code_after:
                # 行内有代码，不算纯注释行
                return (False, False)
            return (True, False)
        else:
            # 多行注释开始
            if has_code_before:
                # 注释前有代码，不算纯注释行
                return (False, True)
            return (True, True)
    
    # 检查单行注释
    if stripped.startswith('//'):
        return (True, False)
    
    # 检查行内注释（但行内有有效代码）
    if '//' in line:
        # 检查//之前是否有有效代码
        before_comment = line.split('//')[0].strip()
        if before_comment:
            return (False, False)
        return (True, False)
    
    return (False, False)

def is_valid_code_line(line):
    """
    检查是否为有效的代码行
    跳过：空行、单符号行、无有效字符行
    """
    stripped = line.strip()
    
    # 空行
    if not stripped:
        return False
    
    # 单符号行（如只有 { 或 }）
    if len(stripped) == 1 and stripped in '{})':
        return False
    
    # 检查是否有有效字符（非空白字符）
    if not re.search(r'\S', stripped):
        return False
    
    return True

def count_lines_in_file(file_path):
    """
    统计单个文件的有效代码行数
    """
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception as e:
        print(f"警告: 无法读取文件 {file_path}: {e}")
        return 0
    
    valid_lines = 0
    in_multiline_comment = False
    
    for line in lines:
        # 检查是否为注释行
        is_comment, in_multiline_comment = is_comment_line(line, in_multiline_comment)
        
        if is_comment:
            continue
        
        # 检查是否为有效代码行
        if is_valid_code_line(line):
            valid_lines += 1
    
    return valid_lines

def format_size(size_bytes):
    """格式化文件大小"""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size_bytes < 1024.0:
            return f"{size_bytes:.2f} {unit}"
        size_bytes /= 1024.0
    return f"{size_bytes:.2f} TB"

def count_lines_in_directory(directory):
    """
    递归统计目录中所有源码文件的有效代码行数
    返回: (total_lines, file_stats)
    file_stats: [(file_path, lines, file_size), ...]
    """
    total_lines = 0
    file_stats = []
    
    directory_path = Path(directory)
    
    if not directory_path.exists():
        print(f"错误: 目录不存在: {directory}")
        return 0, []
    
    # 遍历所有文件
    for file_path in directory_path.rglob('*'):
        if not file_path.is_file():
            continue
        
        # 检查文件扩展名
        ext = file_path.suffix.lower()
        
        # 跳过编译结果
        if ext in BUILD_EXTENSIONS:
            continue
        
        # 只统计源码文件
        if ext not in SOURCE_EXTENSIONS:
            continue
        
        # 统计行数
        lines = count_lines_in_file(file_path)
        if lines > 0:
            total_lines += lines
            relative_path = file_path.relative_to(directory_path)
            file_size = file_path.stat().st_size
            file_stats.append((str(relative_path), lines, file_size))
    
    # 按行数排序
    file_stats.sort(key=lambda x: x[1], reverse=True)
    
    return total_lines, file_stats

def main():
    """
    主函数
    """
    # 解析命令行参数
    parser = argparse.ArgumentParser(description='统计renderer目录中的源码行数')
    parser.add_argument('-l', '--list', action='store_true', 
                       help='列出所有文件的详细统计（行数和大小）')
    args = parser.parse_args()
    
    # renderer目录路径
    renderer_dir = Path(__file__).parent / 'renderer'
    
    if not renderer_dir.exists():
        print(f"错误: renderer目录不存在: {renderer_dir}")
        return
    
    print("=" * 60)
    print("renderer目录源码行数统计")
    print("=" * 60)
    print(f"统计目录: {renderer_dir}")
    print(f"源码文件类型: {', '.join(sorted(SOURCE_EXTENSIONS))}")
    print(f"跳过编译结果: {', '.join(sorted(BUILD_EXTENSIONS))}")
    print("=" * 60)
    print()
    
    # 统计行数
    total_lines, file_stats = count_lines_in_directory(renderer_dir)
    
    # 显示统计结果
    print(f"总有效代码行数: {total_lines}")
    print(f"统计文件数: {len(file_stats)}")
    print()
    
    # 显示每个文件的统计
    if file_stats:
        if args.list:
            print("文件详细统计（按行数降序）:")
            print("-" * 80)
            print(f"{'行数':>8s}  |  {'大小':>12s}  |  {'文件路径'}")
            print("-" * 80)
            total_size = 0
            for file_path, lines, file_size in file_stats:
                total_size += file_size
                size_str = format_size(file_size)
                print(f"{lines:8d}  |  {size_str:>12s}  |  {file_path}")
            print("-" * 80)
            print(f"{'总计':>8s}  |  {format_size(total_size):>12s}  |  {len(file_stats)} 个文件")
            print()
        else:
            print("文件统计（按行数降序）:")
            print("-" * 60)
            for file_path, lines, _ in file_stats:
                print(f"{lines:6d} 行  |  {file_path}")
            print("-" * 60)
            print()
            print("提示: 使用 -l 或 --list 参数可查看文件大小详情")
            print()
    
    # 按扩展名分类统计
    print("按文件类型统计:")
    print("-" * 60)
    type_stats = {}
    for file_path, lines, file_size in file_stats:
        ext = Path(file_path).suffix.lower()
        if ext not in type_stats:
            type_stats[ext] = {'count': 0, 'lines': 0, 'size': 0}
        type_stats[ext]['count'] += 1
        type_stats[ext]['lines'] += lines
        type_stats[ext]['size'] += file_size
    
    for ext in sorted(type_stats.keys()):
        stats = type_stats[ext]
        if args.list:
            size_str = format_size(stats['size'])
            print(f"{ext:8s}  |  {stats['count']:3d} 个文件  |  {stats['lines']:6d} 行  |  {size_str:>12s}")
        else:
            print(f"{ext:8s}  |  {stats['count']:3d} 个文件  |  {stats['lines']:6d} 行")
    print("-" * 60)

if __name__ == '__main__':
    main()


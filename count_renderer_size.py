#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
统计renderer文件夹大小，排除编译结果和注释内容
"""

import os
import re
import argparse
from pathlib import Path

# 编译结果文件扩展名
COMPILED_EXTENSIONS = {
    '.obj',      # C++对象文件
    '.o',        # Unix对象文件
    '.spv',      # Shader编译结果（SPIR-V）
    '.exe',      # 可执行文件
    '.dll',      # 动态库
    '.so',       # Unix动态库
    '.lib',      # 静态库
    '.a',        # Unix静态库
    '.pyc',      # Python编译文件
    '.pyo',      # Python优化编译文件
}

# 编译结果目录名
COMPILED_DIRS = {
    '__pycache__',
    'build',
    'dist',
    '.build',
    '.dist',
}


def format_size(size_bytes):
    """格式化文件大小"""
    for unit in ['B', 'KB', 'MB', 'GB']:
        if size_bytes < 1024.0:
            return f"{size_bytes:.2f} {unit}"
        size_bytes /= 1024.0
    return f"{size_bytes:.2f} TB"

# 源码文件扩展名（用于行数统计）
SOURCE_EXTENSIONS = {'.cpp', '.h', '.glsl', '.frag', '.vert', '.hpp', '.c', '.py'}

def is_comment_line(line, in_multiline_comment):
    """
    检查是否为注释行（用于行数统计）
    返回: (is_comment, new_in_multiline_comment)
    """
    stripped = line.strip()
    
    # 检查多行注释
    if in_multiline_comment:
        if '*/' in line:
            after_comment = line.split('*/', 1)[1].strip()
            if after_comment and not after_comment.startswith('//'):
                return (False, False)
            return (True, False)
        return (True, True)
    
    # 检查多行注释开始
    if '/*' in line:
        comment_start_idx = line.find('/*')
        before_comment = line[:comment_start_idx].strip()
        has_code_before = bool(before_comment and not before_comment.startswith('//'))
        
        if '*/' in line:
            comment_end_idx = line.find('*/') + 2
            after_comment = line[comment_end_idx:].strip()
            has_code_after = bool(after_comment and not after_comment.startswith('//'))
            
            if has_code_before or has_code_after:
                return (False, False)
            return (True, False)
        else:
            if has_code_before:
                return (False, True)
            return (True, True)
    
    # 检查单行注释
    if stripped.startswith('//') or stripped.startswith('#'):
        return (True, False)
    
    # 检查行内注释
    if '//' in line or '#' in line:
        comment_marker = '//' if '//' in line else '#'
        before_comment = line.split(comment_marker)[0].strip()
        if before_comment:
            return (False, False)
        return (True, False)
    
    return (False, False)

def is_valid_code_line(line):
    """检查是否为有效的代码行"""
    stripped = line.strip()
    
    if not stripped:
        return False
    
    if len(stripped) == 1 and stripped in '{})':
        return False
    
    if not re.search(r'\S', stripped):
        return False
    
    return True

def count_lines_in_file(file_path):
    """统计单个文件的有效代码行数"""
    ext = file_path.suffix.lower()
    
    # 只统计源码文件
    if ext not in SOURCE_EXTENSIONS:
        return 0
    
    try:
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            lines = f.readlines()
    except Exception:
        return 0
    
    valid_lines = 0
    in_multiline_comment = False
    
    for line in lines:
        is_comment, in_multiline_comment = is_comment_line(line, in_multiline_comment)
        
        if is_comment:
            continue
        
        if is_valid_code_line(line):
            valid_lines += 1
    
    return valid_lines


def is_compiled_file(file_path):
    """判断是否为编译结果文件"""
    # 检查文件扩展名
    if file_path.suffix.lower() in COMPILED_EXTENSIONS:
        return True
    return False


def is_compiled_dir(dir_name):
    """判断是否为编译结果目录"""
    return dir_name in COMPILED_DIRS


def remove_cpp_comments(content):
    """移除C/C++/GLSL注释（// 和 /* */）"""
    # 先处理多行注释 /* */
    # 使用非贪婪匹配，避免跨行问题
    # 但要排除字符串中的 /* */
    result = []
    i = 0
    in_string = False
    string_char = None
    in_multiline_comment = False
    
    while i < len(content):
        if in_multiline_comment:
            # 在多行注释中，查找结束标记 */
            if i < len(content) - 1 and content[i:i+2] == '*/':
                in_multiline_comment = False
                i += 2
                continue
            i += 1
            continue
        
        if in_string:
            # 在字符串中，查找结束引号
            if content[i] == '\\' and i + 1 < len(content):
                # 跳过转义字符
                i += 2
                continue
            if content[i] == string_char:
                in_string = False
                string_char = None
            result.append(content[i])
            i += 1
            continue
        
        # 不在字符串中
        if content[i] in ['"', "'"]:
            in_string = True
            string_char = content[i]
            result.append(content[i])
            i += 1
        elif i < len(content) - 1 and content[i:i+2] == '//':
            # 单行注释，跳过到行尾
            while i < len(content) and content[i] != '\n':
                i += 1
            if i < len(content):
                result.append('\n')
                i += 1
        elif i < len(content) - 1 and content[i:i+2] == '/*':
            # 多行注释开始
            in_multiline_comment = True
            i += 2
        else:
            result.append(content[i])
            i += 1
    
    content = ''.join(result)
    
    # 移除空行（可选，但保留基本结构）
    lines = []
    for line in content.split('\n'):
        lines.append(line.rstrip())
    
    return '\n'.join(lines)


def remove_python_comments(content):
    """移除Python注释（# 和 """ """ 或 ''' '''）"""
    # 移除docstring（三引号字符串），因为它们通常作为注释使用
    # 同时移除单行注释 #
    
    result = []
    i = 0
    in_string = False
    string_char = None
    in_triple_quote = False
    triple_quote_type = None
    
    while i < len(content):
        if in_triple_quote:
            # 在三引号字符串中，查找结束标记
            if i < len(content) - 2 and content[i:i+3] == triple_quote_type:
                in_triple_quote = False
                triple_quote_type = None
                i += 3
                continue
            i += 1
            continue
        
        if in_string:
            # 在单引号或双引号字符串中
            if content[i] == '\\' and i + 1 < len(content):
                # 跳过转义字符
                i += 2
                continue
            if content[i] == string_char:
                in_string = False
                string_char = None
            result.append(content[i])
            i += 1
            continue
        
        # 不在字符串中
        if i < len(content) - 2 and content[i:i+3] in ['"""', "'''"]:
            # 三引号字符串开始（docstring），跳过
            triple_quote_type = content[i:i+3]
            in_triple_quote = True
            i += 3
        elif content[i] in ['"', "'"]:
            in_string = True
            string_char = content[i]
            result.append(content[i])
            i += 1
        elif content[i] == '#':
            # 单行注释，跳过到行尾
            while i < len(content) and content[i] != '\n':
                i += 1
            if i < len(content):
                result.append('\n')
                i += 1
        else:
            result.append(content[i])
            i += 1
    
    content = ''.join(result)
    
    # 移除空行（可选，但保留基本结构）
    lines = []
    for line in content.split('\n'):
        lines.append(line.rstrip())
    
    return '\n'.join(lines)


def remove_comments_from_file(file_path):
    """根据文件类型移除注释，返回移除注释后的内容大小（字节）"""
    ext = file_path.suffix.lower()
    
    try:
        # 尝试以文本模式读取
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
            content = f.read()
    except Exception:
        # 如果读取失败，返回原文件大小
        return file_path.stat().st_size
    
    # 根据文件类型移除注释
    if ext in ['.cpp', '.h', '.hpp', '.c', '.frag', '.vert', '.glsl']:
        # C/C++/GLSL文件
        content = remove_cpp_comments(content)
    elif ext == '.py':
        # Python文件
        content = remove_python_comments(content)
    # 其他文件类型（如.md）不处理注释
    
    # 返回移除注释后的内容大小（包括换行符）
    return len(content.encode('utf-8'))


def count_renderer_size(renderer_path, list_files=False):
    """统计renderer文件夹大小，排除编译结果"""
    renderer_path = Path(renderer_path)
    
    if not renderer_path.exists():
        print(f"错误: {renderer_path} 不存在")
        return
    
    if not renderer_path.is_dir():
        print(f"错误: {renderer_path} 不是目录")
        return
    
    total_size = 0
    total_lines = 0
    file_count = 0
    excluded_size = 0
    excluded_count = 0
    file_types = {}
    file_details = []  # 存储文件详细信息
    
    print(f"正在统计: {renderer_path.absolute()}")
    print("-" * 60)
    
    # 遍历所有文件
    for root, dirs, files in os.walk(renderer_path):
        # 排除编译结果目录
        dirs[:] = [d for d in dirs if not is_compiled_dir(d)]
        
        for file in files:
            file_path = Path(root) / file
            
            # 检查是否为编译结果
            if is_compiled_file(file_path):
                excluded_size += file_path.stat().st_size
                excluded_count += 1
                continue
            
            # 统计源文件（移除注释后的大小）
            original_size = file_path.stat().st_size
            code_size = remove_comments_from_file(file_path)
            lines = count_lines_in_file(file_path)
            
            total_size += code_size
            total_lines += lines
            file_count += 1
            
            # 保存文件详细信息
            relative_path = file_path.relative_to(renderer_path)
            file_details.append({
                'path': str(relative_path),
                'lines': lines,
                'code_size': code_size,
                'original_size': original_size
            })
            
            # 统计文件类型
            ext = file_path.suffix.lower() or '(无扩展名)'
            if ext not in file_types:
                file_types[ext] = {'size': 0, 'count': 0, 'original_size': 0, 'lines': 0}
            file_types[ext]['size'] += code_size
            file_types[ext]['original_size'] += original_size
            file_types[ext]['count'] += 1
            file_types[ext]['lines'] += lines
    
    # 计算原始大小（用于对比）
    original_total_size = sum(info['original_size'] for info in file_types.values())
    comments_size = original_total_size - total_size
    
    # 输出结果
    print(f"\n统计结果（已排除注释）:")
    print(f"  总文件数: {file_count}")
    print(f"  代码大小: {format_size(total_size)}")
    print(f"  原始大小: {format_size(original_total_size)}")
    print(f"  注释大小: {format_size(comments_size)}")
    print(f"  总代码行数: {total_lines}")
    print(f"\n排除的编译结果:")
    print(f"  文件数: {excluded_count}")
    print(f"  大小: {format_size(excluded_size)}")
    
    # 显示文件详细列表
    if list_files and file_details:
        print(f"\n文件详细列表（按代码大小降序）:")
        print("-" * 100)
        print(f"{'行数':>8s}  |  {'代码大小':>12s}  |  {'原始大小':>12s}  |  {'文件路径'}")
        print("-" * 100)
        # 按代码大小排序
        file_details.sort(key=lambda x: x['code_size'], reverse=True)
        for detail in file_details:
            if detail['lines'] > 0 or detail['code_size'] > 0:
                print(f"{detail['lines']:8d}  |  {format_size(detail['code_size']):>12s}  |  "
                      f"{format_size(detail['original_size']):>12s}  |  {detail['path']}")
        print("-" * 100)
        print()
    
    # 按文件类型统计
    if file_types:
        print(f"\n按文件类型统计（已排除注释）:")
        # 按大小排序
        sorted_types = sorted(file_types.items(), key=lambda x: x[1]['size'], reverse=True)
        for ext, info in sorted_types:
            code_size_str = format_size(info['size'])
            original_size_str = format_size(info['original_size'])
            comments_size = info['original_size'] - info['size']
            comments_size_str = format_size(comments_size)
            if list_files:
                print(f"  {ext:20s} - {info['count']:4d} 个文件, {info['lines']:6d} 行, "
                      f"代码: {code_size_str:>12s}, 原始: {original_size_str:>12s}, "
                      f"注释: {comments_size_str:>12s}")
            else:
                print(f"  {ext:20s} - {info['count']:4d} 个文件, {info['lines']:6d} 行, "
                      f"代码: {code_size_str:>12s}, 原始: {original_size_str:>12s}, "
                      f"注释: {comments_size_str:>12s}")
        if not list_files:
            print(f"\n提示: 使用 -l 或 --list 参数可查看每个文件的详细统计")


if __name__ == '__main__':
    # 解析命令行参数
    parser = argparse.ArgumentParser(description='统计renderer文件夹大小，排除编译结果和注释内容')
    parser.add_argument('-l', '--list', action='store_true',
                       help='列出所有文件的详细统计（行数、代码大小、原始大小）')
    args = parser.parse_args()
    
    # 获取脚本所在目录的renderer文件夹
    script_dir = Path(__file__).parent
    renderer_path = script_dir / 'renderer'
    
    count_renderer_size(renderer_path, list_files=args.list)


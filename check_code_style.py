#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
代码风格检查脚本
根据 renderer/开发标准.md 检查代码风格问题

支持的检查项：
1. 头文件保护 (#pragma once)
2. Include 顺序
3. 命名规范（类、成员变量、方法等）
4. 注释风格（Doxygen）
5. 禁止 using namespace
6. 禁止缩写命名
7. 成员变量前缀检查
8. 接口类命名和位置检查
"""

import os
import re
import sys
import argparse
from pathlib import Path
from typing import List, Tuple, Dict, Optional
from dataclasses import dataclass
from enum import Enum

# 源码文件扩展名
SOURCE_EXTENSIONS = {'.cpp', '.h'}

# 需要忽略的目录
IGNORE_DIRS = {'test', 'example', '__pycache__', '.git', 'Lib', 'Include', 'Scripts', 'thirdparty'}

# 禁止的缩写（根据开发标准）
FORBIDDEN_ABBREVIATIONS = {
    'rdr', 'Rdr', 'RDR',           # 应为 Renderer
    'mgr', 'Mgr', 'MGR',           # 应为 Manager
    'fact', 'Fact', 'FACT',        # 应为 Factory
    'btn', 'Btn',                  # 应为 Button
    'txt', 'Txt',                  # 应为 Text
    'img', 'Img',                  # 应为 Image
    'ctx', 'Ctx',                  # 应为 Context
    'impl', 'Impl',                # 应为 Implementation
    'init', 'Init',                # 应为 Initialize
    'clean', 'Clean',              # 应为 Cleanup
    'desc', 'Desc',                # 应为 Description
    'info', 'Info',                # 应为 Information
    'str',                         # 在变量名中应避免（保留字除外）
    'buf', 'Buf',                  # 应为 Buffer
    'ptr', 'Ptr',                  # 应为 Pointer
    'len', 'Len',                  # 应为 Length
    'idx', 'Idx',                  # 应为 Index
    'cnt', 'Cnt',                  # 应为 Count
    'val', 'Val',                  # 应为 Value
    'arr', 'Arr',                  # 应为 Array
    'vec', 'Vec',                  # 应为 Vector（除非是数学向量类型）
    'msg', 'Msg',                  # 应为 Message
    'param', 'Param',              # 应为 Parameter
    'cfg', 'Cfg',                  # 应为 Config
    'req', 'Req',                  # 应为 Request
    'resp', 'Resp',                # 应为 Response
}

# 允许的缩写（Windows API等）
ALLOWED_ABBREVIATIONS = {
    'HWND', 'HINSTANCE', 'HMODULE', 'HDC', 'HGLRC',  # Windows API
    'LRESULT', 'WPARAM', 'LPARAM', 'UINT',           # Windows API
    'POINT', 'RECT', 'SIZE', 'MSG',                  # Windows 结构
    'Vk',                                            # Vulkan 前缀
    'GL', 'gl',                                      # OpenGL 前缀
    'ID', 'id',                                      # 标识符（通用）
    'x', 'y', 'z', 'w',                              # 坐标/向量分量
    'r', 'g', 'b', 'a',                              # 颜色分量
    'u', 'v',                                        # 纹理坐标
    'h', 'w',                                        # 高度、宽度（在变量名中）
    'std',                                           # 标准库
    'stl',                                           # STL
}

# PIMPL 模式的允许命名
PIMPL_NAMES = {'Impl', 'impl', 'PImpl', 'pImpl'}

# 方法名中允许的缩写（如 .str(), .size() 等）
METHOD_ALLOWED = {'str', 'size', 'length', 'empty', 'clear', 'push', 'pop', 'top', 'front', 'back'}

class IssueType(Enum):
    """问题类型"""
    ERROR = '错误'
    WARNING = '警告'
    INFO = '信息'

@dataclass
class CodeIssue:
    """代码问题"""
    file_path: str
    line_number: int
    issue_type: IssueType
    category: str
    message: str
    code_snippet: str = ""

class CodeStyleChecker:
    """代码风格检查器"""
    
    def __init__(self, root_dir: str, strict: bool = False):
        self.root_dir = Path(root_dir)
        self.strict = strict
        self.issues: List[CodeIssue] = []
        
    def check_file(self, file_path: Path) -> List[CodeIssue]:
        """检查单个文件"""
        issues = []
        
        try:
            with open(file_path, 'r', encoding='utf-8', errors='ignore') as f:
                lines = f.readlines()
        except Exception as e:
            issues.append(CodeIssue(
                file_path=str(file_path),
                line_number=0,
                issue_type=IssueType.ERROR,
                category='文件读取',
                message=f'无法读取文件: {e}'
            ))
            return issues
        
        is_header = file_path.suffix == '.h'
        is_source = file_path.suffix == '.cpp'
        
        # 检查头文件保护
        if is_header:
            issues.extend(self._check_header_guard(file_path, lines))
        
        # 检查 Include 顺序
        if is_header or is_source:
            issues.extend(self._check_include_order(file_path, lines, is_source))
        
        # 检查 using namespace
        issues.extend(self._check_using_namespace(file_path, lines))
        
        # 检查命名规范
        issues.extend(self._check_naming_conventions(file_path, lines))
        
        # 检查注释风格（可选）
        if self.strict:
            issues.extend(self._check_comment_style(file_path, lines))
        
        return issues
    
    def _check_header_guard(self, file_path: Path, lines: List[str]) -> List[CodeIssue]:
        """检查头文件保护"""
        issues = []
        
        if not lines:
            return issues
        
        # 检查第一行是否为 #pragma once
        first_line = lines[0].strip()
        if not first_line.startswith('#pragma once'):
            issues.append(CodeIssue(
                file_path=str(file_path),
                line_number=1,
                issue_type=IssueType.ERROR,
                category='头文件保护',
                message='头文件缺少 #pragma once 保护',
                code_snippet=first_line
            ))
        
        return issues
    
    def _check_include_order(self, file_path: Path, lines: List[str], is_source: bool) -> List[CodeIssue]:
        """检查 Include 顺序"""
        issues = []
        
        # 收集所有 include 行
        include_lines = []
        current_line = 0
        
        in_multiline_comment = False
        for i, line in enumerate(lines, 1):
            stripped = line.strip()
            
            # 检查多行注释
            if '/*' in line:
                in_multiline_comment = True
            if '*/' in line:
                in_multiline_comment = False
            if in_multiline_comment:
                continue
            
            # 跳过单行注释
            if stripped.startswith('//'):
                continue
            
            # 查找 include 行
            if stripped.startswith('#include'):
                include_lines.append((i, line, stripped))
                current_line = i
        
        if not include_lines:
            return issues
        
        # 检查 include 顺序
        # 顺序应该是：
        # 1. 当前文件的对应头文件（.cpp 文件）
        # 2. 系统头文件（<...>）
        # 3. 第三方库头文件
        # 4. 项目头文件（"..."）
        
        sections = {
            'current_header': [],      # 1. 对应头文件
            'system': [],              # 2. 系统头文件
            'thirdparty': [],          # 3. 第三方库
            'project': []              # 4. 项目头文件
        }
        
        current_header_name = file_path.stem + '.h'
        
        for line_num, original_line, stripped in include_lines:
            # 提取 include 内容
            match = re.match(r'#include\s+[<"]([^>"]+)[>"]', stripped)
            if not match:
                continue
            
            include_path = match.group(1)
            
            # 判断分类
            if is_source and include_path.endswith(current_header_name):
                sections['current_header'].append((line_num, original_line, include_path))
            elif stripped.startswith('#include <'):
                # 判断是否为第三方库（如 vulkan/vulkan.h）
                if 'vulkan' in include_path or 'glfw' in include_path or 'glm' in include_path:
                    sections['thirdparty'].append((line_num, original_line, include_path))
                else:
                    sections['system'].append((line_num, original_line, include_path))
            else:  # 项目头文件
                sections['project'].append((line_num, original_line, include_path))
        
        # 检查顺序是否正确
        last_section_end = 0
        
        # 1. 对应头文件应该在第一位
        if is_source and sections['current_header']:
            expected_line = include_lines[0][0]
            actual_line = sections['current_header'][0][0]
            if actual_line != expected_line:
                issues.append(CodeIssue(
                    file_path=str(file_path),
                    line_number=actual_line,
                    issue_type=IssueType.WARNING,
                    category='Include 顺序',
                    message=f'对应头文件应该在最前面（当前在第 {actual_line} 行）',
                    code_snippet=sections['current_header'][0][1].strip()
                ))
            last_section_end = sections['current_header'][-1][0]
        
        # 2. 系统头文件应该在第三方库之前
        if sections['system'] and sections['thirdparty']:
            if sections['thirdparty'][0][0] < sections['system'][-1][0]:
                issues.append(CodeIssue(
                    file_path=str(file_path),
                    line_number=sections['thirdparty'][0][0],
                    issue_type=IssueType.WARNING,
                    category='Include 顺序',
                    message='第三方库头文件应该在系统头文件之后',
                    code_snippet=sections['thirdparty'][0][1].strip()
                ))
        
        # 3. 项目头文件应该在最后
        if sections['project']:
            project_start = sections['project'][0][0]
            all_other_includes = []
            if sections['current_header']:
                all_other_includes.extend([x[0] for x in sections['current_header']])
            if sections['system']:
                all_other_includes.extend([x[0] for x in sections['system']])
            if sections['thirdparty']:
                all_other_includes.extend([x[0] for x in sections['thirdparty']])
            
            if all_other_includes and project_start < max(all_other_includes):
                issues.append(CodeIssue(
                    file_path=str(file_path),
                    line_number=project_start,
                    issue_type=IssueType.WARNING,
                    category='Include 顺序',
                    message='项目头文件应该在系统/第三方库头文件之后',
                    code_snippet=sections['project'][0][1].strip()
                ))
        
        return issues
    
    def _check_using_namespace(self, file_path: Path, lines: List[str]) -> List[CodeIssue]:
        """检查禁止的 using namespace"""
        issues = []
        
        in_multiline_comment = False
        for i, line in enumerate(lines, 1):
            stripped = line.strip()
            
            # 检查多行注释
            if '/*' in line:
                in_multiline_comment = True
            if '*/' in line:
                in_multiline_comment = False
            if in_multiline_comment:
                continue
            
            # 跳过单行注释
            if stripped.startswith('//'):
                continue
            
            # 检查 using namespace
            if re.search(r'\busing\s+namespace\b', stripped):
                issues.append(CodeIssue(
                    file_path=str(file_path),
                    line_number=i,
                    issue_type=IssueType.ERROR,
                    category='禁止 using namespace',
                    message='严格禁止使用 using namespace',
                    code_snippet=stripped
                ))
        
        return issues
    
    def _check_naming_conventions(self, file_path: Path, lines: List[str]) -> List[CodeIssue]:
        """检查命名规范"""
        issues = []
        
        in_multiline_comment = False
        in_string = False
        string_char = None
        
        for i, line in enumerate(lines, 1):
            stripped = line.strip()
            
            # 跳过空行和注释
            if not stripped or stripped.startswith('//'):
                continue
            
            # 检查多行注释
            if '/*' in line:
                in_multiline_comment = True
            if '*/' in line:
                in_multiline_comment = False
            if in_multiline_comment:
                continue
            
            # 检查字符串
            if '"' in stripped or "'" in stripped:
                # 简单检查是否在字符串中（不完美但足够）
                if stripped.count('"') % 2 != 0:
                    in_string = not in_string
                continue
            
            if in_string:
                continue
            
            # 检查类名（接口类应该以 I 开头）
            # 只检查完整的类定义，排除枚举、前向声明等
            # 完整类定义：class ClassName { 或 class ClassName :
            # 前向声明：class ClassName;
            # 枚举：enum class EnumName
            
            # 跳过枚举类定义
            if re.search(r'\benum\s+class\b', stripped):
                continue
            
            # 检查完整的类定义（包含 { 或 :）
            class_def_match = re.search(r'\bclass\s+([A-Za-z_][A-Za-z0-9_]*)\s*[:\{]', stripped)
            if class_def_match:
                class_name = class_def_match.group(1)
                
                # 检查是否在 core/interfaces/ 目录下
                file_str = str(file_path)
                is_interface_dir = 'interfaces' in file_str
                
                if is_interface_dir and not class_name.startswith('I'):
                    issues.append(CodeIssue(
                        file_path=str(file_path),
                        line_number=i,
                        issue_type=IssueType.ERROR,
                        category='命名规范',
                        message=f'接口类必须以 I 开头: {class_name}',
                        code_snippet=stripped
                    ))
            
            # 检查成员变量（应该以 m_ 或 s_ 开头）
            member_match = re.search(r'\b([a-z_][a-zA-Z0-9_]*)\s*[=;,\[)]', stripped)
            if member_match and ('private:' in lines[i-2:i+1] or 'public:' in lines[i-2:i+1] or 'protected:' in lines[i-2:i+1]):
                var_name = member_match.group(1)
                # 排除常见关键字和类型
                if var_name not in {'bool', 'int', 'float', 'double', 'char', 'void', 'auto',
                                   'const', 'static', 'virtual', 'inline', 'explicit', 'override',
                                   'final', 'public', 'private', 'protected', 'struct', 'class',
                                   'enum', 'union', 'namespace', 'using', 'typedef', 'template',
                                   'typename', 'if', 'else', 'for', 'while', 'return', 'new',
                                   'delete', 'this', 'nullptr', 'true', 'false'}:
                    if not var_name.startswith('m_') and not var_name.startswith('s_'):
                        # 检查是否在类定义中
                        context_lines = ''.join(lines[max(0, i-5):i+1])
                        if 'class' in context_lines or 'struct' in context_lines:
                            issues.append(CodeIssue(
                                file_path=str(file_path),
                                line_number=i,
                                issue_type=IssueType.WARNING,
                                category='命名规范',
                                message=f'成员变量应该以 m_ 或 s_ 开头: {var_name}',
                                code_snippet=stripped
                            ))
            
            # 检查缩写
            for abbrev in FORBIDDEN_ABBREVIATIONS:
                # 使用单词边界匹配
                pattern = r'\b' + re.escape(abbrev) + r'\b'
                matches = re.finditer(pattern, stripped, re.IGNORECASE)
                for match in matches:
                    matched_text = match.group(0)
                    
                    # 检查是否在允许的缩写列表中
                    if matched_text in ALLOWED_ABBREVIATIONS:
                        continue
                    
                    # 检查是否是 PIMPL 模式的命名（完全允许）
                    if matched_text in PIMPL_NAMES:
                        continue
                    
                    # 检查是否在方法调用中（如 .str(), .size() 等）
                    before_match = stripped[:match.start()]
                    after_match = stripped[match.end():]
                    if before_match.rstrip().endswith('.') or before_match.rstrip().endswith('->'):
                        if matched_text.lower() in METHOD_ALLOWED:
                            continue
                    
                    # 检查是否是 Windows API 类型（如 MSG, POINT 等）
                    if matched_text.isupper() and matched_text in {'MSG', 'POINT', 'RECT', 'SIZE'}:
                        # Windows API 类型完全允许
                        continue
                    
                    # 检查是否是 Windows API 参数名（msg 在 Windows 消息处理中很常见）
                    if matched_text.lower() == 'msg':
                        # 检查是否在 Windows 消息处理相关的上下文中
                        # 检查当前函数范围内是否有 MSG 类型的参数
                        context_lines = ''.join(lines[max(0, i-10):i+1])
                        # 检查函数参数中是否有 const MSG& msg 或 MSG msg
                        if re.search(r'const\s+MSG\s*&\s*msg|MSG\s+msg', context_lines):
                            continue
                        # 检查函数名是否包含 Message
                        if re.search(r'\w*Message\s*\([^)]*MSG', context_lines):
                            continue
                        # 检查是否在 Windows API 调用附近
                        if any(keyword in context_lines for keyword in ['MSG', 'PeekMessage', 'TranslateMessage', 'DispatchMessage']):
                            continue
                    
                    # 检查是否在 lambda 表达式中（lambda 参数允许短命名）
                    lambda_match = re.search(r'\[[^\]]*\].*?\([^)]*\b' + re.escape(matched_text) + r'\b', stripped)
                    if lambda_match:
                        continue
                    
                    # 检查是否是枚举值（如 LogLevel::Info, EventType::ButtonClicked）
                    enum_match = re.search(r'\w+::' + re.escape(matched_text) + r'\b', stripped)
                    if enum_match:
                        continue
                    
                    # 检查是否在 case 语句中（枚举值的 case）
                    case_match = re.search(r'\bcase\s+\w+::' + re.escape(matched_text) + r'\b', stripped)
                    if case_match:
                        continue
                    
                    # 检查是否在枚举定义中（如 enum class LogLevel { Info, ... }）
                    # 查找前面的行，看是否有 enum class 或 enum
                    context_lines = ''.join(lines[max(0, i-5):i+1])
                    if re.search(r'\benum\s+(class\s+)?\w+.*\{', context_lines):
                        # 检查当前行是否是枚举值（通常是 Name, 或 Name = value,）
                        if re.search(r'^\s*' + re.escape(matched_text) + r'\s*[,=]', stripped):
                            continue
                    
                    # 检查是否是循环变量（在 for 循环中）
                    # for (Type var : ...) 或 for (Type var; ...; ...)
                    loop_pattern1 = re.search(r'\bfor\s*\([^)]*\b' + re.escape(matched_text) + r'\s*[:;]', stripped)
                    loop_pattern2 = re.search(r'\bfor\s*\([^)]*\b' + re.escape(matched_text) + r'\s*\)', stripped)
                    if loop_pattern1 or loop_pattern2:
                        # 循环变量允许短命名
                        continue
                    
                    # 检查是否是局部变量（允许短命名，特别是在小作用域内）
                    # 如果是在赋值语句或声明语句中，且不是成员变量，允许
                    var_decl_match = re.search(r'\b(auto|int|size_t|uint32_t|float|double|bool|char|std::\w+)\s+' + re.escape(matched_text) + r'\s*[=;]', stripped)
                    if var_decl_match:
                        # 检查上下文，确保不在类成员定义中
                        context_before = ''.join(lines[max(0, i-5):i])
                        if not ('private:' in context_before or 'public:' in context_before or 'protected:' in context_before):
                            # 局部变量允许短命名（如 idx, btn, info 等）
                            # 但如果变量名长度 >= 3，且不在循环中，仍然检查
                            if len(matched_text) <= 3:
                                continue
                    
                    # 检查是否是方法名（如 Info(), Log() 等）
                    method_match = re.search(r'\b' + re.escape(matched_text) + r'\s*\(', stripped)
                    if method_match:
                        # 检查是否是常见的日志级别方法名
                        if matched_text.lower() in {'info', 'debug', 'warn', 'error'}:
                            # 日志级别方法名允许
                            continue
                    
                    # 检查是否在注释或字符串中
                    before_match_text = stripped[:match.start()]
                    comment_count = before_match_text.count('//') + before_match_text.count('/*')
                    quote_count = before_match_text.count('"') + before_match_text.count("'")
                    
                    if comment_count % 2 == 0 and quote_count % 2 == 0:
                        # 检查是否在类型转换中（如 static_cast<>）
                        if 'cast' in before_match_text.lower() or 'reinterpret_cast' in before_match_text.lower():
                            continue
                        
                        issues.append(CodeIssue(
                            file_path=str(file_path),
                            line_number=i,
                            issue_type=IssueType.WARNING,
                            category='命名规范',
                            message=f'禁止使用缩写: {matched_text}（应使用完整单词）',
                            code_snippet=stripped
                        ))
        
        return issues
    
    def _check_comment_style(self, file_path: Path, lines: List[str]) -> List[CodeIssue]:
        """检查注释风格（可选，仅在严格模式下）"""
        issues = []
        
        # 检查事件性注释（TODO, FIXME 等）
        for i, line in enumerate(lines, 1):
            stripped = line.strip()
            
            # 检查 TODO, FIXME 等
            if re.search(r'\b(TODO|FIXME|HACK|XXX|NOTE)\b', stripped, re.IGNORECASE):
                issues.append(CodeIssue(
                    file_path=str(file_path),
                    line_number=i,
                    issue_type=IssueType.INFO,
                    category='注释风格',
                    message='发现事件性注释（应使用解释性注释或 Issue 跟踪系统）',
                    code_snippet=stripped
                ))
        
        return issues
    
    def check_directory(self, directory: Optional[Path] = None) -> List[CodeIssue]:
        """递归检查目录中的所有源码文件"""
        if directory is None:
            directory = self.root_dir
        
        all_issues = []
        
        for file_path in directory.rglob('*'):
            # 跳过非文件
            if not file_path.is_file():
                continue
            
            # 跳过不支持的扩展名
            if file_path.suffix not in SOURCE_EXTENSIONS:
                continue
            
            # 跳过忽略的目录
            parts = file_path.parts
            if any(ignore_dir in parts for ignore_dir in IGNORE_DIRS):
                continue
            
            # 只检查 renderer 目录下的文件
            if 'renderer' not in parts:
                continue
            
            # 检查文件
            issues = self.check_file(file_path)
            all_issues.extend(issues)
        
        return all_issues
    
    def print_report(self, issues: List[CodeIssue], show_info: bool = False):
        """打印检查报告"""
        if not issues:
            print("[OK] 未发现代码风格问题")
            return
        
        # 按文件分组
        issues_by_file: Dict[str, List[CodeIssue]] = {}
        for issue in issues:
            if issue.file_path not in issues_by_file:
                issues_by_file[issue.file_path] = []
            issues_by_file[issue.file_path].append(issue)
        
        # 统计
        error_count = sum(1 for i in issues if i.issue_type == IssueType.ERROR)
        warning_count = sum(1 for i in issues if i.issue_type == IssueType.WARNING)
        info_count = sum(1 for i in issues if i.issue_type == IssueType.INFO)
        
        # 打印统计
        print("=" * 80)
        print("代码风格检查报告")
        print("=" * 80)
        print(f"总问题数: {len(issues)}")
        print(f"  错误: {error_count}")
        print(f"  警告: {warning_count}")
        if show_info:
            print(f"  信息: {info_count}")
        print("=" * 80)
        print()
        
        # 打印详细问题
        for file_path in sorted(issues_by_file.keys()):
            file_issues = issues_by_file[file_path]
            
            # 过滤信息级别（如果不需要显示）
            if not show_info:
                file_issues = [i for i in file_issues if i.issue_type != IssueType.INFO]
                if not file_issues:
                    continue
            
            # 获取相对路径
            try:
                rel_path = Path(file_path).relative_to(self.root_dir)
            except ValueError:
                rel_path = Path(file_path)
            
            print(f"文件: {rel_path}")
            print("-" * 80)
            
            for issue in file_issues:
                issue_type_str = {
                    IssueType.ERROR: '错误',
                    IssueType.WARNING: '警告',
                    IssueType.INFO: '信息'
                }[issue.issue_type]
                
                icon = {
                    IssueType.ERROR: '[X]',
                    IssueType.WARNING: '[!]',
                    IssueType.INFO: '[i]'
                }[issue.issue_type]
                
                print(f"  {icon} [{issue_type_str}] 第 {issue.line_number} 行 - {issue.category}")
                print(f"      {issue.message}")
                if issue.code_snippet:
                    snippet = issue.code_snippet.strip()
                    if len(snippet) > 70:
                        snippet = snippet[:67] + '...'
                    print(f"      {snippet}")
                print()
            
            print()

def main():
    """主函数"""
    parser = argparse.ArgumentParser(
        description='检查代码风格问题（根据 renderer/开发标准.md）',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
检查项包括：
  - 头文件保护 (#pragma once)
  - Include 顺序
  - 命名规范（类、成员变量、方法等）
  - 禁止 using namespace
  - 禁止缩写命名
  - 成员变量前缀检查
  - 接口类命名和位置检查

示例：
  python check_code_style.py
  python check_code_style.py --strict
  python check_code_style.py --dir renderer/core
  python check_code_style.py --info
        """
    )
    
    parser.add_argument(
        '--dir', '-d',
        default='.',
        help='要检查的目录（默认：当前目录）'
    )
    
    parser.add_argument(
        '--strict', '-s',
        action='store_true',
        help='严格模式（包括注释风格检查）'
    )
    
    parser.add_argument(
        '--info',
        action='store_true',
        help='显示信息级别的问题'
    )
    
    parser.add_argument(
        '--file', '-f',
        help='只检查指定文件'
    )
    
    parser.add_argument(
        '--quiet', '-q',
        action='store_true',
        help='静默模式（只显示错误）'
    )
    
    args = parser.parse_args()
    
    # 创建检查器
    checker = CodeStyleChecker(args.dir, strict=args.strict)
    
    # 检查文件或目录
    if args.file:
        file_path = Path(args.file)
        if not file_path.exists():
            print(f"错误: 文件不存在: {args.file}", file=sys.stderr)
            return 1
        
        issues = checker.check_file(file_path)
    else:
        issues = checker.check_directory()
    
    # 过滤问题级别
    if args.quiet:
        issues = [i for i in issues if i.issue_type == IssueType.ERROR]
    
    # 打印报告
    checker.print_report(issues, show_info=args.info)
    
    # 返回错误代码
    error_count = sum(1 for i in issues if i.issue_type == IssueType.ERROR)
    return 1 if error_count > 0 else 0

if __name__ == '__main__':
    sys.exit(main())
#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
check_name.py
一次性扫描目录下所有 .h/.hpp/.cpp 文件，给出命名规范违规提示。
用法:
    python check_name.py [dir]   # 默认扫描当前目录
    python check_name.py [dir] --ignore-dirs build,third_party  # 忽略指定目录
"""

import os
import re
import sys
import argparse
from pathlib import Path
from collections import defaultdict

# ---------- 可调整的配置 ----------
EXTS = ('.h', '.hpp', '.cpp', '.cc', '.cxx')

# 忽略的目录（默认）
DEFAULT_IGNORE_DIRS = {'build', 'builds', 'out', 'bin', 'obj', '.git', '.vs', 'third_party', 'third-party', 'external'}

# 缩写黑名单（小写）
ABBR_BLACK = {
    'mgr', 'fact', 'rdr', 'wnd', 'txt', 'cnt', 'idx', 'num', 'buf', 'img',
    'ctx', 'hdl', 'impl', 'params', 'calc', 'init', 'util', 'str', 'val',
    'ptr', 'ref', 'tmp', 'temp', 'info', 'desc', 'cfg', 'config', 'opt'
}

# Windows API 保留缩写（大写）
WIN_API_OK = {'HWND', 'HINSTANCE', 'HRESULT', 'HANDLE', 'DWORD', 'LPARAM', 'WPARAM', 'LRESULT', 'LPSTR', 'LPCSTR'}

# 白名单：允许出现在参数/变量名中的完整单词（防止误杀）
WHITE_WORD = {'renderer', 'factory', 'manager', 'window', 'event', 'buffer', 'texture', 'shader'}

# C++ 关键字（忽略）
CPP_KEYWORDS = {
    'override', 'final', 'const', 'static', 'virtual', 'inline', 'explicit',
    'public', 'private', 'protected', 'namespace', 'using', 'template', 'typename',
    'class', 'struct', 'enum', 'union', 'return', 'if', 'else', 'for', 'while',
    'do', 'switch', 'case', 'default', 'break', 'continue', 'goto', 'try', 'catch',
    'throw', 'new', 'delete', 'this', 'nullptr', 'true', 'false', 'bool', 'int',
    'float', 'double', 'char', 'void', 'auto', 'decltype', 'sizeof', 'alignof'
}

# ---------- 正则 ----------
RE_CLASS = re.compile(r'^\s*(?:class|struct)\s+([A-Za-z_]\w*)')
RE_ENUM = re.compile(r'^\s*(?:enum\s+(?:class\s+)?)?([A-Za-z_]\w*)\s*[:{]')
RE_ENUM_VALUE = re.compile(r'^\s*([A-Za-z_]\w*)\s*(?:=\s*[^,}]+)?\s*[,}]')
RE_MEMBER = re.compile(r'^\s*(?:static\s+)?(?:const\s+)?(?:\w+\s*\*\s*|\w+\s+)(\*\s*)?([A-Za-z_]\w*)\s*[;=]')
RE_CONST_MEMBER = re.compile(r'^\s*(?:static\s+)?const\s+(?:\w+\s+)?([A-Z][A-Z0-9_]*)\s*[=;]')
RE_FUNC = re.compile(r'^\s*(?:virtual\s+)?(?:static\s+)?(?:inline\s+)?(?:explicit\s+)?(?:\w+\s*\*?\s+)?([A-Za-z_]\w*)\s*\(')
RE_PARAM = re.compile(r'^\s*(?:const\s+)?(?:\w+\s*\*?\s*|\w+\s*\&?\s*)([A-Za-z_]\w*)\s*[,)]')
RE_NAMESPACE = re.compile(r'^\s*namespace\s+([A-Za-z_]\w*)')
RE_MACRO = re.compile(r'^\s*#define\s+([A-Z][A-Z0-9_]*)\s+')
RE_TYPEDEF = re.compile(r'^\s*typedef\s+.*\s+([A-Za-z_]\w*)\s*;')
RE_USING = re.compile(r'^\s*using\s+([A-Za-z_]\w*)\s*=')

# ---------- 统计 ----------
stats = defaultdict(int)

# ---------- 工具 ----------
def is_pascal(s: str) -> bool:
    """检查是否为 PascalCase"""
    if not s:
        return False
    return s[0].isupper() and '_' not in s and s.isalnum()

def is_camel(s: str) -> bool:
    """检查是否为 camelCase"""
    if not s:
        return False
    return s[0].islower() and '_' not in s and s.isalnum()

def is_upper_snake(s: str) -> bool:
    """检查是否为 UPPER_SNAKE_CASE（用于常量）"""
    if not s:
        return False
    return s.isupper() and '_' in s and all(c.isalnum() or c == '_' for c in s)

def has_abbr(name: str) -> bool:
    """检查是否包含黑名单缩写"""
    if not name:
        return False
    # 分割驼峰命名
    parts = re.split(r'(?=[A-Z])', name)
    for part in parts:
        if part and part.lower() in ABBR_BLACK and part.lower() not in WHITE_WORD:
            return True
    return False

def should_ignore_path(path: Path, ignore_dirs: set) -> bool:
    """检查路径是否应该被忽略"""
    parts = path.parts
    for part in parts:
        if part in ignore_dirs:
            return True
    return False

def report(file, line_no, col, msg, hint='', category=''):
    """报告问题"""
    stats[category or msg] += 1
    file_str = str(file)
    print(f'{file_str}:{line_no}:{col}: {msg}  {hint}')

# ---------- 检查 ----------
def check_class(line, file, line_no):
    """检查类命名"""
    m = RE_CLASS.search(line)
    if not m:
        return
    name = m.group(1)
    if name.startswith('I'):
        if len(name) == 1 or not is_pascal(name[1:]):
            report(file, line_no, m.start(1), '接口类命名', 'I 后应 PascalCase：IRenderer', '类命名')
    elif name.endswith('Factory'):
        if not is_pascal(name[:-7]):
            report(file, line_no, m.start(1), '工厂类命名', '前缀应为 PascalCase：VulkanRendererFactory', '类命名')
    elif name.endswith('Manager'):
        if not is_pascal(name[:-7]):
            report(file, line_no, m.start(1), '管理器类命名', '前缀应为 PascalCase：WindowManager', '类命名')
    elif not is_pascal(name):
        report(file, line_no, m.start(1), '普通类命名', '应使用 PascalCase：EventManager', '类命名')

    if has_abbr(name):
        report(file, line_no, m.start(1), '类名含缩写', '禁止使用缩写', '缩写检查')

def check_enum(line, file, line_no):
    """检查枚举类型命名"""
    m = RE_ENUM.search(line)
    if not m:
        return
    name = m.group(1)
    if name and not is_pascal(name):
        report(file, line_no, m.start(1), '枚举类型命名', '应使用 PascalCase：EventType', '枚举命名')

def check_enum_value(line, file, line_no):
    """检查枚举值命名"""
    # 只在枚举定义块内检查
    m = RE_ENUM_VALUE.search(line)
    if not m:
        return
    name = m.group(1)
    if name and name not in CPP_KEYWORDS and not is_pascal(name):
        report(file, line_no, m.start(1), '枚举值命名', '应使用 PascalCase：ButtonClicked', '枚举命名')

def check_member(line, file, line_no):
    """检查成员变量命名"""
    m = RE_MEMBER.search(line)
    if not m:
        return
    name = m.group(2)
    if not name or name in CPP_KEYWORDS:
        return
    if name.upper() in WIN_API_OK:
        return
    
    # 检查常量成员（全大写）
    if 'const' in line and not 'static' in line:
        # 非静态常量成员，检查是否为全大写
        if not is_upper_snake(name) and name not in WIN_API_OK:
            # 允许 m_ 前缀的常量成员
            if not name.startswith('m_'):
                report(file, line_no, m.start(2), '常量成员命名', '应使用 UPPER_SNAKE_CASE 或 m_ 前缀', '成员命名')
        return
    
    static = 'static' in line
    if static:
        if not name.startswith('s_'):
            report(file, line_no, m.start(2), '静态成员', '应以 s_ 开头：s_instance', '成员命名')
        else:
            core = name[2:]
            if core and not is_camel(core):
                report(file, line_no, m.start(2), '静态成员', 's_ 后应 camelCase：s_instance', '成员命名')
    else:
        if not name.startswith('m_'):
            report(file, line_no, m.start(2), '成员变量', '应以 m_ 开头：m_renderer', '成员命名')
        else:
            core = name[2:]
            if core and not is_camel(core):
                report(file, line_no, m.start(2), '成员变量', 'm_ 后应 camelCase：m_renderer', '成员命名')

    if has_abbr(name):
        report(file, line_no, m.start(2), '成员变量含缩写', '禁止使用缩写', '缩写检查')

def check_const_member(line, file, line_no):
    """检查常量成员（全大写）"""
    m = RE_CONST_MEMBER.search(line)
    if not m:
        return
    name = m.group(1)
    if name and name.upper() not in WIN_API_OK and not is_upper_snake(name):
        report(file, line_no, m.start(1), '常量命名', '应使用 UPPER_SNAKE_CASE：MAX_BUFFER_SIZE', '常量命名')

def check_func(line, file, line_no):
    """检查函数命名"""
    m = RE_FUNC.search(line)
    if not m:
        return
    name = m.group(1)
    if not name or name in CPP_KEYWORDS:
        return
    
    # 跳过析构函数
    if name.startswith('~'):
        return
    
    if name.startswith('Get'):
        if len(name) > 3 and not is_pascal(name[3:]):
            report(file, line_no, m.start(1), 'Getter 命名', 'Get 后应 PascalCase：GetHandle', '函数命名')
    elif name.startswith('Set'):
        if len(name) > 3 and not is_pascal(name[3:]):
            report(file, line_no, m.start(1), 'Setter 命名', 'Set 后应 PascalCase：SetEventBus', '函数命名')
    elif name.startswith('Is') or name.startswith('Has'):
        suffix = name[2:] if name.startswith('Is') else name[3:]
        if suffix and not is_pascal(suffix):
            report(file, line_no, m.start(1), '布尔查询命名', 'Is/Has 后应 PascalCase：IsRunning', '函数命名')
    elif not is_pascal(name):
        report(file, line_no, m.start(1), '方法命名', '应使用 PascalCase：Initialize', '函数命名')

    if has_abbr(name):
        report(file, line_no, m.start(1), '方法名含缩写', '禁止使用缩写', '缩写检查')

def check_param(line, file, line_no):
    """检查参数命名"""
    for m in RE_PARAM.finditer(line):
        name = m.group(1)
        if not name or name in CPP_KEYWORDS:
            continue
        if name.upper() in WIN_API_OK:
            continue
        if not is_camel(name):
            report(file, line_no, m.start(1), '参数命名', '应使用 camelCase：rendererFactory', '参数命名')
        if has_abbr(name):
            report(file, line_no, m.start(1), '参数含缩写', '禁止使用缩写', '缩写检查')

def check_namespace(line, file, line_no):
    """检查命名空间命名"""
    m = RE_NAMESPACE.search(line)
    if not m:
        return
    name = m.group(1)
    if name and name not in {'std', 'detail', 'internal'} and not is_camel(name) and not is_pascal(name):
        report(file, line_no, m.start(1), '命名空间命名', '应使用 camelCase 或 PascalCase', '命名空间命名')

def check_macro(line, file, line_no):
    """检查宏定义命名"""
    m = RE_MACRO.search(line)
    if not m:
        return
    name = m.group(1)
    if name and not is_upper_snake(name):
        # 允许一些常见的宏
        if name not in {'WIN32_LEAN_AND_MEAN', 'NOMINMAX', 'VK_USE_PLATFORM_WIN32_KHR'}:
            report(file, line_no, m.start(1), '宏定义命名', '应使用 UPPER_SNAKE_CASE：MAX_SIZE', '宏命名')

def check_typedef(line, file, line_no):
    """检查 typedef 命名"""
    m = RE_TYPEDEF.search(line)
    if not m:
        return
    name = m.group(1)
    if name and not is_pascal(name):
        report(file, line_no, m.start(1), '类型别名命名', '应使用 PascalCase', '类型命名')

def check_using(line, file, line_no):
    """检查 using 别名命名"""
    m = RE_USING.search(line)
    if not m:
        return
    name = m.group(1)
    if name and not is_pascal(name):
        report(file, line_no, m.start(1), 'using 别名命名', '应使用 PascalCase', '类型命名')

# ---------- 入口 ----------
def scan_file(path: Path):
    """扫描单个文件"""
    in_enum = False
    try:
        with path.open(encoding='utf-8', errors='ignore') as f:
            for no, line in enumerate(f, 1):
                line = line.rstrip()
                # 跳过空行和注释
                if not line or line.strip().startswith('//'):
                    continue
                # 跳过预处理指令（除了宏定义）
                if line.strip().startswith('#') and not line.strip().startswith('#define'):
                    check_macro(line, path, no)
                    continue
                
                # 检查是否进入/退出枚举块
                if 'enum' in line and '{' in line:
                    in_enum = True
                if in_enum and '}' in line:
                    in_enum = False
                
                check_class(line, path, no)
                check_enum(line, path, no)
                if in_enum:
                    check_enum_value(line, path, no)
                check_member(line, path, no)
                check_const_member(line, path, no)
                check_func(line, path, no)
                check_param(line, path, no)
                check_namespace(line, path, no)
                check_macro(line, path, no)
                check_typedef(line, path, no)
                check_us
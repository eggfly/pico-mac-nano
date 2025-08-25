#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
ROM 提取脚本
从固件文件中提取 128KB 的 ROM 数据
通过查找 ROM 结尾标记 (39 38 35 10) 来定位 ROM 结束位置
"""

import os
import sys
import argparse
from pathlib import Path

def find_rom_end_pattern(firmware_path):
    """查找固件中的 ROM 结尾模式 (39 38 35 10)"""
    print(f"正在搜索 ROM 结尾模式 (39 38 35 10) 在文件: {firmware_path}")
    
    end_positions = []
    pattern = b'\x39\x38\x35\x10'  # "985."
    
    try:
        with open(firmware_path, 'rb') as f:
            data = f.read()
            
        # 查找所有匹配位置
        pos = 0
        while True:
            pos = data.find(pattern, pos)
            if pos == -1:
                break
            end_positions.append(pos + 4)  # 包含整个模式，所以结束位置是 pos + 4
            pos += 1
            
        print(f"找到 {len(end_positions)} 个 ROM 结尾模式位置:")
        for i, pos in enumerate(end_positions):
            print(f"  {i+1}. 偏移: {pos} (0x{pos:08x})")
            
        return end_positions
        
    except Exception as e:
        print(f"错误: 无法读取文件 {firmware_path}: {e}")
        return []

def extract_rom(firmware_path, output_path, rom_end_position, rom_size=131072):
    """从固件中提取 ROM 数据"""
    print(f"\n正在提取 ROM 数据...")
    print(f"ROM 结尾位置: {rom_end_position} (0x{rom_end_position:08x})")
    print(f"ROM 大小: {rom_size} 字节 ({rom_size//1024} KB)")
    
    # 计算 ROM 开始位置 (ROM 结尾位置往前 rom_size 字节)
    rom_start = rom_end_position - rom_size
    
    if rom_start < 0:
        print(f"错误: ROM 开始位置为负数 ({rom_start})，文件可能太小")
        return False
    
    print(f"ROM 开始位置: {rom_start} (0x{rom_start:08x})")
    print(f"ROM 结束位置: {rom_end_position} (0x{rom_end_position:08x})")
    
    try:
        with open(firmware_path, 'rb') as f:
            f.seek(rom_start)
            rom_data = f.read(rom_size)
            
        if len(rom_data) != rom_size:
            print(f"警告: 读取的数据大小 ({len(rom_data)}) 与期望大小 ({rom_size}) 不匹配")
            return False
            
        # 写入输出文件
        with open(output_path, 'wb') as f:
            f.write(rom_data)
            
        print(f"✓ ROM 数据已成功提取到: {output_path}")
        print(f"提取大小: {len(rom_data)} 字节 ({len(rom_data)//1024} KB)")
        
        # 显示 ROM 数据的开头和结尾
        print(f"\nROM 数据开头 (前 32 字节):")
        print(' '.join(f'{b:02x}' for b in rom_data[:32]))
        
        print(f"\nROM 数据结尾 (后 32 字节):")
        print(' '.join(f'{b:02x}' for b in rom_data[-32:]))
        
        return True
        
    except Exception as e:
        print(f"错误: 提取 ROM 数据时出错: {e}")
        return False

def verify_rom_data(rom_path):
    """验证提取的 ROM 数据"""
    print(f"\n正在验证 ROM 数据: {rom_path}")
    
    try:
        with open(rom_path, 'rb') as f:
            data = f.read()
            
        print(f"文件大小: {len(data)} 字节 ({len(data)//1024} KB)")
        
        # 检查文件大小是否为 128KB
        if len(data) == 131072:
            print("✓ 文件大小正确 (128KB)")
        else:
            print(f"⚠ 文件大小不正确，期望 131072 字节，实际 {len(data)} 字节")
            
        # 检查文件末尾是否包含 ROM 结尾模式
        if data.endswith(b'\x39\x38\x35\x10'):
            print("✓ 文件末尾包含 ROM 结尾模式 (39 38 35 10)")
        else:
            print("⚠ 文件末尾不包含预期的 ROM 结尾模式")
            # 显示文件末尾的字节
            print(f"文件末尾字节: {' '.join(f'{b:02x}' for b in data[-16:])}")
            
        return True
        
    except Exception as e:
        print(f"错误: 验证 ROM 数据时出错: {e}")
        return False

def main():
    parser = argparse.ArgumentParser(description='从固件文件中提取 ROM 数据')
    parser.add_argument('firmware', help='固件文件路径')
    parser.add_argument('-o', '--output', default='rom.bin', help='输出 ROM 文件路径 (默认: rom.bin)')
    parser.add_argument('-s', '--size', type=int, default=131072, help='ROM 大小 (默认: 131072 字节)')
    parser.add_argument('--rom-end', type=int, help='手动指定 ROM 结尾位置 (十六进制)')
    
    args = parser.parse_args()
    
    firmware_path = Path(args.firmware)
    output_path = Path(args.output)
    
    # 检查输入文件
    if not firmware_path.exists():
        print(f"错误: 固件文件不存在: {firmware_path}")
        sys.exit(1)
        
    print("=== ROM 提取工具 ===")
    print(f"固件文件: {firmware_path}")
    print(f"输出文件: {output_path}")
    print(f"ROM 大小: {args.size} 字节 ({args.size//1024} KB)")
    
    # 如果手动指定了 ROM 结尾位置
    if args.rom_end:
        rom_end_position = args.rom_end
        print(f"使用手动指定的 ROM 结尾位置: {rom_end_position} (0x{rom_end_position:08x})")
    else:
        # 自动查找 ROM 结尾模式
        end_positions = find_rom_end_pattern(firmware_path)
        
        if not end_positions:
            print("错误: 未找到 ROM 结尾模式 (39 38 35 10)")
            sys.exit(1)
            
        if len(end_positions) > 1:
            print(f"\n找到多个 ROM 结尾模式，使用第一个位置: {end_positions[0]}")
            
        rom_end_position = end_positions[0]
    
    # 提取 ROM 数据
    if extract_rom(firmware_path, output_path, rom_end_position, args.size):
        # 验证提取的数据
        verify_rom_data(output_path)
        print(f"\n=== ROM 提取完成 ===")
    else:
        print(f"\n=== ROM 提取失败 ===")
        sys.exit(1)

if __name__ == '__main__':
    main()

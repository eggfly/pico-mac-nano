#!/bin/bash

# ROM提取脚本
# 从固件文件中提取128KB的ROM数据

INPUT_FILE="/Users/eggfly/github/pico-mac-all/pico-mac-nano/firmware/firmware_3_2.bin"
OUTPUT_FILE="extracted_rom.bin"
ROM_SIZE=131072  # 128KB in bytes

echo "=== ROM提取工具 ==="
echo "输入文件: $INPUT_FILE"

# 检查输入文件是否存在
if [ ! -f "$INPUT_FILE" ]; then
    echo "错误: 输入文件不存在!"
    exit 1
fi

# 获取文件大小
FILE_SIZE=$(stat -f%z "$INPUT_FILE")
echo "文件大小: $FILE_SIZE 字节"

# 使用已知的正确交界位置
echo ""
echo "使用已知的ROM和disc交界位置..."
PATTERN_OFFSET=430520  # 0x691b8 - 已知的交界位置（完整模式开始）

echo "已知交界位置偏移: $PATTERN_OFFSET (0x$(printf '%x' $PATTERN_OFFSET))"

# 计算ROM开始位置（从文件开头到交界模式开始）
ROM_START=0
ROM_END=$PATTERN_OFFSET  # ROM结束于交界模式开始处

echo ""
echo "ROM位置信息:"
echo "ROM开始偏移: $ROM_START (0x$(printf '%x' $ROM_START))"
echo "ROM结束偏移: $ROM_END (0x$(printf '%x' $ROM_END))"
echo "计算出的ROM大小: $((ROM_END - ROM_START)) 字节"

# 验证ROM大小
ACTUAL_ROM_SIZE=$((ROM_END - ROM_START))
echo "实际ROM大小: $ACTUAL_ROM_SIZE 字节 ($((ACTUAL_ROM_SIZE / 1024)) KB)"

if [ $ACTUAL_ROM_SIZE -eq $ROM_SIZE ]; then
    echo "✓ ROM大小正确 (128KB)"
else
    echo "⚠ ROM大小不匹配，期望: $ROM_SIZE 字节，实际: $ACTUAL_ROM_SIZE 字节"
    echo "使用实际计算出的ROM大小进行提取..."
fi

# 验证开始位置的数据
echo ""
echo "验证ROM开始位置的数据..."
ROM_START_DATA=$(hexdump -C "$INPUT_FILE" | head -4)
echo "ROM开始位置数据:"
echo "$ROM_START_DATA"

# 验证结束位置的数据
echo ""
echo "验证ROM结束位置的数据..."
ROM_END_HEX=$(printf '%08x' $ROM_END)
ROM_END_DATA=$(hexdump -C "$INPUT_FILE" | grep -A 2 -B 2 "$ROM_END_HEX" | head -5)
echo "ROM结束位置数据:"
echo "$ROM_END_DATA"

# 提取ROM数据
echo ""
echo "正在提取ROM数据..."
dd if="$INPUT_FILE" of="$OUTPUT_FILE" bs=1 skip=$ROM_START count=$ACTUAL_ROM_SIZE 2>/dev/null

if [ $? -eq 0 ]; then
    EXTRACTED_SIZE=$(stat -f%z "$OUTPUT_FILE")
    echo "提取完成!"
    echo "输出文件: $OUTPUT_FILE"
    echo "提取大小: $EXTRACTED_SIZE 字节"
    echo "提取大小: $((EXTRACTED_SIZE / 1024)) KB"
    
    # 显示提取文件的前几行
    echo ""
    echo "提取文件的前64字节:"
    hexdump -C "$OUTPUT_FILE" | head -4
    
    # 显示提取文件的末尾
    echo ""
    echo "提取文件的末尾64字节:"
    hexdump -C "$OUTPUT_FILE" | tail -4
    
    # 验证提取的数据
    echo ""
    echo "验证提取的数据..."
    
    # 检查文件大小
    if [ $EXTRACTED_SIZE -eq $ACTUAL_ROM_SIZE ]; then
        echo "✓ 文件大小正确 ($ACTUAL_ROM_SIZE 字节)"
    else
        echo "⚠ 文件大小不正确，期望: $ACTUAL_ROM_SIZE 字节，实际: $EXTRACTED_SIZE 字节"
    fi
    
    # 检查文件末尾是否包含交界模式
    echo ""
    echo "检查文件末尾是否包含交界模式..."
    TAIL_BYTES=$(hexdump -C "$OUTPUT_FILE" | tail -2 | tr -d ' ' | cut -c11-50)
    echo "文件末尾字节: $TAIL_BYTES"
    
    if echo "$TAIL_BYTES" | grep -q "4c4b"; then
        echo "✓ 文件末尾包含交界标记 (4C4B)"
    else
        echo "⚠ 文件末尾可能不包含预期的交界标记"
    fi
    
else
    echo "提取失败!"
    exit 1
fi

# 清理临时文件
rm -f /tmp/search_pattern.bin /tmp/partial_pattern.bin

echo ""
echo "=== ROM提取完成 ==="

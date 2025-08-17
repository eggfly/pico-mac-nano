#!/bin/bash

# 简单的ROM提取脚本
# 找到LK位置，往前取128KB

INPUT_FILE="/Users/eggfly/github/pico-mac-all/pico-mac-nano/firmware/firmware_3_2.bin"
OUTPUT_FILE="extracted_rom.bin"
ROM_SIZE=131072  # 128KB

echo "=== 简单ROM提取工具 ==="
echo "输入文件: $INPUT_FILE"

# 检查输入文件是否存在
if [ ! -f "$INPUT_FILE" ]; then
    echo "错误: 输入文件不存在!"
    exit 1
fi

# 使用已知的正确LK位置
echo "使用已知的LK位置..."
LK_OFFSET=430584  # 0x691f8 - 已知的LK位置

echo "LK位置: $LK_OFFSET (0x$(printf '%x' $LK_OFFSET))"

# 计算ROM开始位置 (LK位置往前128KB)
ROM_START=$((LK_OFFSET - ROM_SIZE))

echo ""
echo "ROM位置信息:"
echo "LK位置: $LK_OFFSET (0x$(printf '%x' $LK_OFFSET))"
echo "ROM开始: $ROM_START (0x$(printf '%x' $ROM_START))"
echo "ROM结束: $((ROM_START + ROM_SIZE)) (0x$(printf '%x' $((ROM_START + ROM_SIZE))))"
echo "ROM大小: $ROM_SIZE 字节 (128KB)"

# 提取ROM数据
echo ""
echo "正在提取ROM数据..."
dd if="$INPUT_FILE" of="$OUTPUT_FILE" bs=1 skip=$ROM_START count=$ROM_SIZE 2>/dev/null

if [ $? -eq 0 ]; then
    EXTRACTED_SIZE=$(stat -f%z "$OUTPUT_FILE")
    echo "提取完成!"
    echo "输出文件: $OUTPUT_FILE"
    echo "提取大小: $EXTRACTED_SIZE 字节"
    
    if [ $EXTRACTED_SIZE -eq $ROM_SIZE ]; then
        echo "✓ 文件大小正确 (128KB)"
    else
        echo "⚠ 文件大小不正确，期望: $ROM_SIZE 字节，实际: $EXTRACTED_SIZE 字节"
    fi
    
    # 显示提取文件的前几行和末尾
    echo ""
    echo "提取文件的前64字节:"
    hexdump -C "$OUTPUT_FILE" | head -4
    
    echo ""
    echo "提取文件的末尾64字节:"
    hexdump -C "$OUTPUT_FILE" | tail -4
    
else
    echo "提取失败!"
    exit 1
fi

echo ""
echo "=== ROM提取完成 ==="

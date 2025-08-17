#!/bin/bash

# 修正版本的umac_disc数据提取脚本
# 使用正确的开始位置（跳过ROM末尾标记）

INPUT_FILE="/Users/eggfly/github/pico-mac-all/pico-mac-nano/firmware/firmware_3_2.bin"
OUTPUT_FILE="extracted_corrected_disc_image.bin"

echo "=== 修正版磁盘镜像提取工具 ==="
echo "输入文件: $INPUT_FILE"

# 检查输入文件是否存在
if [ ! -f "$INPUT_FILE" ]; then
    echo "错误: 输入文件不存在!"
    exit 1
fi

# 获取文件大小
FILE_SIZE=$(stat -f%z "$INPUT_FILE")
echo "文件大小: $FILE_SIZE 字节"

# 修正后的位置
DISC_START_OFFSET=430584  # 0x691f8 - 跳过ROM末尾标记后的正确开始位置
DISC_END_OFFSET=1740888   # 计算出的umac_disc结束位置

echo ""
echo "修正后的位置:"
echo "umac_disc开始偏移: $DISC_START_OFFSET (0x$(printf '%x' $DISC_START_OFFSET))"
echo "umac_disc结束偏移: $DISC_END_OFFSET (0x$(printf '%x' $DISC_END_OFFSET))"

# 计算DISC大小
DISC_SIZE=$((DISC_END_OFFSET - DISC_START_OFFSET))
echo "umac_disc大小: $DISC_SIZE 字节"
echo "umac_disc大小: $((DISC_SIZE / 1024)) KB"
echo "umac_disc大小: $((DISC_SIZE / 1024 / 1024)) MB"

# 验证开始位置的数据
echo ""
echo "验证umac_disc开始位置的数据..."
DISC_START_DATA=$(hexdump -C "$INPUT_FILE" | grep -A 2 -B 2 "$(printf '%08x' $DISC_START_OFFSET)" | head -5)
echo "umac_disc开始位置数据:"
echo "$DISC_START_DATA"

# 提取umac_disc数据
echo ""
echo "正在提取umac_disc数据..."
dd if="$INPUT_FILE" of="$OUTPUT_FILE" bs=1 skip=$DISC_START_OFFSET count=$DISC_SIZE 2>/dev/null

if [ $? -eq 0 ]; then
    EXTRACTED_SIZE=$(stat -f%z "$OUTPUT_FILE")
    echo "提取完成!"
    echo "输出文件: $OUTPUT_FILE"
    echo "提取大小: $EXTRACTED_SIZE 字节"
    echo "提取大小: $((EXTRACTED_SIZE / 1024)) KB"
    echo "提取大小: $((EXTRACTED_SIZE / 1024 / 1024)) MB"
    
    # 显示提取文件的前几行
    echo ""
    echo "提取文件的前64字节:"
    hexdump -C "$OUTPUT_FILE" | head -4
    
    # 显示提取文件的末尾
    echo ""
    echo "提取文件的末尾64字节:"
    hexdump -C "$OUTPUT_FILE" | tail -4
    
    # 验证提取的数据是否以正确的标记开始
    echo ""
    echo "验证提取的数据..."
    FIRST_BYTES=$(hexdump -C "$OUTPUT_FILE" | head -1 | cut -c11-26)
    echo "文件开头字节: $FIRST_BYTES"
    
    if echo "$FIRST_BYTES" | grep -q "4c 4b 60 00"; then
        echo "✓ 文件以正确的umac_disc标记开始 (LK...)"
    else
        echo "⚠ 文件开头可能不正确"
    fi
    
    # 检查文件大小是否合理
    if [ $EXTRACTED_SIZE -gt 1000000 ] && [ $EXTRACTED_SIZE -lt 2000000 ]; then
        echo "✓ 文件大小在合理范围内 (1-2MB)"
    else
        echo "⚠ 文件大小可能不在预期范围内"
    fi
else
    echo "提取失败!"
    exit 1
fi

echo ""
echo "=== 提取完成 ==="

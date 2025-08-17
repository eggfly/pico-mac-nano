#!/bin/bash

# 从firmware_3_2.bin中提取完整的umac_disc数据
# 基于map文件分析和十六进制标记

INPUT_FILE="/Users/eggfly/github/pico-mac-all/pico-mac-nano/firmware/firmware_3_2.bin"
OUTPUT_FILE="extracted_full_disc_image.bin"

# 从map文件分析得到的地址
UMAC_ROM_START=0x10049650
UMAC_ROM_END=0x10069650
UMAC_DISC_START=0x10069650

# 转换为二进制文件中的偏移量（减去FLASH基地址0x10000000）
ROM_START_OFFSET=$((UMAC_ROM_START - 0x10000000))
ROM_END_OFFSET=$((UMAC_ROM_END - 0x10000000))
DISC_START_OFFSET=$((UMAC_DISC_START - 0x10000000))

echo "=== 完整磁盘镜像提取工具 v2 ==="
echo "输入文件: $INPUT_FILE"
echo "ROM开始偏移: 0x$(printf '%x' $ROM_START_OFFSET) ($ROM_START_OFFSET)"
echo "ROM结束偏移: 0x$(printf '%x' $ROM_END_OFFSET) ($ROM_END_OFFSET)"
echo "DISC开始偏移: 0x$(printf '%x' $DISC_START_OFFSET) ($DISC_START_OFFSET)"

# 检查输入文件是否存在
if [ ! -f "$INPUT_FILE" ]; then
    echo "错误: 输入文件不存在!"
    exit 1
fi

# 获取文件大小
FILE_SIZE=$(stat -f%z "$INPUT_FILE")
echo "文件大小: $FILE_SIZE 字节"

# 计算umac_disc的大小（从开始位置到文件末尾）
DISC_SIZE=$((FILE_SIZE - DISC_START_OFFSET))
echo "可提取的DISC大小: $DISC_SIZE 字节"

# 验证ROM结束位置
echo ""
echo "验证ROM结束位置..."
ROM_END_DATA=$(hexdump -C "$INPUT_FILE" | grep -A 1 -B 1 "$(printf '%08x' $ROM_END_OFFSET)" | head -3)
echo "ROM结束位置数据:"
echo "$ROM_END_DATA"

# 提取umac_disc数据
echo ""
echo "正在提取完整的umac_disc数据..."
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
else
    echo "提取失败!"
    exit 1
fi

echo ""
echo "=== 提取完成 ==="

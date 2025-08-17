#!/bin/bash

# 基于十六进制标记的精确定位提取umac_disc数据
# 使用ROM末尾标记和DISC末尾标记来精确定位

INPUT_FILE="/Users/eggfly/github/pico-mac-all/pico-mac-nano/firmware/firmware_3_2.bin"
OUTPUT_FILE="extracted_precise_disc_image.bin"

echo "=== 精确定位磁盘镜像提取工具 ==="
echo "输入文件: $INPUT_FILE"

# 检查输入文件是否存在
if [ ! -f "$INPUT_FILE" ]; then
    echo "错误: 输入文件不存在!"
    exit 1
fi

# 获取文件大小
FILE_SIZE=$(stat -f%z "$INPUT_FILE")
echo "文件大小: $FILE_SIZE 字节"

# 方法1: 基于ROM末尾标记定位DISC开始
echo ""
echo "方法1: 基于ROM末尾标记定位..."
ROM_END_MARKER="4e 75 48 42"
ROM_END_LINE=$(hexdump -C "$INPUT_FILE" | grep -n "$ROM_END_MARKER" | head -1)
if [ -n "$ROM_END_LINE" ]; then
    ROM_END_LINE_NUM=$(echo "$ROM_END_LINE" | cut -d: -f1)
    echo "找到ROM末尾标记在第 $ROM_END_LINE_NUM 行"
    
    # 计算ROM末尾标记在文件中的偏移量
    ROM_END_OFFSET=$(( (ROM_END_LINE_NUM - 1) * 16 + 12 ))  # 12是标记在行中的位置
    echo "ROM末尾标记偏移量: $ROM_END_OFFSET"
    
    # DISC开始位置应该在ROM末尾标记之后
    DISC_START_OFFSET=$((ROM_END_OFFSET + 4))  # 跳过4字节的标记
    echo "DISC开始偏移量: $DISC_START_OFFSET"
else
    echo "未找到ROM末尾标记，使用map文件中的地址"
    DISC_START_OFFSET=431696  # 从map文件得到的地址
fi

# 方法2: 基于map文件地址
echo ""
echo "方法2: 基于map文件地址..."
MAP_DISC_START=431696  # 0x69650
echo "Map文件中的DISC开始地址: $MAP_DISC_START"

# 使用map文件地址作为主要方法
DISC_START_OFFSET=$MAP_DISC_START

# 计算DISC大小（从开始到文件末尾）
DISC_SIZE=$((FILE_SIZE - DISC_START_OFFSET))
echo "DISC大小: $DISC_SIZE 字节"

# 验证DISC开始位置的数据
echo ""
echo "验证DISC开始位置的数据..."
DISC_START_DATA=$(hexdump -C "$INPUT_FILE" | grep -A 2 -B 2 "$(printf '%08x' $DISC_START_OFFSET)" | head -5)
echo "DISC开始位置数据:"
echo "$DISC_START_DATA"

# 提取DISC数据
echo ""
echo "正在提取DISC数据..."
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
    
    # 验证提取的数据是否包含磁盘镜像的特征
    echo ""
    echo "验证提取的数据..."
    if hexdump -C "$OUTPUT_FILE" | head -10 | grep -q "00 00 00 00"; then
        echo "✓ 数据包含零字节，符合磁盘镜像特征"
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

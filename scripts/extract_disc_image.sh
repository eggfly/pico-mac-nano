#!/bin/bash

# 提取磁盘镜像数据的脚本
# 基于标记字节序列 ff ff ff ff ca fe ba be

INPUT_FILE="/Users/eggfly/github/pico-mac-all/pico-mac-nano/build/0xcafebabe_firmware.bin"
OUTPUT_FILE="extracted_disc_image.bin"
MARKER_OFFSET=0x69650  # 标记字节序列的位置

echo "=== 磁盘镜像提取工具 ==="
echo "输入文件: $INPUT_FILE"
echo "标记位置: $MARKER_OFFSET ($(($MARKER_OFFSET)) 字节)"

# 检查输入文件是否存在
if [ ! -f "$INPUT_FILE" ]; then
    echo "错误: 输入文件不存在!"
    exit 1
fi

# 获取文件大小
FILE_SIZE=$(stat -f%z "$INPUT_FILE")
echo "文件大小: $FILE_SIZE 字节"

# 计算从标记位置到文件末尾的大小
DATA_SIZE=$((FILE_SIZE - MARKER_OFFSET))
echo "可提取数据大小: $DATA_SIZE 字节"

# 提取数据（跳过8字节的标记）
echo "正在提取磁盘镜像数据..."
dd if="$INPUT_FILE" of="$OUTPUT_FILE" bs=1 skip=$((MARKER_OFFSET + 8)) count=$((DATA_SIZE - 8)) 2>/dev/null

if [ $? -eq 0 ]; then
    EXTRACTED_SIZE=$(stat -f%z "$OUTPUT_FILE")
    echo "提取完成!"
    echo "输出文件: $OUTPUT_FILE"
    echo "提取大小: $EXTRACTED_SIZE 字节"
    
    # 显示提取文件的前几行
    echo ""
    echo "提取文件的前32字节:"
    hexdump -C "$OUTPUT_FILE" | head -2
else
    echo "提取失败!"
    exit 1
fi

echo ""
echo "=== 提取完成 ==="

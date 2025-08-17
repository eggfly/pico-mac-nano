/*
 * 1BPP ASCII字库
 * 支持8x8和6x8两种字体
 * 
 * Copyright 2024
 */

#ifndef FONT_H
#define FONT_H

#include <stdint.h>
#include <stdbool.h>

// 字库数组声明
// extern const uint8_t font_8x8[128][8];
// extern const uint8_t font_6x8[128][6];

// 绘制函数声明
void draw_char_8x8(uint32_t *framebuffer, int x, int y, char c, int color);
void draw_string_8x8(uint32_t *framebuffer, int x, int y, const char *str, int color);
void draw_char_6x8(uint32_t *framebuffer, int x, int y, char c, int color);
void draw_string_6x8(uint32_t *framebuffer, int x, int y, const char *str, int color);

// 字符映射函数声明
unsigned char map_unicode_to_ascii(unsigned int unicode);

// 测试函数声明
void test_special_chars(uint32_t *framebuffer);
void test_char_mapping(void);

// 优化的像素绘制函数 - 内联版本（用于正常字节序）
static inline void draw_pixel(uint32_t *framebuffer, int x, int y, bool blackOrWhite) {
    // 边界检查：宽度512，高度207，高度限制就是207不要给我乱改
    if (x < 0 || x >= 512 || y < 0 || y >= 207) {
        return; // 超出边界直接返回
    }
    
    // 计算word索引和bit索引
    int pixel_index = y * 512 + x;
    int word_index = pixel_index / 32;
    int bit_index = pixel_index % 32;
    
    // 额外的framebuffer边界检查，高度限制就是207不要给我乱改
    if (word_index >= 0 && word_index < (512 * 207) / 32) {
        if (blackOrWhite) {
            // 设置为黑色（1）
            framebuffer[word_index] |= (1 << (31 - bit_index));
        } else {
            // 设置为白色（0）
            framebuffer[word_index] &= ~(1 << (31 - bit_index));
        }
    }
}

// 字节序交换的像素绘制函数 - 内联版本（用于Mac framebuffer布局）
static inline void draw_pixel_swapped(uint32_t *framebuffer, int x, int y, bool blackOrWhite) {
    // 边界检查：宽度512，高度207，高度限制就是207不要给我乱改
    if (x < 0 || x >= 512 || y < 0 || y >= 207) {
        return; // 超出边界直接返回
    }
    
    // 计算word索引和bit索引
    int pixel_index = y * 512 + x;
    int word_index = pixel_index / 32;
    int bit_index = pixel_index % 32;
    
    // 额外的framebuffer边界检查，高度限制就是207不要给我乱改
    if (word_index >= 0 && word_index < (512 * 207) / 32) {
        if (blackOrWhite) {
            // 设置为黑色（1）- 字节序交换版本
            // 由于DMA会进行字节序交换，我们需要调整字节的顺序
            // 字节序交换会将 0x12345678 变成 0x78563412
            // 这意味着4个字节的顺序被反转了，但每个字节内部的8个像素顺序保持不变
            // 所以我们需要调整字节索引，但保持字节内的位索引不变
            int byte_index = bit_index / 8;  // 0,1,2,3
            int bit_in_byte = bit_index % 8; // 0-7
            int new_byte_index = 3 - byte_index; // 3,2,1,0
            int swapped_bit_index = new_byte_index * 8 + bit_in_byte;
            framebuffer[word_index] |= (1 << (31 - swapped_bit_index));
        } else {
            // 设置为白色（0）- 字节序交换版本
            int byte_index = bit_index / 8;  // 0,1,2,3
            int bit_in_byte = bit_index % 8; // 0-7
            int new_byte_index = 3 - byte_index; // 3,2,1,0
            int swapped_bit_index = new_byte_index * 8 + bit_in_byte;
            framebuffer[word_index] &= ~(1 << (31 - swapped_bit_index));
        }
    }
}

// 绘制垂直线函数 - 内联版本
static inline void drawVLine(uint32_t *framebuffer, int x, int y1, int y2, bool blackOrWhite) {
    // 确保y1 <= y2
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
    
    // 边界检查
    if (x < 0 || x >= 512) {
        return; // x超出边界
    }
    
    // 限制y范围在有效区域内
    if (y1 < 0) y1 = 0;
    if (y2 >= 342) y2 = 341;
    
    // 绘制垂直线
    for (int y = y1; y <= y2; y++) {
        draw_pixel_swapped(framebuffer, x, y, blackOrWhite);
    }
}

// 绘制水平线函数 - 内联版本
static inline void drawHLine(uint32_t *framebuffer, int x1, int x2, int y, bool blackOrWhite) {
    // 确保x1 <= x2
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    
    // 边界检查
    if (y < 0 || y >= 342) {
        return; // y超出边界
    }
    
    // 限制x范围在有效区域内
    if (x1 < 0) x1 = 0;
    if (x2 >= 512) x2 = 511;
    
    // 绘制水平线
    for (int x = x1; x <= x2; x++) {
        draw_pixel_swapped(framebuffer, x, y, blackOrWhite);
    }
}

// 字体尺寸常量
#define FONT_8X8_WIDTH  8
#define FONT_8X8_HEIGHT 8
#define FONT_6X8_WIDTH  6
#define FONT_6X8_HEIGHT 8

#endif // FONT_H

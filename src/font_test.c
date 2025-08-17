/*
 * 字体测试程序
 * 用于测试特殊字符的显示
 */

#include "font.h"
#include <stdio.h>
#include <string.h>

// 测试特殊字符显示
void test_special_chars(uint32_t *framebuffer) {
    // 清屏
    memset(framebuffer, 0, 512 * 207 / 8);
    
    // 测试制表符字符
    draw_string_8x8(framebuffer, 0, 0*8, "┌──────────────────────────────────────────────────────────┐", 0);
    draw_string_8x8(framebuffer, 0, 1*8, "│  RETRO OS v3.1                                           │", 0);
    draw_string_8x8(framebuffer, 0, 2*8, "│  (C) 1989-2025                                           │", 0);
    draw_string_8x8(framebuffer, 0, 3*8, "│                                                          │", 0);
    draw_string_8x8(framebuffer, 0, 4*8, "│  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓                                   │", 0);
    draw_string_8x8(framebuffer, 0, 5*8, "│  Initializing devices... OK                              │", 0);
    draw_string_8x8(framebuffer, 0, 6*8, "│  Mounting filesystem... OK                               │", 0);
    draw_string_8x8(framebuffer, 0, 7*8, "│  Starting services...  OK                                │", 0);
    draw_string_8x8(framebuffer, 0, 8*8, "│                                                          │", 0);
    draw_string_8x8(framebuffer, 0, 9*8, "│  C:\\> _                                                  │", 0);
    draw_string_8x8(framebuffer, 0, 10*8, "└──────────────────────────────────────────────────────────┘", 0);
    
    // 测试其他特殊字符
    draw_string_8x8(framebuffer, 0, 12*8, "Special Characters Test:", 0);
    draw_string_8x8(framebuffer, 0, 13*8, "┌─┐", 0);
    draw_string_8x8(framebuffer, 0, 14*8, "│ │", 0);
    draw_string_8x8(framebuffer, 0, 15*8, "└─┘", 0);
    draw_string_8x8(framebuffer, 0, 16*8, "▓▓▓", 0);
    draw_string_8x8(framebuffer, 0, 17*8, "███", 0);
}

// 测试字符映射函数
void test_char_mapping() {
    printf("Testing character mapping:\n");
    printf("┌ -> 0x%02X\n", map_unicode_to_ascii(0x250C));
    printf("─ -> 0x%02X\n", map_unicode_to_ascii(0x2500));
    printf("┐ -> 0x%02X\n", map_unicode_to_ascii(0x2510));
    printf("│ -> 0x%02X\n", map_unicode_to_ascii(0x2502));
    printf("└ -> 0x%02X\n", map_unicode_to_ascii(0x2514));
    printf("┘ -> 0x%02X\n", map_unicode_to_ascii(0x2518));
    printf("▓ -> 0x%02X\n", map_unicode_to_ascii(0x2593));
    printf("█ -> 0x%02X\n", map_unicode_to_ascii(0x2588));
}

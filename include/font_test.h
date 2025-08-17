/*
 * 字库测试头文件
 * 
 * Copyright 2024
 */

#ifndef FONT_TEST_H
#define FONT_TEST_H

#include <stdint.h>

// 测试函数声明
void font_test_demo(uint32_t *framebuffer);
void draw_status_text(uint32_t *framebuffer, const char *text, int line);
void draw_menu(uint32_t *framebuffer);

#endif // FONT_TEST_H

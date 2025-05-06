/* Functions to use build it ws2812 Neopixel on the Waveshare Pico Zero:
 *
 * Copyright 2025 Nick Gillard
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hw.h"
// #include "ws2812.h"

/* Setup red and blue colour flags. */
uint8_t led_red = 0; /* Used for TFT SPI write */
uint8_t led_green = 0; /* Used for main loop */
uint8_t led_blue = 0;

static  uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  /* convert rgb flags to RGB values */
  if (r == 1) r = 255;
  if (g == 1) g = 255;
  if (b == 1) b = 255;
  return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

static inline void put_pixel(uint8_t r, uint8_t g, uint8_t b ) {
  pio_sm_put_blocking(pio1, 0, (urgb_u32(r, g, b) << 8u)); /* ws2812 takes 24 bit in order G R B. Shift 32bit value left 8 so data is in top 24 bits */
}

void put_pixel_red(uint8_t val) {
        led_red = val;
        put_pixel(led_red, led_green, led_blue );
}

void put_pixel_green(uint8_t val) {
        led_green = val;
        put_pixel(led_red, led_green, led_blue );
}

void put_pixel_blue(uint8_t val) {
        led_blue = val;
        put_pixel(led_red, led_green, led_blue );
}

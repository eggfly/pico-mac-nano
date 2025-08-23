// 华迪 480x320 横屏 2.0寸 TFT 屏幕

// /* Functions to output to 2.0" 480x640 (Portrait) SPI & RBG565 TFT panel:
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
#include "pico/time.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hw.h"
#include "ws2812.h" /* Needed for neo-pixel LED */
#include "tft_2p.h"

// TFT SPI Write command
static void tft_write_com(unsigned char comm) {
	static int i;

    put_pixel_red(1);
    gpio_put(TFT_SPI_CS, 0); // Chip select enabled
    gpio_put(TFT_SPI_MOSI, 0);
    gpio_put(TFT_SPI_CLK, 0);
    sleep_us(TFT_SPI_PAUSE);
    gpio_put(TFT_SPI_CLK, 1);

    for(i=0;i<8;i++) {
        if(comm & 0x80) {
            gpio_put(TFT_SPI_MOSI, 1);
        } else {
            gpio_put(TFT_SPI_MOSI, 0);
        }
        gpio_put(TFT_SPI_CLK, 0);
        sleep_us(TFT_SPI_PAUSE);
        gpio_put(TFT_SPI_CLK, 1);
        comm<<=1;
    }
    gpio_put(TFT_SPI_CS, 1); // Chip select disabled
    put_pixel_red(0);
}

// SPI Write Data
static void tft_write_dat(unsigned char datt) {
    static int j;

    put_pixel_red(1);
    gpio_put(TFT_SPI_CS, 0); // Chip select enabled

    // First send a 1 for bit 9
    gpio_put(TFT_SPI_MOSI, 1);
    gpio_put(TFT_SPI_CLK, 0);
    sleep_us(TFT_SPI_PAUSE);
    gpio_put(TFT_SPI_CLK, 1);

    for(j=0;j<8;j++) {
        if(datt & 0x80) { // true if left most bit is a 1. Bitwisecompare of datt with binary 10000000
            gpio_put(TFT_SPI_MOSI, 1);
        } else {
            gpio_put(TFT_SPI_MOSI, 0);
        }
        gpio_put(TFT_SPI_CLK, 0);
        sleep_us(TFT_SPI_PAUSE);
        gpio_put(TFT_SPI_CLK, 1);
        datt<<=1; // Bit shift left 1
    }

    gpio_put(TFT_SPI_CS, 1); // Chip select disabled
    put_pixel_red(0);
}

void tft_init() {
    
    gpio_init(TFT_RESET);
    gpio_set_dir(TFT_RESET, GPIO_OUT);

    // Initialize SPI pins
    gpio_init(TFT_SPI_CS);
    gpio_set_dir(TFT_SPI_CS, GPIO_OUT);
    gpio_put(TFT_SPI_CS, 1);// Initialize TFT CS pin high
    gpio_init(TFT_SPI_CLK);
    gpio_set_dir(TFT_SPI_CLK, GPIO_OUT);
    gpio_init(TFT_SPI_MOSI);
    gpio_set_dir(TFT_SPI_MOSI, GPIO_OUT);

    // sleep_ms(5);

    gpio_put(TFT_RESET, 1); // Set TFT Reset pin high
    sleep_ms(1);
    gpio_put(TFT_RESET, 0);  // Set TFT Reset pin low
    sleep_ms(1);
    gpio_put(TFT_RESET, 1);  // Set TFT Reset pin high
    sleep_ms(1);

    tft_write_com(0xFF);    // Command2 BKx Selection
    tft_write_dat(0x77);
    tft_write_dat(0x01);
    tft_write_dat(0x00);
    tft_write_dat(0x00);
    tft_write_dat(0x13);

    tft_write_com(0xEF);    // 
    tft_write_dat(0x08);

    tft_write_com(0xFF);    // Command2 BKx Selection
    tft_write_dat(0x77);
    tft_write_dat(0x01);
    tft_write_dat(0x00);
    tft_write_dat(0x00);
    tft_write_dat(0x10);

    tft_write_com(0xC0);    // LNESET: Display Line Setting
    tft_write_dat(0x2C);
    tft_write_dat(0x00);

    tft_write_com(0xC1);    // PORCTRL: Porch Control
    tft_write_dat(0x0D);
    tft_write_dat(0x02);

    tft_write_com(0xC2);    // INVSET: Inversion selection & Frame Rate Control
    tft_write_dat(0x31);
    tft_write_dat(0x05);

    tft_write_com(0xCC);    // 
    tft_write_dat(0x10);

    tft_write_com(0xB0);    // PVGAMCTRL: Positive Voltage Gamma Control
    tft_write_dat(0x0A);
    tft_write_dat(0x14);
    tft_write_dat(0x1B);
    tft_write_dat(0x0D);
    tft_write_dat(0x10);
    tft_write_dat(0x05);
    tft_write_dat(0x07);
    tft_write_dat(0x08);
    tft_write_dat(0x06);
    tft_write_dat(0x22);
    tft_write_dat(0x03);
    tft_write_dat(0x11);
    tft_write_dat(0x10);
    tft_write_dat(0xAD);
    tft_write_dat(0x31);
    tft_write_dat(0x1B);

    tft_write_com(0xB1);    // NVGAMCTRL: Negative Voltage Gamma Control
    tft_write_dat(0x0A);
    tft_write_dat(0x14);
    tft_write_dat(0x1B);
    tft_write_dat(0x0D);
    tft_write_dat(0x10);
    tft_write_dat(0x05);
    tft_write_dat(0x07);
    tft_write_dat(0x08);
    tft_write_dat(0x06);
    tft_write_dat(0x22);
    tft_write_dat(0x03);
    tft_write_dat(0x11);
    tft_write_dat(0x10);
    tft_write_dat(0xAD);
    tft_write_dat(0x31);
    tft_write_dat(0x1B);

    tft_write_com(0xFF);    // Command2 BKx Selection
    tft_write_dat(0x77);
    tft_write_dat(0x01);
    tft_write_dat(0x00);
    tft_write_dat(0x00);
    tft_write_dat(0x11);

    tft_write_com(0xB0);    // PVGAMCTRL: Positive Voltage Gamma Control
    tft_write_dat(0x50);

    tft_write_com(0xB1);    // NVGAMCTRL: Negative Voltage Gamma Control
    tft_write_dat(0x5E);

    tft_write_com(0xB2);    // VGHSS: VGH Voltage setting
    tft_write_dat(0x87);

    tft_write_com(0xB3);    // TESTCMD: TEST Command Setting
    tft_write_dat(0x80);

    tft_write_com(0xB5);    // VGLS: VGL Voltage setting
    tft_write_dat(0x47);

    tft_write_com(0xB7);    // PWCTRL1: Power Control 1
    tft_write_dat(0x85);

    tft_write_com(0xB8);    // DGMEN: Digital Gamma Enable
    tft_write_dat(0x21);

    tft_write_com(0xC1);    // PORCTRL: Porch Control
    tft_write_dat(0x78);

    tft_write_com(0xC2);    // INVSET: Inversion selection & Frame Rate Control
    tft_write_dat(0x78);

    tft_write_com(0xD0);    // MIPISET1: MIPI Setting 1
    tft_write_dat(0x88);

    tft_write_com(0xE0);    // SECTRL: Sunlight Readable Enhancement
    tft_write_dat(0x00);

    tft_write_com(0x1B);    // 
    tft_write_dat(0x02);

    tft_write_com(0xE1);    // NRCTRL: Noise Reduce Control
    tft_write_dat(0x08);
    tft_write_dat(0xA0);
    tft_write_dat(0x00);
    tft_write_dat(0x00);
    tft_write_dat(0x07);
    tft_write_dat(0xA0);
    tft_write_dat(0x00);
    tft_write_dat(0x00);
    tft_write_dat(0x00);
    tft_write_dat(0x44);
    tft_write_dat(0x44);

    tft_write_com(0xE2);    // SECTRL: Sharpness Control
    tft_write_dat(0x11);
    tft_write_dat(0x11);
    tft_write_dat(0x44);
    tft_write_dat(0x44);
    tft_write_dat(0x75);
    tft_write_dat(0xA0);
    tft_write_dat(0x00);
    tft_write_dat(0x00);
    tft_write_dat(0x74);
    tft_write_dat(0xA0);
    tft_write_dat(0x00);
    tft_write_dat(0x00);

    tft_write_com(0xE3);    // CCCTRL: Color Calibration Control
    tft_write_dat(0x00);
    tft_write_dat(0x00);
    tft_write_dat(0x11);
    tft_write_dat(0x11);

    tft_write_com(0xE4);    // SKCTRL: Skin Tone Preservation Control
    tft_write_dat(0x44);
    tft_write_dat(0x44);

    tft_write_com(0xE5);    // 
    tft_write_dat(0x0A);
    tft_write_dat(0x71);
    tft_write_dat(0xD8);
    tft_write_dat(0xA0);
    tft_write_dat(0x0C);
    tft_write_dat(0x73);
    tft_write_dat(0xD8);
    tft_write_dat(0xA0);
    tft_write_dat(0x0E);
    tft_write_dat(0x75);
    tft_write_dat(0xD8);
    tft_write_dat(0xA0);
    tft_write_dat(0x10);
    tft_write_dat(0x77);
    tft_write_dat(0xD8);
    tft_write_dat(0xA0);

    tft_write_com(0xE6);    // 
    tft_write_dat(0x00);
    tft_write_dat(0x00);
    tft_write_dat(0x11);
    tft_write_dat(0x11);

    tft_write_com(0xE7);    // 
    tft_write_dat(0x44);
    tft_write_dat(0x44);

    tft_write_com(0xE8);    // 
    tft_write_dat(0x09);
    tft_write_dat(0x70);
    tft_write_dat(0xD8);
    tft_write_dat(0xA0);
    tft_write_dat(0x0B);
    tft_write_dat(0x72);
    tft_write_dat(0xD8);
    tft_write_dat(0xA0);
    tft_write_dat(0x0D);
    tft_write_dat(0x74);
    tft_write_dat(0xD8);
    tft_write_dat(0xA0);
    tft_write_dat(0x0F);
    tft_write_dat(0x76);
    tft_write_dat(0xD8);
    tft_write_dat(0xA0);

    tft_write_com(0xEB);    // 
    tft_write_dat(0x02);
    tft_write_dat(0x00);
    tft_write_dat(0xE4);
    tft_write_dat(0xE4);
    tft_write_dat(0x88);
    tft_write_dat(0x00);
    tft_write_dat(0x40);

    tft_write_com(0xEC);    // 
    tft_write_dat(0x3C);
    tft_write_dat(0x00);

    tft_write_com(0xED);    // 
    tft_write_dat(0xAB);
    tft_write_dat(0x89);
    tft_write_dat(0x76);
    tft_write_dat(0x54);
    tft_write_dat(0x02);
    tft_write_dat(0xFF);
    tft_write_dat(0xFF);
    tft_write_dat(0xFF);
    tft_write_dat(0xFF);
    tft_write_dat(0xFF);
    tft_write_dat(0xFF);
    tft_write_dat(0x20);
    tft_write_dat(0x45);
    tft_write_dat(0x67);
    tft_write_dat(0x98);
    tft_write_dat(0xBA);

    tft_write_com(0xEF);    // 
    tft_write_dat(0x08);
    tft_write_dat(0x08);
    tft_write_dat(0x08);
    tft_write_dat(0x45);
    tft_write_dat(0x3F);
    tft_write_dat(0x54);

    tft_write_com(0xFF);    // Command2 BKx Selection
    tft_write_dat(0x77);
    tft_write_dat(0x01);
    tft_write_dat(0x00);
    tft_write_dat(0x00);
    tft_write_dat(0x13);

    tft_write_com(0xE8);    // 
    tft_write_dat(0x00);
    tft_write_dat(0x0E);

    tft_write_com(0xE8);    // 
    tft_write_dat(0x00);
    tft_write_dat(0x0C);

    tft_write_com(0xE8);    // 
    tft_write_dat(0x00);
    tft_write_dat(0x00);

    tft_write_com(0xFF);    // Command2 BKx Selection
    tft_write_dat(0x77);
    tft_write_dat(0x01);
    tft_write_dat(0x00);
    tft_write_dat(0x00);
    tft_write_dat(0x00);

    tft_write_com(0x3A);    // COLMOD: Interface Pixel Format
    tft_write_dat(0x55);    // RGB565

    tft_write_com(0x36);    // MADCTL: Memory Access Control
    tft_write_dat(0x48);

    tft_write_com(0x11);    // SLPOUT: Sleep Out
    sleep_ms(120);
    tft_write_com(0x29);    // DISPON: Display On
    tft_write_com(0x35);    // TEON: Tearing Effect Line On
    tft_write_dat(0x00);
 //   sleep_ms(120);

//   tft_write_com(0x23); // All pixels on (white)
 //   sleep_ms(120);
 //   tft_write_com(0x22); // All pixels off (black)
}

/* Video output:
 *
 * NJG: Modified to use 480x342 frame buffer output across top of 480x640 portrait LCD.
 * 
 * Using PIO[1], output the Mac 480x342 1BPP framebuffer to VGA/pins.  This is done
 * directly from the Mac framebuffer (without having to reformat in an intermediate
 * buffer).  The video output is 640x480, with the visible pixel data centred with
 * borders:  for analog VGA this is easy, as it just means increasing the horizontal
 * back porch/front porch (time between syncs and active video) and reducing the
 * display portion of a line.
 *
 * [1]: see pio_video.pio
 *
 * Copyright 2024 Matt Evans
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
#include "hardware/dma.h"
#include "hardware/gpio.h"
#include "hardware/structs/padsbank0.h"
#include "pio_video.pio.h"

#include "hw.h"
#include "font.h"

// 内存监控函数
static void print_memory_usage() {
    
    printf("==================\n");
}
// 1bpp 24x31 image, 1=row major, black=1, white=0
const uint8_t mac_icon[31*3] = {
        0x3f, 0xff, 0xfc,
        0x40, 0x00, 0x02,
        0x80, 0x00, 0x01,
        0x8f, 0xff, 0xf1,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x89, 0x09,
        0x90, 0x92, 0x09,
        0x90, 0x92, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x90, 0x00, 0x09,
        0x50, 0x00, 0x0a,
        0x4c, 0x00, 0x32,
        0x43, 0xff, 0xc2
    };

// 使用 draw_pixel 函数绘制 mac_icon，通过行列循环保证字节序
void draw_mac_icon(uint32_t *framebuffer, int start_x, int start_y) {
    const int icon_width = 24;
    const int icon_height = 31;
    
    // 遍历图标的每一行
    for (int row = 0; row < icon_height; row++) {
        // 计算当前行在 mac_icon 数组中的起始位置
        int byte_offset = row * 3; // 每行3个字节
        
        // 遍历图标的每一列
        for (int col = 0; col < icon_width; col++) {
            // 计算当前像素在字节中的位置
            int byte_index = col / 8;  // 0, 1, 2
            int bit_index = 7 - (col % 8);  // 从高位到低位，7,6,5,4,3,2,1,0
            
            // 获取对应的字节
            uint8_t pixel_byte = mac_icon[byte_offset + byte_index];
            
            // 提取像素值（1=黑色，0=白色）
            bool is_black = (pixel_byte & (1 << bit_index)) != 0;
            
            // 使用 draw_pixel 函数绘制像素，保证字节序
            draw_pixel(framebuffer, start_x + col, start_y + row, is_black);
        }
    }
}
    
////////////////////////////////////////////////////////////////////////////////
/* VESA VGA mode 640x480@60 */

/* The pixel clock _should_ be (125/2/25.175) (about 2.483) but that seems to
 * make my VGA-HDMI adapter sample weird, and pixels crawl.  Fudge a little,
 * looks better:
 */
#define VIDEO_PCLK_MULT         (2.5*2) /* Was (2.5*2) */
#define VIDEO_HSW               2  /* Was 96 */
#define VIDEO_HBP               63  /* Horizontal Back Porch */
#define VIDEO_HRES              480 /* Hor Resolution. changed from 640 */
#define VIDEO_HFP               32  /* Horizontal Front Porch */
#define VIDEO_H_TOTAL_NOSYNC    (VIDEO_HBP + VIDEO_HRES + VIDEO_HFP)
#define VIDEO_VSW               2
#define VIDEO_VBP               17  /* Vertical Back Porch. changed from 33 */
#define VIDEO_VRES              640 /* Ver Resolution. changed from 480 */
#define VIDEO_VFP               12  /* Vertical Front Porch */
#define VIDEO_V_TOTAL           (VIDEO_VSW + VIDEO_VBP + VIDEO_VRES + VIDEO_VFP)
/* The visible vertical span in the VGA output, [start, end) lines: */
#define VIDEO_V_VIS_START       (VIDEO_VSW + VIDEO_VBP)
#define VIDEO_V_VIS_END         (VIDEO_V_VIS_START + VIDEO_VRES)

#define VIDEO_FB_HRES           512 /* changed from 512 */
#define VIDEO_FB_VRES           342

/* The lines at which the FB data is actively output: */
#define VIDEO_FB_V_VIS_START    118      /* Was (VIDEO_V_VIS_START + ((VIDEO_VRES - VIDEO_FB_VRES)/2)) */
#define VIDEO_FB_V_VIS_END      (VIDEO_FB_V_VIS_START + VIDEO_FB_VRES)

/* Words of 1BPP pixel data per line; this dictates the length of the
 * video data DMA transfer:
 */
#define VIDEO_VISIBLE_WPL       (VIDEO_FB_HRES / 32) /* Words per line?? */

#if (VIDEO_HRES & 31)
#error "VIDEO_HRES: must be a multiple of 32b!"
#endif

////////////////////////////////////////////////////////////////////////////////
// Video DMA, framebuffer pointers
// 上面镂空 118, 下面高度180
static uint32_t video_null[VIDEO_VISIBLE_WPL * 207]; // TODO: 这个尺寸不重要，且看是否超过了__HeapLimit

static uint32_t *video_framebuffer;

/* DMA buffer containing 2 pairs of per-line config words, for VS and not-VS: */
static uint32_t video_dma_cfg[4];

/* 3 DMA channels are used.  The first to transfer data to PIO, and
 * the other two to transfer descriptors to the first channel.
 */
static uint8_t video_dmach_tx;
static uint8_t video_dmach_descr_cfg;
static uint8_t video_dmach_descr_data;

typedef struct {
        const void *raddr;
        void *waddr;
        uint32_t count;
        uint32_t ctrl;
} dma_descr_t;

static dma_descr_t video_dmadescr_cfg;
static dma_descr_t video_dmadescr_data;

static volatile unsigned int video_current_y = 0;

static int      __not_in_flash_func(video_get_visible_y)(unsigned int y) {
        if ((y >= VIDEO_FB_V_VIS_START) && (y < VIDEO_FB_V_VIS_END)) {
                return y - VIDEO_FB_V_VIS_START;
        } else {
                return -1;
        }
}

static const uint32_t   *__not_in_flash_func(video_line_addr)(unsigned int y)
{
        int vy = video_get_visible_y(y);
        if (vy >= 0)
                return (const uint32_t *)&video_framebuffer[vy * VIDEO_VISIBLE_WPL];
        else if (y<VIDEO_FB_V_VIS_START) {
                // 减去 27 参数是正确的，减去越大，越靠上
                return (const uint32_t *)&video_null[((signed int)y-27)*VIDEO_VISIBLE_WPL];
        }
        else {
                return (const uint32_t *)&video_null[(y-VIDEO_FB_V_VIS_END)*VIDEO_VISIBLE_WPL];
        }
}

static const uint32_t   *__not_in_flash_func(video_cfg_addr)(unsigned int y)
{
        return &video_dma_cfg[(y < VIDEO_VSW) ? 0 : 2];
}

static void    __not_in_flash_func(video_dma_prep_new)()
{
        /* The descriptor DMA read pointers have moved on; reset them.
         * The write pointers wrap so should be pointing to the
         * correct DMA regs.
         */
        dma_hw->ch[video_dmach_descr_cfg].read_addr = (uintptr_t)&video_dmadescr_cfg;
        dma_hw->ch[video_dmach_descr_cfg].transfer_count = 4;
        dma_hw->ch[video_dmach_descr_data].read_addr = (uintptr_t)&video_dmadescr_data;
        dma_hw->ch[video_dmach_descr_data].transfer_count = 4;

        /* Configure the two DMA descriptors, video_dmadescr_cfg and
         * video_dmadescr_data, to transfer from video config/data corresponding
         * to the current line.
         *
         * These descriptors will be used to program the video_dmach_tx channel,
         * pushing the buffer to PIO.
         *
         * This can be relatively relaxed, as it's triggered as line data
         * starts; we have until the end of the video line (when the descriptors
         * are retriggered) to program them.
         *
         * FIXME: this time could be used for something clever like split-screen
         * (e.g. info/text lines) constructed on-the-fly.
         */
        video_dmadescr_cfg.raddr = video_cfg_addr(video_current_y);
        video_dmadescr_data.raddr = video_line_addr(video_current_y);

        /* Frame done */
        if (++video_current_y >= VIDEO_V_TOTAL)
                video_current_y = 0;
}

static void     __not_in_flash_func(video_dma_irq)()
{
        /* The DMA IRQ occurs once the video portion of the line has been
         * triggered (not when the video transfer completes, but when the
         * descriptor transfer (that leads to the video transfer!) completes.
         * All we need to do is reconfigure the descriptors; the video DMA will
         * re-trigger the descriptors later.
         */
        if (dma_channel_get_irq0_status(video_dmach_descr_data)) {
                dma_channel_acknowledge_irq0(video_dmach_descr_data);
                video_dma_prep_new();
        }
}

static void video_prep_buffer()
{
        // 初始化video_null数组
        memset(video_null, 0xff, sizeof(video_null));

        // draw_string_8x8(video_null, 5, 10, "~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~!~", 0);
        // draw_string_8x8(video_null, 5, 20, "Hello World! Pico Mac Nano Project!", 0);
        // draw_string_8x8(video_null, 5, 30, "--------- Macintosh System 1-6 since 1984 ---------", 0);
        // 测试特殊字符显示
        draw_string_8x8(video_null, 0, 0*8, "┌──────────────────────────────────────────────────────────┐", 0);
        draw_string_8x8(video_null, 0, 1*8, "│  RETRO OS v3.1                                           │", 0);
        draw_string_8x8(video_null, 0, 2*8, "│  (C) 1989-2025                                           │", 0);
        draw_string_8x8(video_null, 0, 3*8, "│                                                          │", 0);
        draw_string_8x8(video_null, 0, 4*8, "│  ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓                                   │", 0);
        draw_string_8x8(video_null, 0, 5*8, "│  Initializing devices... OK                              │", 0);
        draw_string_8x8(video_null, 0, 6*8, "│  Mounting filesystem... OK                               │", 0);
        draw_string_8x8(video_null, 0, 7*8, "│  Starting services...  OK                                │", 0);
        draw_string_8x8(video_null, 0, 8*8, "│                                                          │", 0);
        draw_string_8x8(video_null, 0, 9*8, "│  C:\\> _                                                  │", 0); // 这里多了是因为斜杠的转义
        draw_string_8x8(video_null, 0, 10*8, "└──────────────────────────────────────────────────────────┘", 0);
        // draw_string_6x8(video_null, 60, 40, "!", 0);
        // draw_string_6x8(video_null, 60, 50, "P", 0);
        // 上方 0-90 = 91 高度？
        // 中间 342 高度
        // 底部 207高度？
        // 91+342+207 = 640
        // 高度 480, y=0的横线
        drawHLine(video_null, 0, 479, 0, false);
        // 高度 480, y=1的横线
        drawHLine(video_null, 0, 479, 1, false);
        // 宽度 480, y=100的横线
        drawHLine(video_null, 0, 479, 100, false);
        // 宽度 480, y=116的横线
        drawHLine(video_null, 0, 479, 80, false);
        // 宽度 480, y=117的横线
        drawHLine(video_null, 0, 479, 90, false);
        // 宽度 480, y=179的横线
        drawHLine(video_null, 0, 479, 179, false);
        // 宽度 480, y=205的横线
        drawHLine(video_null, 0, 479, 205, false);
        // 宽度 480, y=206的横线
        drawHLine(video_null, 0, 479, 206, false);
        // 宽度 480, y=207的横线
        drawHLine(video_null, 0, 479, 207, false);
        // 高度 0-180，x=0 的竖线
        drawVLine(video_null, 0, 0, 180, false);
        // // 高度 0-118，x=200 的竖线
        // drawVLine(video_null, 200, 0, 118, false);
        // // 高度 118-179，x=200 的竖线
        // drawVLine(video_null, 200, 118, 179, false);
        // 高度 179-342，x=200 的竖线
        drawVLine(video_null, 200, 179, 342, false);
        // for (int y = 0; y < VIDEO_FB_V_VIS_START; y++) {
        //         // x=200, 高度 118 的一个竖线
        //         draw_pixel(video_null, 200, y, false);
        // }
        // for (int x = 0; x < 479; x++) {
        //         // y=10的全宽度横线
        //         draw_pixel(video_null, x, 10, false);
        // }
        // for (int x = 0; x < 479; x++) {
        //         // y=179的全宽度横线
        //         draw_pixel(video_null, x, 179, false);
        // }
        draw_mac_icon(video_null, 0, 0);
        draw_string_8x8(video_null, 20, 80, "0123456789.9876543210", 0);
        unsigned int porch_padding = (VIDEO_HRES - VIDEO_FB_HRES)/2;
        // FIXME: HBP/HFP are prob off by one or so, check
        uint32_t timing = ((VIDEO_HSW - 1) << 23) |
                ((VIDEO_HBP + porch_padding - 3) << 15) |
                ((VIDEO_HFP + porch_padding - 4) << 7);
        video_dma_cfg[0] = timing | 0x80000000;
        video_dma_cfg[1] = VIDEO_FB_HRES - 1;
        video_dma_cfg[2] = timing;
        video_dma_cfg[3] = VIDEO_FB_HRES - 1;
}

static void     video_init_dma()
{
        /* pio_video expects each display line to be composed of two words of config
         * describing the line geometry and whether VS is asserted, followed by
         * visible data.
         *
         * To avoid having to embed config metadata in the display framebuffer,
         * we use two DMA transfers to PIO for each line.  The first transfers
         * the config from a config buffer, and then triggers the second to
         * transfer the video data from the framebuffer.  (This lets us use a
         * flat, regular FB.)
         *
         * The PIO side emits 1BPP MSB-first.  The other advantage of
         * using a second DMA transfer is then we can also can
         * byteswap the DMA of the video portion to match the Mac
         * framebuffer layout.
         *
         *  "Another caveat is that multiple channels should not be connected
         *   to the same DREQ.":
         * The final complexity is that only one DMA channel can do the
         * transfers to PIO, because of how the credit-based flow control works.
         * So, _only_ channel 0 transfers from $SOME_BUFFER into the PIO FIFO,
         * and channel 1+2 are used to reprogram/trigger channel 0 from a DMA
         * descriptor list.
         *
         * Two extra channels are used to manage interrupts; ch1 programs ch0,
         * completes, and does nothing.  (It programs a descriptor that causes
         * ch0 to transfer config, then trigger ch2 when complete.)  ch2 then
         * programs ch0 with a descriptor to transfer data, then trigger ch1
         * when ch0 completes; when ch2 finishes doing that, it produces an IRQ.
         * Got that?
         *
         * The IRQ handler sets up ch1 and ch2 to point to 2 fresh cfg+data
         * descriptors; the deadline is by the end of ch0's data transfer
         * (i.e. a whole line).  When ch0 finishes the data transfer it again
         * triggers ch1, and the new config entry is programmed.
         */
        video_dmach_tx = dma_claim_unused_channel(true);
        video_dmach_descr_cfg = dma_claim_unused_channel(true);
        video_dmach_descr_data = dma_claim_unused_channel(true);

        /* Transmit DMA: config+video data */
        /* First, make dmacfg for data to transfer from config buffers + data buffers: */
        dma_channel_config dc_tx_c = dma_channel_get_default_config(video_dmach_tx);
        channel_config_set_dreq(&dc_tx_c, DREQ_PIO0_TX0);
        channel_config_set_transfer_data_size(&dc_tx_c, DMA_SIZE_32);
        channel_config_set_read_increment(&dc_tx_c, true);
        channel_config_set_write_increment(&dc_tx_c, false);
        channel_config_set_bswap(&dc_tx_c, false);
        /* Completion of the config TX triggers the video_dmach_descr_data channel */
        channel_config_set_chain_to(&dc_tx_c, video_dmach_descr_data);
        video_dmadescr_cfg.raddr = NULL;                /* Reprogrammed each line */
        video_dmadescr_cfg.waddr = (void *)&pio0_hw->txf[0];
        video_dmadescr_cfg.count = 2;                   /* 2 words of video config */
        video_dmadescr_cfg.ctrl = dc_tx_c.ctrl;

        dma_channel_config dc_tx_d = dma_channel_get_default_config(video_dmach_tx);
        channel_config_set_dreq(&dc_tx_d, DREQ_PIO0_TX0);
        channel_config_set_transfer_data_size(&dc_tx_d, DMA_SIZE_32);
        channel_config_set_read_increment(&dc_tx_d, true);
        channel_config_set_write_increment(&dc_tx_d, false);
        channel_config_set_bswap(&dc_tx_d, true);      /* This channel bswaps */
        /* Completion of the data TX triggers the video_dmach_descr_cfg channel */
        channel_config_set_chain_to(&dc_tx_d, video_dmach_descr_cfg);
        video_dmadescr_data.raddr = NULL;               /* Reprogrammed each line */
        video_dmadescr_data.waddr = (void *)&pio0_hw->txf[0];
        video_dmadescr_data.count = VIDEO_VISIBLE_WPL;
        video_dmadescr_data.ctrl = dc_tx_d.ctrl;

        /* Now, the descr_cfg and descr_data channels transfer _those_
         * descriptors to program the video_dmach_tx channel:
         */
        dma_channel_config dcfg = dma_channel_get_default_config(video_dmach_descr_cfg);
        channel_config_set_transfer_data_size(&dcfg, DMA_SIZE_32);
        channel_config_set_read_increment(&dcfg, true);
        channel_config_set_write_increment(&dcfg, true);
        /* This channel loops on 16-byte/4-wprd boundary (i.e. writes all config): */
        channel_config_set_ring(&dcfg, true, 4);
        /* No completion IRQ or chain: the video_dmach_tx DMA completes and triggers
         * the next 'data' descriptor transfer.
         */
        dma_channel_configure(video_dmach_descr_cfg, &dcfg,
                              &dma_hw->ch[video_dmach_tx].read_addr,
                              &video_dmadescr_cfg,
                              4 /* 4 words of config */,
                              false /* Not yet */);

        dma_channel_config ddata = dma_channel_get_default_config(video_dmach_descr_data);
        channel_config_set_transfer_data_size(&ddata, DMA_SIZE_32);
        channel_config_set_read_increment(&ddata, true);
        channel_config_set_write_increment(&ddata, true);
        channel_config_set_ring(&ddata, true, 4);
        /* This transfer has a completion IRQ.  Receipt of that means that both
         * config and data descriptors have been transferred, and should be
         * reprogrammed for the next line.
         */
        dma_channel_set_irq0_enabled(video_dmach_descr_data, true);
        dma_channel_configure(video_dmach_descr_data, &ddata,
                              &dma_hw->ch[video_dmach_tx].read_addr,
                              &video_dmadescr_data,
                              4 /* 4 words of config */,
                              false /* Not yet */);

        /* Finally, set up video_dmadescr_cfg.raddr and video_dmadescr_data.raddr to point
         * to next line's video cfg/data buffers.  Then, video_dmach_descr_cfg can be triggered
         * to start video.
         */
}

////////////////////////////////////////////////////////////////////////////////

/* Initialise PIO, DMA, start sending pixels.  Passed a pointer to a 512x342x1
 * Mac-order framebuffer.
 *
 * FIXME: Add an API to change the FB base after init live, e.g. for bank
 * switching.
 * 
 * NJG : Added call to lcd_init()
 */
void    video_init(uint32_t *framebuffer)
{
        printf("Video init\n");
        
        // 打印内存使用情况
        print_memory_usage();

        pio_video_program_init(pio0, 0,
                               pio_add_program(pio0, &pio_video_program),
                               GPIO_VID_DATA, /* Followed by HS, VS, CLK */
                               VIDEO_PCLK_MULT);

        /* Invert output pins:  HS/VS are active-low, also invert video! */
        gpio_set_outover(GPIO_VID_HS, GPIO_OVERRIDE_INVERT);
        gpio_set_outover(GPIO_VID_VS, GPIO_OVERRIDE_INVERT);
        gpio_set_outover(GPIO_VID_DATA, GPIO_OVERRIDE_INVERT);
        /* Highest drive strength (VGA is current-based, innit) */
        hw_write_masked(&padsbank0_hw->io[GPIO_VID_DATA],
                        PADS_BANK0_GPIO0_DRIVE_VALUE_12MA << PADS_BANK0_GPIO0_DRIVE_LSB,
                        PADS_BANK0_GPIO0_DRIVE_BITS);

        /* IRQ handlers for DMA_IRQ_0: */
        irq_set_exclusive_handler(DMA_IRQ_0, video_dma_irq);
        irq_set_enabled(DMA_IRQ_0, true);

        video_init_dma();

        /* Init config word buffers */
        video_current_y = 0;
        video_framebuffer = framebuffer;
        video_prep_buffer();

        /* Set up pointers to first line, and start DMA */
        video_dma_prep_new();
        dma_channel_start(video_dmach_descr_cfg);
        
        // // 在帧缓冲区上绘制一些测试文字
        // draw_string_8x8(video_framebuffer, 10, 10, "Pico Mac Nano", 1);
        // draw_string_6x8(video_framebuffer, 10, 30, "Video System Ready", 1);
        // draw_string_6x8(video_framebuffer, 10, 50, "Font System: OK", 1);
}

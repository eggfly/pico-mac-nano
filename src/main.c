/* pico-umac
 *
 * Main loop to initialise umac, and run main event loop (piping
 * keyboard/mouse events in).
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
#include <unistd.h>
#include <string.h>
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/sync.h"
#include "hardware/pwm.h"
#include "pico/multicore.h"
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hw.h"
#include "video.h"
#include "kbd.h"

#include "bsp/rp2040/board.h"
#include "tusb.h"

#include "umac.h"

#if USE_SD
#include "f_util.h"
#include "ff.h"
#include "rtc.h"
#include "hw_config.h"
#endif

#if PICO_ZERO
#include "ws2812.h"
#include "ws2812.pio.h"
#endif

#if TFT_2P
#include "tft_2p.h"
#endif

#if BEEP
#include "audio.h"
#endif

extern hid_info_t g_hid_info;

////////////////////////////////////////////////////////////////////////////////
// Imports and data

extern void     hid_app_task(void);
extern int cursor_x;
extern int cursor_y;
extern int cursor_button;

// Mac binary data:  disc and ROM images
static const uint8_t umac_disc[] = {
#include "umac-disc.h"
};
static const uint8_t umac_rom[] = {
#include "umac-rom.h"
};

static uint8_t umac_ram[RAM_SIZE];

////////////////////////////////////////////////////////////////////////////////

// PWM初始化函数
static void init_pwm_outputs()
{
        // IO6 PWM配置
        gpio_set_function(6, GPIO_FUNC_PWM);
        uint slice_num_6 = pwm_gpio_to_slice_num(6);
        uint chan_6 = pwm_gpio_to_channel(6);
        
        // 设置PWM频率为1kHz
        pwm_set_clkdiv(slice_num_6, 125.0f); // 125MHz / 125 = 1MHz
        pwm_set_wrap(slice_num_6, 9999); // 1MHz / 10000 = 100Hz，但可以设置更低占空比
        pwm_set_chan_level(slice_num_6, chan_6, 1000); // 0.01%占空比 (1/10000)
        pwm_set_enabled(slice_num_6, true);
        
        printf("IO6 PWM initialized: 100Hz, 0.01%% duty cycle\n");
        
        // IO7 PWM配置检查
        gpio_set_function(7, GPIO_FUNC_PWM);
        uint slice_num_7 = pwm_gpio_to_slice_num(7);
        uint chan_7 = pwm_gpio_to_channel(7);
        
        // 检查IO7是否可以配置为PWM
        if (slice_num_7 != slice_num_6) {
                // IO7使用不同的PWM slice，可以独立配置
                pwm_set_clkdiv(slice_num_7, 125.0f);
                pwm_set_wrap(slice_num_7, 999);
                pwm_set_chan_level(slice_num_7, chan_7, 250); // 25%占空比作为测试
                pwm_set_enabled(slice_num_7, true);
                printf("IO7 PWM initialized: 1kHz, 25%% duty cycle\n");
        } else {
                // IO7与IO6共享同一个PWM slice，只能使用相同的频率
                pwm_set_chan_level(slice_num_7, chan_7, 750); // 75%占空比
                pwm_set_enabled(slice_num_7, true);
                printf("IO7 PWM initialized: same frequency as IO6, 75%% duty cycle\n");
        }
}

static void     io_init()
{
#if PICO_ZERO
                PIO pio = pio1; /* Use PIO 1 as Vidio is using PIO 0 */
                uint offset = pio_add_program(pio, &ws2812_program);
                uint sm = pio_claim_unused_sm(pio, true); //Claim a free state machine on a PIO instance
                ws2812_program_init(pio, sm, offset, GPIO_LED_PIN, 800000, true); /* 800000 is frequency */
#else
                gpio_init(GPIO_LED_PIN);
                gpio_set_dir(GPIO_LED_PIN, GPIO_OUT);
#endif
}

static void     poll_led_etc()
{
        static uint8_t led_on = 0;
        static absolute_time_t last = 0;
        absolute_time_t now = get_absolute_time();
        bool hasKeyboard = false;
        bool hasMouse = false;
        for (int i = 0; i < CFG_TUH_HID; i++) {
                if (g_hid_info.info[i].hid_type == 1) {
                        if (!hasKeyboard) {
                            hasKeyboard = true;
                        }       
                } else if (g_hid_info.info[i].hid_type == 2) {
                        if (!hasMouse) {
                            hasMouse = true;
                        }
                }
        }
        // Mouse exist led color green, keyboard exist led color blue
        uint8_t red = 0;
        uint8_t green = 0;
        uint8_t blue = 0;
        // red -> green
        // blue -> blue
        // green -> red
        if (hasMouse && !hasKeyboard) {
                green = 1;
        }else if (hasKeyboard && !hasMouse) {
                red = 1;
        }else if (hasKeyboard && hasMouse) {
                red = 1;
                green = 1;
                blue = 1;
        } else {
                // all 0
        }
        if (absolute_time_diff_us(last, now) > 500*1000) {
                last = now;
                printf("hasKeyboard = %d, hasMouse = %d, red = %d, green = %d, blue = %d\r\n", hasKeyboard, hasMouse, red, green, blue);

                led_on ^= 1;
#if PICO_ZERO
                put_pixel_rgb(red, green, blue); 
#else
                gpio_put(GPIO_LED_PIN, led_on);
#endif
        }
}

static int umac_cursor_x = 0;
static int umac_cursor_y = 0;
static int umac_cursor_button = 0;

static void     poll_umac()
{
        static absolute_time_t last_1hz = 0;
        static absolute_time_t last_vsync = 0;
        absolute_time_t now = get_absolute_time();

        umac_loop();

        int64_t p_1hz = absolute_time_diff_us(last_1hz, now);
        int64_t p_vsync = absolute_time_diff_us(last_vsync, now);
        if (p_vsync >= 16667) {
                /* FIXME: Trigger this off actual vsync */
                umac_vsync_event();
                last_vsync = now;
        }
        if (p_1hz >= 1000000) {
                umac_1hz_event();
                last_1hz = now;
        }

        int update = 0;
        int dx = 0;
        int dy = 0;
        int b = umac_cursor_button;
        if (cursor_x != umac_cursor_x) {
                dx = cursor_x - umac_cursor_x;
                umac_cursor_x = cursor_x;
                update = 1;
        }
        if (cursor_y != umac_cursor_y) {
                dy = cursor_y - umac_cursor_y;
                umac_cursor_y = cursor_y;
                update = 1;
        }
        if (cursor_button != umac_cursor_button) {
                b = cursor_button;
                umac_cursor_button = cursor_button;
                update = 1;
        }
        if (update) {
                umac_mouse(dx, -dy, b);
        }
        // handle_my_mouse(cursor_x, cursor_y, cursor_button);

        if (!kbd_queue_empty()) {
                uint16_t k = kbd_queue_pop();
                umac_kbd_event(k & 0xff, !!(k & 0x8000));
        }
}

#if USE_SD
static int      disc_do_read(void *ctx, uint8_t *data, unsigned int offset, unsigned int len)
{
        FIL *fp = (FIL *)ctx;
        f_lseek(fp, offset);
        unsigned int did_read = 0;
        FRESULT fr = f_read(fp, data, len, &did_read);
        if (fr != FR_OK || len != did_read) {
                printf("disc: f_read returned %d, read %u (of %u)\n", fr, did_read, len);
                return -1;
        }
        return 0;
}

static int      disc_do_write(void *ctx, uint8_t *data, unsigned int offset, unsigned int len)
{
        FIL *fp = (FIL *)ctx;
        f_lseek(fp, offset);
        unsigned int did_write = 0;
        FRESULT fr = f_write(fp, data, len, &did_write);
        if (fr != FR_OK || len != did_write) {
                printf("disc: f_write returned %d, read %u (of %u)\n", fr, did_write, len);
                return -1;
        }
        return 0;
}

static FIL discfp;
#endif

static void     disc_setup(disc_descr_t discs[DISC_NUM_DRIVES])
{
#if USE_SD
        char *disc0_name;
        const char *disc0_ro_name = "umac0ro.img";
        const char *disc0_pattern = "umac0*.img";

        /* Mount SD filesystem */
        printf("Starting SPI/FatFS:\n");
        set_spi_dma_irq_channel(true, false);
        sd_card_t *pSD = sd_get_by_num(0);
        FRESULT fr = f_mount(&pSD->fatfs, pSD->pcName, 1);
        printf("  mount: %d\n", fr);
        if (fr != FR_OK) {
                printf("  error mounting disc: %s (%d)\n", FRESULT_str(fr), fr);
                goto no_sd;
        }

        /* Look for a disc image */
        DIR di = {0};
        FILINFO fi = {0};
        fr = f_findfirst(&di, &fi, "/", disc0_pattern);
        if (fr != FR_OK) {
                printf("  Can't find images %s: %s (%d)\n", disc0_pattern, FRESULT_str(fr), fr);
                goto no_sd;
        }
        disc0_name = fi.fname;
        f_closedir(&di);

        int read_only = !strcmp(disc0_name, disc0_ro_name);
        printf("  Opening %s (R%c)\n", disc0_name, read_only ? 'O' : 'W');

        /* Open image, set up disc info: */
        fr = f_open(&discfp, disc0_name, FA_OPEN_EXISTING | FA_READ | FA_WRITE);
        if (fr != FR_OK && fr != FR_EXIST) {
                printf("  *** Can't open %s: %s (%d)!\n", disc0_name, FRESULT_str(fr), fr);
                goto no_sd;
        } else {
                printf("  Opened, size 0x%x\n", f_size(&discfp));
                if (read_only)
                        printf("  (disc is read-only)\n");
                discs[0].base = 0; // Means use R/W ops
                discs[0].read_only = read_only;
                discs[0].size = f_size(&discfp);
                discs[0].op_ctx = &discfp;
                discs[0].op_read = disc_do_read;
                discs[0].op_write = disc_do_write;
        }

        /* FIXME: Other files can be stored on SD too, such as logging
         * and NVRAM storage.
         *
         * We could also implement a menu here to select an image,
         * writing text to the framebuffer and checking kbd_queue_*()
         * for user input.
         */
        return;

no_sd:
#endif
        /* If we don't find (or look for) an SD-based image, attempt
         * to use in-flash disc image:
         */
        discs[0].base = (void *)umac_disc;
        discs[0].read_only = 1;
        discs[0].size = sizeof(umac_disc);
}

static void     core1_main()
{
        disc_descr_t discs[DISC_NUM_DRIVES] = {0};

        printf("Core 1 started\n");
        disc_setup(discs);

        umac_init(umac_ram, (void *)umac_rom, discs);

       /* Video runs on core 1, i.e. IRQs/DMA are unaffected by
         * core 0's USB activity.
         */
        video_init((uint32_t *)(umac_ram + umac_get_fb_offset()));

        printf("Enjoyable Mac times now begin:\n\n");

        while (true) {
                poll_umac();
        }
}

int     main()
{
        set_sys_clock_khz(250*1000, true);

	stdio_init_all();
        io_init(); /* Just sets up the LED */
        
        // 初始化PWM功能
        // init_pwm_outputs();

#if BEEP
        printf("Output audio beep\n");
        beep();
#endif

#if TFT_2P /* Need to initialise TFT before UMAC starts because SPI is shared between TFT and SD card */
        printf("TFT init\n");
        tft_init();
#endif

        multicore_launch_core1(core1_main);

	printf("Starting, init usb\n");
        tusb_init();

        /* This happens on core 0: */
	while (true) {
                tuh_task();
                hid_app_task();
                poll_led_etc();
                
                // // Add periodic hub status monitoring
                // static uint32_t last_hub_check = 0;
                // uint32_t now = to_ms_since_boot(get_absolute_time());
                // if (now - last_hub_check > 5000) { // Check every 5 seconds
                //         printf("USB Hub Status Check - Device count: %d\r\n", tuh_device_count());
                //         last_hub_check = now;
                // }
	}

	return 0;
}


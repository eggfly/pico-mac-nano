# pico-mac-nano

<img alt="pico-mac-nano" src="https://github.com/user-attachments/assets/d9a80395-f821-47a4-9e62-aa7472d8036d" height="160px" />
<img alt="pico-mac-nano" src="https://github.com/user-attachments/assets/5b1be959-36a9-44cd-a6f1-652e655c3235" height="160px" /><img alt="big brother" src="https://github.com/user-attachments/assets/53f04e85-f4c4-42e8-bdb0-ae4d853ce238" height="160px" /><img alt="inside" src="https://github.com/user-attachments/assets/92f52392-7c24-46c8-81e2-6b47628ced43" height="160px" />

v0.3 20 May 2025

Fork of https://github.com/evansm7/pico-mac v0.21

The purpose of this fork is the creation of a miniature functioning Macintosh running pico-mac on a [Waveshare Pico Zero](https://www.waveshare.com/wiki/RP2040-Zero) (or pin/function compatible RP2040 MCU) by adding support for output to a small TFT LCD panel. 

This involved the following steps:
   * Selecting and testing the smallest suitably high resolution display I could locate at
     a reasonable price.
   * Working backwards from the LCD image size to calculate scale of the Macintosh model.
   * Designing the internal layout of the Macintosh to accomodate all components.
   * 3D modelling the Macintosh case in two halves with internal supports for components
   * Designing a PCB to connect all components.
   * Updating the pico-mac source code to ...
   	 - configure the LCD via SPI interface shared with SD card module
   	 - output suitable 480x342 pixel frame buffer data for the LCD
   	 - use the Pico Zero's built in Neo-pixel RGB LED
   	 - output out of phase 600Hz square waves on two GPIOs for 1s at startup to simulate
   	  the Macintosh startup beep

The finished Macintosh is just 62mm high but houses a 2.0" TFT LCD panel, rp2040 Pico Zero MCU, micro-SD card reader and the custom PCB.

For more details on the journey and design decisions, see the [pico-mac-nano project page](https://blog.1bitrainbow.com/pico-mac-nano/).

The firmware directory contains an example, pre-built .uf2 rp2040 firmware file for the Pico Zero. This version is built with the standard 128K ram.

The PCB directory contains all gerber, drill hole, position and BOM files for the most recent revision of the PCB.

Data sheets for the TFT module and the controller are in the data_sheets directory.

The 3D_model directory contains the .stl files for the four components of the Macintosh 128K case; front housing, rear housing, switch blank (blocks the switch apperture intended for the [battery power module](https://www.1bitrainbow.com/parts-store.php?cPath=972_973_P4470)) and reset/boot select button actuator.

All parts used in pico-mac-nano can be ordered on [1bitrainbow.com](https://www.1bitrainbow.com/parts-store.php?cPath=972_973). This includes the 3D printed case, the fully assembled custom PCB and the Pico Zero (pre-flashed with the latest pico-mac-nano firmware). You can even order a fully assembled pico-mac-nano or the collectors edition so you can be as hands-on or hands-off as you want.

## Hardware Notes

The pico code, PCB and Macintosh case are all designed for the 2.0" 480x640px DX7 D200N2409V0 LCD panel. This uses the ST7701S controller, is configured via SPI and receives image data in RGB565 (16-bit colour). This LCD is natively portrait and the ST7701S controller does not offer image rotation in hardware so I am displaying a 480x342 image across the upper half of the LCD panel. See the [pico-mac-nano project page](https://blog.1bitrainbow.com/pico-mac-nano/) for more details of how I got to this decision.

The case and internal layout is designed to allow the Pico Zero and SD module to be connected via 2.54mm header sockets rather than being soldered. Due to the small size of the PCB, I chose not to include sockets for the unused Pico Zero pins. Not having these unnecessary through-hole pins allowed routing of traces more efficiently. Consequently, the Pico Zero connects to the PCB via a 9-pin (J3), a 2-pin (J4), and a 4-pin (J6) socket.

The PCB includes the capacitors (C1 & C2) recommended by evansm7 across the micro-SD card module power rails.

The Pico 5V (VCC) is used to power the TFT backlight via a resistor (R1) which maintains the correct forward voltage specified by the TFT data sheet.

The 40-pin, vertical FFC socket (J1) is used to connect the TFT LCD panel. Note that for some reason, the LCD flex cable has its 'pin' numbers labelled in the reverse order to the FCC socket. This is why the PCB indicates where the LCD flex pin 1 should be.

Labelled test pads are provided on the PCB for all data signals.

Provision for battery power is provided on the PCB in the form of +5v and Ground header pins. This 5v input passes through an on-board Schottky diode (D1) to protect against pico-mac-nano being connected to a USB host and receiving 5V via USB while using battery power.
The optional battery power module which uses a 3v lithium C2 battery, 5v boost regulator and micro switch, is available now on [1bitrainbow.com](https://www.1bitrainbow.com/parts-store.php?cPath=972_973_P4470).

All components should be mounted on the underside of the PCB with the exception of the 2-pin 2.54mm male header (right-angle if using the [1-bit rainbow battery power module](https://www.1bitrainbow.com/parts-store.php?cPath=972_973_P4470)) for 5v battery power, and the 2-pin 2.54mm female header for Audio (Beep). These should be mounted on the top to allow connection of a power source and/or speaker.

<img alt="pcb bottom" src="https://github.com/user-attachments/assets/2985da71-811d-4a72-b823-25e356b60896" height="320px" />
<img alt="pcb top side" src="https://github.com/user-attachments/assets/69a76ac8-57c6-4426-9abd-42eba6f9679b" height="320px" />

For reference, here are the pin assignments I have used for the Pico Zero.

|**Pin**|Ref|**Function**|
| --- | --- | --- |
|1		|5V			|VCC (TFT Backlight)|
|2		|GND		|Ground|
|3		|3V3		|VDD|
|4		|GPIO 29	|H-Sync|
|5		|GPIO 28	|Pix Clk|
|6		|GPIO 27	|V-Sync|
|7		|GPIO 26	|Vid Data|
|8		|GPIO 15	|TFT Reset|
|9		|GPIO 14	|TFT SPI CS|
|10		|GPIO 13	|Audio +|
|11		|GPIO 12	|Audio -|
|12		|GPIO 11	|N/C|
|13		|GPIO 10	|N/C|
|14		|GPIO 9		|N/C|
|15		|GPIO 8		|N/C|
|16		|GPIO 7		|N/C|
|17		|GPIO 6		|N/C|
|18		|GPIO 5		|SD SPI CS|
|19		|GPIO 4		|SPI MISO (shared)|
|20		|GPIO 3		|SPI MOSI (shared)|
|21		|GPIO 2		|SPI Clk (shared)|
|22		|GPIO 1		|N/C|
|23		|GPIO 0		|N/C|

The Audio + and Audio - pins output a 600Hz square wave for 1 second at the start of the initialisation code to give an approximate rendition of the original Macintosh startup beep. Audio - is just the inverse of (180 deg out of sync with) Audio +. By connecting a speaker between this pair of outputs, we get a 6.6v differential signal which gave an acceptable volume without the need for any amplification. When the speaker was just connected between one output and ground, the volume was too low to be of use (In case you're worrying, both pins are set low after the beep so the Pico is not left with a high GPIO shorting to ground through the speaker).
Audio + and Audio - are presented on their own header pins on the PCB to allow a speaker to be easily connected.
Using a suitable inductor (the value will depend on the impedance of the speaker you are using) in series with the speaker, or a capacitor across the speaker, should filter out some of the higher frequencies so the beep is less buzzy.

As with the pico-mac project, pico-mac-nano is a personal hobby project and proof of concept rather than a commercial product. My aim was to see how small I could make it and this involved some compromises.

Disclaimer: This project is provided with no warranty. All due care has been taken in design/docs, but if you choose to build it then I disclaim any responsibility for your hardware or personal safety.

Below is a modified version of [evansm7](https://github.com/evansm7) original readme from pico-mac v0.21 minus the Hardware Construction section since this variant uses a custom PCB so there is none of the original construction to be done.

# Pico Micro Mac (pico-umac) v0.2
This project embeds the [umac Mac 128K
emulator](https://github.com/evansm7/umac) project into a Raspberry Pi
Pico microcontroller.  At long last, the worst Macintosh in a cheap,
portable form factor!

It has features, many features, the best features:

   * Outputs 480x342px at 60Hz, monochrome to 2.0" LCD 
   * USB HID keyboard and mouse
   * Read-only disc image in flash (your creations are ephemeral, like life itself)
   * Or, if you have a hard time letting go, support for rewritable
     disc storage on an SPI-attached SD card
   * Mac 128K by default, or you can make use of more of the Pico's
     memory and run as a _Mac 208K_

Great features.  It even doesn't hang at random!  (Anymore.)

The _Mac 208K_ was, of course, never a real machine.  But, _umac_
supports odd-sized memories, and more memory runs more things.  A
surprising amount of software runs on the 128K config, but if you need
to run _MacPaint_ specifically then you'll need to build both SD
storage in addition to the _Mac 208K_ config.

To build this project yourself, you'll need at least a WaveShare Pico Zero or compatible board, a 3D printer, the 2.0" LCD panel, SPI micro-SD card module, interconnect PCB, USB mouse (and maybe a USB keyboard/hub), plus a suitable OTG USB splitter.

# Build

## Prerequisites/essentials

   * git submodules
      - Clone the repo with `--recursive`, or `git submodule update --init --recursive`
   * Install/set up the [Pico/RP2040 SDK](https://github.com/raspberrypi/pico-sdk)

## Build umac

Install and build `umac` first using the display width and height options. 
It'll give you a preview of the fun to come, plus is required to generate a patched ROM image.

```
cd external/umac
make DISP_WIDTH=480 DISP_HEIGHT=342
```

If you want to use a non-default memory size (i.e. >128K) you will
need to build `umac` with a matching `MEMSIZE` build parameter, for
example:

```
cd external/umac
make DISP_WIDTH=480 DISP_HEIGHT=342 MEMSIZE=208
```

This is because `umac` is used to patch the ROM, and when using
unsupported sizes between 128K and 512K the RAM size can't be probed
automatically, so the size needs to be embedded.

## Build pico-umac

Do the initial Pico SDK `cmake` setup into an out-of-tree build dir,
providing config options if required.

From the top-level `pico-umac` directory:

```
mkdir build
(cd build ; PICO_SDK_PATH=/path/to/sdk cmake .. <options>)
```

If you want more than the default 128K of memory, you need to build with the following option.
 
   * `-DMEMSIZE=<size in KB>`: The maximum practical size is about
     208KB, but values between 128 and 208 should work on a RP2040.
     Note that although apps and Mac OS seem to gracefully detect free
     memory, these products never existed and some apps might behave
     strangely.
      - With the `Mac Plus` ROM, a _Mac 128K_ doesn't quite have
        enough memory to run _MacPaint_.  So, 192 or 208 (and a
        writeable boot volume on SD) will allow _MacPaint_ to run.
      - **NOTE**: When this option is used, the ROM image must be
          built with an `umac` build with a corresponding `MEMSIZE`

**NOTE** All other options available in pico-mac are still recognised but are not listed here because the LCD and custom PCB used in pico-mac-nano require them to be set to specific values. Overriding the default for any option other than DMEMSIZE will cause problems.

Tip: `cmake` caches these variables, so if you see weird behaviour
having built previously and then changed an option, delete the `build`
directory and start again.

## ROM image

The flow is to use `umac` built on your workstation (e.g. Linux,
but WSL may work too) to prepare a patched ROM image.

`umac` is passed the 4D1F8172 MacPlusv3 ROM, and `-W` to write the
post-patching binary out:

```
./external/umac/main -r '4D1F8172 - MacPlus v3.ROM' -W rom.bin
```

Note: Again, remember that if you are using the `-DMEMSIZE` option to
increase the `pico-umac` memory, you will need to create this ROM
image with a `umac` built with the corresponding `MEMSIZE` option, as
above.

## Disc image

Micro-SD support is enabled by default. An internal read-only disc image is
still stored in flash by default which is used as a fallback if
no SD card is present, no valid boot image exists on the SD card, or the SD boot fails.

Grab a Macintosh system disc from somewhere.  A 400K or 800K floppy
image works just fine, up to System 3.2 (the last version to support
Mac128Ks).  I've used images from
<https://winworldpc.com/product/mac-os-0-6/system-3x> but also check
the various forums and MacintoshRepository.  See the `umac` README for
info on formats (it needs to be raw data without header).

The image size can be whatever you have space for in flash (typically
about 1.3MB is free there), or on the SD card.  (I don't know what the
HFS limits are.  But if you make a 50MB disc you're unlikely to fill
it with software that actually works on the _Mac 128K_ :) )

Use a FAT-formatted micro-SD card and copy your disc image
into _one_ of the following files in the root of the card:

   * `umac0.img`:  A normal read/write disc image
   * `umac0ro.img`:  A read-only disc image

## Putting it together, and building

Given the `rom.bin` prepared above and a `disc.bin` destinated for
flash, you can now generate includes from them and perform the build:

```
mkdir incbin
xxd -i < rom.bin > incbin/umac-rom.h
xxd -i < disc.bin > incbin/umac-disc.h
make -C build
```

You'll get a `build/firmware.uf2` out the other end.  Flash this to
your Pico Zero: e.g. plug it into your computer with the Boot Select button held down. This should cause the Pico Zero to appear as a removable volume. Drag and drop the formware file onto the volume. ([evansm7](https://github.com/evansm7) advocated using an SWD probe but due to design constraints, UART serial pins are not routed on the custom PCB)

The RGB LED should light up blue while the audio beep is being played, rapidly flash red while the LCD is being configured and then steadily flash green at about 2Hz.


# Software

Both CPU cores are used, and are overclocked (blush) to 250MHz so that
Missile Command is enjoyable to play.

The `umac` emulator and video output runs on core 1, and core 0 deals
with USB HID input.  Video DMA is initialised pointing to the
framebuffer in the Mac's RAM.

Other than that, it's just a main loop in `main.c` shuffling things
into `umac`.

Quite a lot of optimisation has been done in `umac` and `Musashi` to
get performance up on Cortex-M0+ and the RP2040, like careful location
of certain routines in RAM, ensuring inlining/constants can be
foldeed, etc.  It's 5x faster than it was at the beginning.

The top-level project might be a useful framework for other emulators,
or other projects that need USB HID input and a framebuffer (e.g. a
VT220 emulator!).

The USB HID code is largely stolen from the TinyUSB example, but shows
how in practice you might capture keypresses/deal with mouse events.

## Video

The video system is pretty good and IMHO worth stealing for other
projects: It uses one PIO state machine and 3 DMA channels to provide
a rock-solid bitmapped 1BPP 480x342 video output. ~~The Mac 512x342
framebuffer is centred inside this by using horizontal blanking
regions (programmed into the line scan-out) and vertical blanking
areas from a dummy "always black" mini-framebuffer.~~

~~It supports (at build time) flexible resolutions/timings.  The one
caveat (or advantage?) is that it uses an HSYNC IRQ routine to
recalculate the next DMA buffer pointer; doing this at scan-time costs
about 1% of the CPU time (on core 1).  However, it could be used to
generate video on-the-fly from characters/tiles without a true
framebuffer.~~

I'm considering improvements to the video system:

   * Supporting multiple BPP/colour output
   * Implement the rest of `DE`/display valid strobe support, making
     driving LCDs possible.
   * Using a video DMA address list and another DMA channel to reduce
     the IRQ frequency (CPU overhead) to per-frame, at the cost of a
     couple of KB of RAM.


# Licence

`hid.c` and `tusb_config.h` are based on code from the TinyUSB
project, which is Copyright (c) 2019, 2021 Ha Thach (tinyusb.org) and
released under the MIT licence.  `sd_hw_config.c` is based on code
from the no-OS-FatFS-SD-SPI-RPi-Pico project, which is Copyright (c)
2021 Carl John Kugler III.

The remainder of the code is released under the MIT licence:

 Copyright (c) 2024 Matt Evans:

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.


# 关于 208K 和 480 分辨率的编译步骤

* 首先必须 external/umac 带 `make MEMSIZE=208` 然后 ./main -W 从 4D1F8172 - MacPlus v3.ROM 生成 patched_rom
* 然后 pico-mac 项目 cmake的时候 带上-DMEMSIZE=208 (256是不够的，RAM段恰好差了40多KB)
* 生成的width 应该是512的（后续可以通过鼠标滚动处理？）
* 208K可以通过Finder的白色背景的about固件看到
* MacPaint 需要208K 且需要tf卡可读写 disc 模式才能运行，否则弹窗报错

* 另，尝试使用`make DISP_WIDTH=480 DISP_HEIGHT=342 MEMSIZE=208` 打patched_rom以后，再到cmake 出来画面是错位的，然后需要在video.c 中修改#define VIDEO_FB_HRES           480 才可以，不知道为何
* 从128K切换到208K以后，开机速度很慢，且流畅度确实有所降低
* 改成480以后，左边出现了32像素的黑色边，右边超出屏幕了，video.c中还没有找到合适的改法
* external/umac 会有.o 目标文件，在外面make的时候可能会用到缓存，所以在重新配置MEMSIZE等参数的时候，需要make clean一下umac

# 128K 内存可以运行程序列表:
* Alice 国际象棋
* Aliens 小蜜蜂
* Amazing 迷宫但是需要512宽度
* MacLanding
* Bricks 打砖块
* ChipWits 黑屏
* City Defense
* Crystal Radar
* Daleks 逃脱游戏
* Destroyer
* Frogger 崩溃了
* Fusillade
* GATO 不行
* GridWars OK 3D效果不错
* GroundZero OK
* Hearts OK
* Life OK
* Missle Command OK
* MLScores OK
* Mouse
* Reversi ok
* Sargon III OK 国际象棋
* Solitaire 1.0 OK
* Transylvania 企鹅游戏不会玩
* 汉诺塔 OK










# A Sinclair ZX81 and ZX80 Emulator for the Raspberry Pi Pico
# Contents
+ [Features](#features)
+ [Quick Start](#quick-start)
+ [Use](#use)
+ [Applications Tested](#applications-tested)
+ [Building](#building)
+ [Extra-Information](#extra-information)
+ [Developer Notes](#developer-notes)

# Features
+ Supports VGA, HDMI, DVI and LCD displays
+ Supports sound over HDMI, onboard DAC or PWM when available in hardware
+ Provides an immersive full screen experience, with a very fast boot time and no operating system
+ Simultaneous USB keyboard and joystick support (using a powered USB hub)
+ Can be fully controlled from a ZX81 style 40 key keyboard
+ 1980s style 9-pin Atari joysticks can be connected to some board types
+ The small form factor makes the board easy to mount in period or reproduction cases. The low cost, relatively low performance and software generated display of the Pico is a 21st century analogue for the ZX80 and ZX81
+ Emulates pseudo and Hi-res graphics
+ Emulates ZX81 and ZX80 (with either 4K or 8K ROM) hardware
+ Emulates ZonX, Quicksilva and TV (sync) sound
+ Emulates user defined graphics, including CHR$128 and QS User Defined Graphics
+ Emulates the [Chroma 80](http://www.fruitcake.plus.com/Sinclair/ZX80/Chroma/ZX80_ChromaInterface.htm) and [Chroma 81](http://www.fruitcake.plus.com/Sinclair/ZX81/Chroma/ChromaInterface.htm) interfaces to allow a colour display. Also supports the enhanced TV sound provided by Chroma
+ Emulation runs at accurate speed of a 3.25MHz ZX81
+ Optionally emulates real-time ZX81/80 program load and save with realistic sound and graphics
+ Emulates European and US configuration (i.e. emulates 50Hz and 60Hz ZX81)
+ Supports larger ZX81 generated displays of over 320 by 240 pixels (40 character width and 30 character height)
+ Load `.p`, `.81`, `.o`, `.80` and `.p81` files from micro SD Card. Save `.p` and `.o` files
+ Can display at 640x480 or 720x576 (for an authentic display on a UK TV)
+ 720x576 can be configured to run at a frame rate to match the "real" ZX81 (~50.65 Hz).
+ An interlaced mode can be selected to display interlaced images with minimal flicker
+ Supports loading and saving of memory blocks using [ZXpand like syntax](https://github.com/charlierobson/ZXpand-Vitamins/wiki/ZXpand---Online-Manual#load)
+ Set-up of emulator (computer type, RAM, Hi-Res graphics, sound, joystick control etc) configurable on a per program basis, using config files
+ Optionally displays graphic of keyboard (taken from [sz81](https://github.com/SegHaxx/sz81)). Can type in code with keyboard visible
+ Can be extended for other board types. Code support included for a custom VGA RGB 332 board similar to that supported by [MCUME](https://github.com/Jean-MarcHarvengt/MCUME) and for a RGB 222 board with CSYNC similar to [PICOZX](https://hackaday.io/project/186039-pico-zx-spectrum-128k)
## Supported Hardware
+ [Pimoroni Pico VGA demo board](https://shop.pimoroni.com/products/pimoroni-pico-vga-demo-base)
+ [Pimoroni Pico DVI demo board (HDMI)](https://shop.pimoroni.com/products/pimoroni-pico-dv-demo-base)
+ [PicoMiteVGA board](https://geoffg.net/picomitevga.html)
+ [Olimex RP2040-PICO-PC (HDMI)](https://www.olimex.com/Products/MicroPython/RP2040-PICO-PC/open-source-hardware)
+ [Waveshare RP2040-PiZero (HDMI)](https://www.waveshare.com/wiki/RP2040-PiZero)
+ [Waveshare Pico-ResTouch-LCD-2.8](https://www.waveshare.com/wiki/Pico-ResTouch-LCD-2.8)
+ [Cytron Maker Pi Pico](https://www.cytron.io/p-maker-pi-pico) with 320 by 240 LCD and RGB222 VGA displays
+ [PICOZX](https://hackaday.io/project/186039-pico-zx-spectrum-128k) All in one board with VGA output, built in keyboard and 9-pin joystick port
+ [PICOZX with LCD](https://hackaday.io/project/186039-pico-zx-spectrum-128k) All in one board with VGA and LCD output, built in keyboard and 9-pin joystick port
+ [PICOZX for ZX-Spectrum case](https://www.pcbway.com/project/shareproject/PICOZX_motherboard_for_ZX_Spectrum_original_case_5bbde8be.html) All in one board with VGA output, designed to be put in a ZX-Spectrum case with keyboard and 9-pin joystick port

## Examples
### Installed in a reproduction case
The following images are taken with permission from a thread on [SinclairZXWorld](https://sinclairzxworld.com/viewtopic.php?f=3&t=5071&start=20) and show how user `computergui` has used picozx81 together with a case created by user `Spinnetti` to create a replica ZX80

<p align="middle">
<img src="images/open_case.jpg" width="48%" />
<img src="images/in_use.jpg" width="48%" />
</p>

### 3d printed ZX81 USB keyboard, USB joystick and LCD
The left image shows a system with a 3d printed ZX81 case being used as a [USB keyboard](https://github.com/ikjordan/ZX81_USB_KBD). A usb hub allows control via the keyboard and a joystick. The high resolution version of [Galaxians](http://zx81.eu5.org/files/soft/toddy/HR-Galax.zip) is displayed on screen

The right image shows the emulator running [MaxDemo](https://bodo4all.fortunecity.ws/zx/maxdemo.html), which generates a 320 by 240 display, exactly filling the 2.8 inch Waveshare LCD

<p align="middle">
<img src="images/lcd_system.jpg" width="46%" />
<img src="images/lcd.jpg" width="49.2%" />
</p>

### HDMI/DVI Output
To the left [ZX81 Hires Invaders](https://www.perfectlynormalsite.com/hiresinvaders.html) can be seen on a TV connected over HDMI. Sound can also be played over HDMI

To the right can be seen a status page, illustrating some of the configurable options for the emulator

<p align="middle">
<img src="images/invaders.jpg" width="51.0%" />
<img src="images/status.jpg" width="45.15%" />
</p>

### LCD Displays with Cytron Maker Board
[Cytron Maker Pi Pico](https://www.cytron.io/p-maker-pi-pico) with [Waveshare 2.0" LCD](https://www.waveshare.com/wiki/2inch_LCD_Module) (ST7789V controller) displaying the [25thanni](https://bodo4all.fortunecity.ws/zx/25thanni.html) full screen demo
<p align="middle">
<img src="images/lcd_2_0.jpg" width="95%" />
</p>

[Cytron Maker Pi Pico](https://www.cytron.io/p-maker-pi-pico) with [Generic 3.2" LCD](http://www.lcdwiki.com/3.2inch_SPI_Module_ILI9341_SKU:MSP3218) (ILI9341 controller) displaying the [25thanni](https://bodo4all.fortunecity.ws/zx/25thanni.html) full screen demo
<p align="middle">
<img src="images/lcd_3_2.jpg" width="95%" />
</p>

### Dedicated hardware
[PICOZX with VGA and LCD output](https://hackaday.io/project/186039-pico-zx-spectrum-128k) running 3D Monster Maze

Pictures supplied by [zzapmort](https://www.ebay.co.uk/usr/zzapmort)
<p align="middle">
<img src="images/3dmonster.jpg" width="49%" />
<img src="images/picozx_lcd.jpg" width="49%" />
</p>

[PICOZX in a Spectrum case](https://www.pcbway.com/project/shareproject/)

A ZX81 disguised as a Spectrum!
<p align="middle">
<img src="images/zx81_spectrum.jpg" width="60%" />
</p>

### Chroma 80 and Chroma 81 Emulation
[ColourAttrModeTest](http://www.fruitcake.plus.com/Sinclair/ZX81/Chroma/Files/Programs/ColourAttrModeTest.zip) and [HiRes ZX-Galaxians](http://zx81.eu5.org/files/soft/toddy/HR-Galax.zip)
<p align="middle">
<img src="images/chroma_attribute.jpg" width="49.0%" />
<img src="images/chroma_galaxians.jpg" width="49.0%" />
</p>

# Quick Start
The fastest way to get started is to:
1. Write data onto a Micro SD Card
2. Install the pre-built binary onto your Pico
3. Connect your board to a keyboard and start exploring

## Populate the SD Card
1. Click [here](https://github.com/ikjordan/picozx81/releases/latest/download/sdcard.zip) to download a zip file containing files and directories to copy to an empty micro SD card
2. unzip the file into the root directory of an empty micro SD Card
3. Insert the micro SD Card into your boards micro SD card slot

## Load the pre-build binary onto the Pico
Click on the uf2 name corresponding to your board in the table below to download the latest version
| Board | uf2 name |
| --- | --- |
| Pimoroni DVI | [`picozx81_dvi.uf2`](https://github.com/ikjordan/picozx81/releases/latest/download/picozx81_dvi.uf2)|
| Pimoroni VGA | [`picozx81_vga.uf2`](https://github.com/ikjordan/picozx81/releases/latest/download/picozx81_vga.uf2)|
| Olimex PICO DVI | [`picozx81_olimexpc.uf2`](https://github.com/ikjordan/picozx81/releases/latest/download/picozx81_olimexpc.uf2)|
| PicoMiteVGA | [`picozx81_picomitevga.uf2`](https://github.com/ikjordan/picozx81/releases/latest/download/picozx81_picomitevga.uf2)|
| Olimex PICO DVI with HDMI Sound| [`picozx81_olimexpc_hdmi_sound.uf2`](https://github.com/ikjordan/picozx81/releases/latest/download/picozx81_olimexpc_hdmi_sound.uf2)|
| Pimoroni DVI with HDMI Sound| [`picozx81_dvi_hdmi_sound.uf2`](https://github.com/ikjordan/picozx81/releases/latest/download/picozx81_dvi_hdmi_sound.uf2)|
| Waveshare PiZero with HDMI Sound| [`picozx81_wspizero_hdmi_sound.uf2`](https://github.com/ikjordan/picozx81/blob/main/uf2/picozx81_wspizero_hdmi_sound.uf2)|
| Waveshare 2.8 LCD | [`picozx81_lcdws28.uf2`](https://github.com/ikjordan/picozx81/releases/latest/download/picozx81_lcdws28.uf2)|
| Cytron Maker + 320x240 LCD | [`picozx81_lcdmaker.uf2`](https://github.com/ikjordan/picozx81/releases/latest/download/picozx81_lcdmaker.uf2)|
| Cytron Maker + VGA 222 CSYNC | [`picozx81_vgamaker222c.uf2`](https://github.com/ikjordan/picozx81/releases/latest/download/picozx81_vgamaker222c.uf2)|
| PICOZX | [`picozx81_picozx.uf2`](https://github.com/ikjordan/picozx81/blob/main/uf2/picozx81_picozx.uf2)|
| PICOZX with LCD | [`picozx81_picozx_lcd.uf2`](https://github.com/ikjordan/picozx81/blob/main/uf2/picozx81_picozx_lcd.uf2)|
| PICOZX for ZX-Spectrum case | [`picozx81_picozxreal.uf2`](https://github.com/ikjordan/picozx81/blob/main/uf2/picozx81_picozxreal.uf2)|

1. Connect your Board to your display using a VGA or HDMI cable, as appropriate for your board
2. Connect the Pico to your PC using a USB cable, whilst pressing the **BOOTSEL** button on the Pico. Use the micro USB connector on the Pico, *not* the micro USB cable on your board
3. The Pico should appear on the PC as a USB drive
4. Drag the uf2 file that you downloaded onto the USB drive
5. The file will be loaded onto the Pico, which will then reboot. You should see the familiar `K` prompt appear on your display
## Connect to a keyboard
1. Remove the USB cable from the Pico. Plug a USB keyboard into the Pico. You will need an [OTG adaptor cable](https://www.google.co.uk/search?q=otg+adaptor) to do this
2. Plug a micro USB power supply into the USB connector on your board. You can also power your board directly from your PC using a micro USB cable
3. You now have an emulated ZX81 with a working keyboard and display
### Next Steps
Read on for more information. The [Applications Tested](#applications-tested) section contains links to free downloads of some of the most iconic ZX80 and ZX81 software

# Use
## First Time
+ If the emulator is started with no SD Card, or with an empty SD Card, then it will emulate a 16K ZX81
+ To switch to always starting emulating a ZX80, a populated SD Card is required. If it is present, the machine that is emulated is specified by the `config.ini` file in the root directory, see [configuring the emulator](#configuring-the-emulator)
+ If the contents of the [examples](examples) directory have been copied to the SD Card, then the included programs can be loaded. Press `F2` to see files in the current directory that can be loaded
+ To make picozx81 emulate a an original 4K ZX80, without changing `config.ini`, either a ZX80 program can be loaded, or the computer type `ZX80-4K` can be selected from the Modify menu. Press F6 to bring up the Modify menu. To emaulate a ZX80 upgraded with a 8K ROM , select `ZX80-8K` from the modify menu
+ The current settings used by the emulator can be viewed by pressing `F3`
+ The following sections describe how to configure the emulator and provide links to programs that can be downloaded, copied to the SD Card and then run using the emulator

## Configuring the Emulator
The capabilities of the emulator are set using configuration files stored on the SD Card

### Configuration Files
Configuration attributes (described in the following sections) are set via `config.ini` files. When a program is loaded the emulation configuration is read, and set for the program before it is run.
The order for configuring an item for a given program (e.g. `prog.p`) is as follows:
1. Search for `config.ini` in the directory that contains `prog.p`
2. If it exists, configure items specified in a `[prog.p]` section
3. If it exists, configure items not yet specified, that exist in a `[default]` section
4. Search for `config.ini` in the root directory
5. If it exists, configure items not yet specified, that exist in a `[default]` section
6. Configure any items not yet specified with the default values given above

**Note:** All entries in a `config.ini` file are case insensitive
### Main Attributes
The following can be configured:
| Item | Description | Default Value | Notes |
| --- | --- | --- | --- |
| Computer | Selects ZX81, ZX81x2, ZX80-4K or ZX80-8K | ZX81 | ZX80-4K selects ZX80 with the original 4kB ROM. ZX80-8K selects a ZX80 that has been upgraded with an 8K ROM. ZX81x2 selects a ZX81 with the ["Big Bang"](https://www.sinclairzxworld.com/viewtopic.php?t=2986) ROM for faster BASIC |
| Memory | In kB. Starting at 0x4000 | 16 | 1, 2, 3, 4, 16, 32 and 48 allowed |
| WRX | Selects if RAM supports Hi-res graphics | Off | Automatically set to on if Memory is 2kB or less |
| LowRAM | Selects if RAM populated between 0x2000 and 0x3fff| Off | Typically used in conjunction with WRX to create a hires display file in low memory, can also be used for UDG graphics emulation if WRX off|
| M1NOT | Allows machine code to be executed between 0x8000 and 0xbfff| Off |Memory must be set to 32 or 48|
| ExtendFile| Enables the loading and saving of memory blocks for the ZX81, using ZXpand+ syntax|On| See [Loading and Saving Memory Blocks](#loading-and-saving-memory-blocks)|
| Centre | When enabled the usual 32 by 24 character display is centred on screen| On | When in 640 by 480 mode, set to Off for some programs that require the full 320 by 240 pixel display (e.g. [QS Defenda](http://www.zx81stuff.org.uk/zx81/tape/QSDefenda) or [MaxDemo](https://bodo4all.fortunecity.ws/zx/maxdemo.html))|
| FrameSync | Synchronises screen updates to the start of the display frame. Option to synchronise frame pairs for programs that display interlaced images| Off |`On` reduces "tearing" in programs with horizontal scrolling, at the expense of a possible small lag. `Interlaced` reduces flickering in programs that display interlaced images|
| CHR128 | Enables emulation of a 128 character user defined graphics board (CHR$128) in Low memory. | Off|When enabled LowRAM is forced to On, WRX and QSUDG are forced to off|
| QSUDG | Enables emulation of the QS user defined graphics board| Off |Memory automatically limited to 16 when selected |
| Sound | Selects sound card (if any) | Off | Valid options are `QUICKSILVA`, `ZONX`, `TV`, `CHROMA` and `OFF` |
| ACB | Enables ACB stereo if sound card enabled | Off |  |
| NTSC | Enables emulation of NTSC (60Hz display refresh)| Off | As for the "real" ZX81, SLOW mode is slower when NTSC is selected|
| VTOL | Specifies the tolerance in lines of the emulated TV display detecting vertical sync| 25 | See notes below|

**Notes:**
1. The "real" QS UDG board had a manual switch to enable / disable. In the emulator, if `QSUDG` is selected, it is assumed to be switched on after the first write to the memory mapped address range (0x8400 to 0x87ff)
2. To emulate other standard UDG graphics cards that reside between 0x2000 and 0x3fff set `LowRAM` to `On` and `WRX` to `Off`. This setting is needed to run e.g. [Galaxians with user defined graphics](https://sinclairzxworld.com/viewtopic.php?f=4&t=4388). If emulation of CHR128 UDG graphics is required set `CHR128` to `On`. This setting is needed to run e.g. [zedgragon](https://github.com/charlierobson/zedragon)
3. If `NTSC` is set to `On` and `Centre` is set to `Off` then a black vsync bar will be seen at the bottom of the display for programs that generate a typical 192 line display
4. A higher tolerance value set for `VTOL` results in faster screen stabilisation. As for a real TV, a low tolerance level results in vertical sync being lost for some programs, such as [QS Defenda](http://www.zx81stuff.org.uk/zx81/tape/QSDefenda) and [Nova2005](http://web.archive.org/web/20170309171559/http://www.user.dccnet.com/wrigter/index_files/NOVA2005.p). Set the value to 15 to emulate a TV that struggles to maintain vertical lock. Run the [Flicker program](examples/ZX81/flicker.p) to see the effects of PAUSE on lock
5. The "Big Bang" ROM can double the speed of BASIC programs
6. The Waveshare LCD 2.8 board has no sound capabilities
7. The `TV` sound option emulates the sound generated through the TV speaker by VSYNC pulses. The `CHROMA` sound option emulates the sound generated through the TV speaker by the Chroma interface when VSYNC pulses are not frame synchronised

### Joystick
In addition a USB joystick,and on some boards a 9-pin joystick, can be configured to generated key presses

| Item | Description | Default Value |
| --- | --- | --- |
| Left | Keypress when joystick moved left | 5 |
| Right | Keypress when joystick moved right | 8 |
| Up | Keypress when joystick moved up | 7 |
| Down | Keypress when joystick moved down | 6 |
| Button | Keypress when joystick button pressed | 0 |

Notes: ENTER and SPACE can be used to represent the New Line and Space keys, respectively
### Extra configuration options
Eight extra options apply across all programs and can only be set in the `[default]` section of the `config.ini` file in the root directory of the SD Card
| Item | Description | Default Value |
| --- | --- | --- |
| FiveSevenSix | Enables the generation of a 720x576p display @ 50Hz `On` , or 720x576p display @ 50.65Hz `Match`. If set to `Off` a 640x480 display @ 60Hz is produced | Off |
| Dir | Sets the initial default directory to load and save programs | / |
| Load | Specifies the name of a program to load automatically on boot in the directory given by `Dir` | "" |
| DoubleShift | Enables the generation of function key presses on a 40 key ZX80 or ZX81 keyboard. See [here](#function-key-menu)| On |
| AllFiles| When set, all files are initially displayed when the [Load Menu](#f2---load) is selected. When off only files with extensions `.p`, `.o`, `.81`, `.80` and `.p81` are initially displayed|Off|
| MenuBorder | Enables a border area (in characters) for the [Load](#f2---load) and [Pause](#f4---pause) menus, useful when using a display with overscan. Range 0 to 2| 1 |
| LoadUsingROM | Runs the Sinclair ROM routines to load a file in real-time. Authentic loading visual and audio effects are emulated | OFF |
| SaveUsingROM | Runs the Sinclair ROM routines to save a file in real-time. Authentic saving visual and audio effects are emulated | OFF |
| NinePinJoystick | When set to `on` Enables reading a 9 pin joystick, if supported in hardware | Off |
| VGA | When set to `on` enables VGA output for the PICOZX + LCD board | off |

**Notes:**
1. By default the European ZX81 generates frames slightly faster than 50Hz (50.65 Hz). Setting `FiveSevenSix` to `Match` enables a display mode slightly faster than the 50Hz TV standard, so that better synchronisation between the frame generates by the emulator and frames sent to the monitor can be achieved. If there are issues with a TV or monitor locking to 50.65 Hz, then `FiveSevenSix` can be set to `On` to generate an exact 50 Hz frame rate
2. The LCD supported displays all have a fixed 320 by 240 resolution. `FiveSevenSix` therefore only sets the framerate for these displays (50 Hz, 50.65 Hz or 60 Hz)
3. Due to the low speed of the ZX8x cassette interface, files can take many minutes to load and save when `LoadUsingROM` and `SaveUsingROM` is enabled

### Examples
Examples of the `config.ini` files used to test the programs listed in this [section](#applications-tested) can be found [here](examples)
### Editing `config.ini`
The `config.ini` file *cannot* be edited within the emulator. Modify the `config.ini` file using another computer.

After replacing the SD Card into the emulator, the pico *must* be restarted, either via a Power cycle, or by pressing the run / reset button on the board, before any edits will be visible to the emulator
### Need for reset of emulated machine
The emulated machine is always reset if any of the following options are changed:

`Computer` `Memory` `LowRAM` `M1NOT` `QSUDG` `CHR128` `LoadUsingROM` `SaveUsingROM`

**Note:** Changing the virtual sound card, or the `FrameSync` or `NTSC` settings, does *not* trigger a reset
## File Storage
Program and configuration files are stored on a micro SD-Card. Directories are supported. File and directory names should only contain characters that exist in the ZX81 character set. File and directory names can be a mixture of upper and lower case, but are used case insensitive. Therefore, all file and child directory names in a given directory must differ by more than just case
## Function key menu
The emulator has several menus that can be selected through function key presses. To keep the look and feel of the ZX8x computers the menus are in black and white and, dependent on the computer type being emulated, use either the ZX81 or ZX80 font

The original ZX80/ZX81 40 key keyboard does not have function keys. A "double shift" mechanism can be used instead. The mechanism is as follows:
1. Shift is pressed without another key
2. Shift is released, without another key being pressed
3. Within one second shift is pressed again
4. Shift is released, without another key being pressed
5. To generate a function key, within one second, a numeric key in the range `1` to `5` is pressed without shift being pressed. If `0` is pressed `Escape` is generated

This mechanism is enabled by default. To disable it set `DoubleShift` to `Off` in the configuration file
### F1 - Reset
Hard resets the emulator. It is equivalent to removing and reconnecting the power
### F2 - Load
A menu displaying directories and files that can be loaded is displayed, using the ZX81 font. Any sound that is playing is paused. Directory names are prepended by `<` and appended by `>` e.g. `<NAME>`

If the name of a file or directory is too long to display in full it is truncated with the last characters as `+` (file) and `+>` (directory)

+ The display can be navigated using the `up`, `down` and `enter` keys. The `7` key also generates `up` and the `6` key also generates `down`
+ For directories with a large number of files it is possible to move to the next page of files by using the `right`, `Page Down` or `8` key. To move to the previous page of files use the `left`, `Page Up` or `5` key
+ Press `A` to display all files in the directory. Press `P` to only display files with extensions `.p`, `.o`, `.81`, `.80` and .`p81`
+ Press `enter` whilst a directory entry is selected to move to that directory
+ Press `enter` when a file is selected to load that file
+ Press `Escape`, `space`, `Q` or `0` to return to the emulation without changing directory or loading a new program
### F3 - View Emulator Configuration
Displays the current emulator status. Any sound that is playing is paused. Note that this display is read only, no changes to the configuration can be made. Press `Escape`, `space`, `Q` or `0` to exit back to the running emulator
### F4 - Pause
Pauses the emulation. Handy if the phone rings during a gaming session! `P` is XORed into the 4 corners of the display to indicate that the emulator is paused. Press `Escape`, `space`, `Q` or `0` to end the pause and return to the running emulator
### F5 - Display Keyboard Overlay
The ZX80 and ZX81 use single key press BASIC entry. Pressing `F5` displays a 4 colour image (VGA) or a grey scale image (DVI / HDMI) representing the keyboard of the computer being emulated, so that the correct key presses can be determined. The image was taken from [sz81](https://github.com/SegHaxx/sz81). It is possible to enter commands whilst the keyboard is displayed

Press `Escape` to remove the keyboard display. The keyboard is also removed if another menu is selected

If a ZX8x 40 key keyboard is being used and `DoubleShift` is enabled, the menu can be removed by pressing and releasing shift twice and then pressing `0` within one second of releasing shift
### F6 - Modify
Allows some values to be modified "on the fly" to see the impact of the changes without having to edit the config files. Select the item to modify using the `up` and `down` keys. The `7` key also generates `up` and the `6` key also generates `down`. Change the value of an item using the `left` and `right` keys. The `5` key also generates `left` and the `8` key also generates `right`

The changes are *not* written back to the config files, so will be lost when the emulator is rebooted. Exit by pressing `Enter` to see the effect of the changes. Press `Escape` to exit without changes
### F7 - Restart
Allows some values to be modified to see the impact of the changes without having to edit the config files. Changing these values will result in the emulated machine being reset and returning to the `K` prompt. Select the item to modify using the `up` and `down` keys. The `7` key also generates `up` and the `6` key also generates `down`. Change the value of an item using the `left` and `right` keys. The `5` key also generates `left` and the `8` key also generates `right`

The changes are *not* written back to the config files, so will be lost when the emulator is rebooted. Exit by pressing `Enter` to action the changes. Press `Escape` to exit without changes
### F8 - Reboot
Allows the impact of changes to display resolution and frequency to be seen without editing config files. If a change is made and the menu is then exited by pressing `Enter` the Pico will reboot and use the new display mode. The changes are *not* written back to the main config files, so any changes will be lost on subsequent reboots.

On the LCD builds the display resolution is fixed and only the frequency can be changed
## Loading and saving options
The emulator supports loading `.p`, `.81`, `.o`, `.80` and `.p81` files from micro SD Card. It can save in `.p` and `.o` format.
Files to be loaded should only contain characters that are in the ZX81 or ZX80 character set
### Load
There are 3 ways to load files:
#### 1. Via the F2 menu
The user can navigate the SD card directory and select a file to load. The emulator is configured to the settings specified for the file in the `config.ini` files, reset and the new file loaded
#### 2. Via `LOAD ""` (ZX81) or `LOAD` (ZX80)
If the user enters the `LOAD` command without specifying a file name the SD Card directory menu is displayed and a file to load can be selected. The emulator is configured to the settings specified for the file in the `config.ini` files. Unlike for option 1, the emulator is only reset if the configuration differs. This, for example, allows for RAMTOP to be manually set before loading a program
#### 3. Via `LOAD "program-name"` (ZX81 only)
If a file name is specified, then `.p` is appended and an attempt is made to load the file from the current directory. The configuration for the file is read. A reset is performed only if required by a configuration change. This allows for multiple parts of an application to be loaded e.g. [HiRes Chess](https://spectrumcomputing.co.uk/entry/32021/ZX81/Hi-res_Chess) or [QS games](#qs-udg-graphics) that include character definitions.

If the supplied filename, with `.p` appended, does not exist, then the `LOAD` fails with error `D`. This is similar to a "real" ZX81, where if a named file is not on a tape, the computer will attempt to load until the user aborts by pressing `BREAK`, generating error `D`
### Save
#### ZX81
To save a program the `SAVE "Filename"` command is used. If `"Filename"` has no extension then `.p` is appended to the supplied file name. The file is saved in the current directory. If a file of the specified name already exists, it is overwritten

#### ZX80
To save a program the `SAVE` command is used. `SAVE` does not take a file name, so mechanisms are provided to supply a file name. When `SAVE` is executed the emulator scans the program for a `REM` statement of the form:

`REM SAVE "filename"`

If such a `REM` is found the file is saved with the name `filename` with `.o` appended if not supplied

**Note:** The `SAVE` in the `REM` is the keyword. Enter it first then use the cursor keys to insert the REM in front of it

If no `REM` statement of the required format is found, then a save screen will be displayed to allow a filename to be entered.
The ZX80 keyboard image is automatically displayed to make it easier to enter non alphanumeric characters. The cursor keys (`SHIFT 5` and `SHIFT 8`) and `Rubout` (`SHIFT 0`) can be used.
Press `ENTER` to exit the screen and use the filename, `.o` is appended if not supplied. Press `Esc` or `SHIFT 1` to leave the screen without setting a filename

The program is saved to the current directory. If no valid file name is supplied a default filename of `"zx80prog.o"` is used. Any existing file with the same name is overwritten

### Loading and Saving Memory Blocks
When emulating a ZX81, extensions are provided to `LOAD` and `SAVE` to support the loading and saving of memory blocks. The syntax is similar to that used by [ZXpand](https://github.com/charlierobson/ZXpand-Vitamins/wiki/ZXpand---Online-Manual)

**Note:** There are differences in failure modes and error reporting compared to the ZXpand. Also `.p` is *not* appended when loading and saving memory blocks

The extensions are enabled by default. Set the `ExtendFile` config option to `Off` to disable the extensions
#### Load
`LOAD "filename;nnnnn"`

where `nnnnn` represents a decimal number specifying the target address
##### Failure Modes
+ Error inverse `1`: The target address is not a number, or is outside of the range 0 to 65535
+ Error inverse `3`: No data would be written into existent RAM. Checks are made to ensure that data is not written to ROM, or non existing RAM addresses
+ Error `D`: The specified filename does not exist (no `.p` extension is added)

**Note:**
+ The string passed to `LOAD` is parsed for target address if and only if it contains at least 1 `;` the target address is assumed to be after the last `;` in the string. If the string does not contain a `;` it is treated as a single filename and a program file is loaded with the name of the supplied string (with `.p` appended if there is no `.` in the supplied string)
#### Save
`SAVE "filename;sssss,lllll"`

where:
+ `sssss` represents a decimal number specifying the start address of memory to be saved
+ `lllll` specifies the number of bytes of memory to be saved
##### Failure Modes
+ Error inverse `1`: The start address is not a number, or is outside of the range 0 to 65535
+ Error inverse `2`: The length address is not a number, or is outside of the range 1 to 65536
+ Error inverse `3`: The sum of the start address and the length is greater than 65536
**Notes:**
+ If a file with the specified name exists on the SD Card, it is overwritten
+ The string passed to `SAVE` is parsed for start and length if and only if it contains at least 1 `;` and a `,` exists in the string after the last `;`. Otherwise the string is treated as a single filename and a program file is saved with the name of the supplied string (with `.p` appended if there is no `.` in the supplied string)
### Directories
The original ZX81 used tape as a storage media, with no concept of a directory structure. The emulator uses an SD Card, and does support the basic use of directories.
#### Examples
+ `LOAD "../HIGHER"` will attempt to load the file `HIGHER.P` from the parent directory of the current directory
+ `LOAD "ABC/LOWER"` will attempt to load the file `LOWER.P` from the child directory with the name `ABC`

### Using the ROM routines
The `LoadUsingROM` and `SaveUsingROM` configuration options allow the ROM code to be executed. This emulates the loading and saving of program files (but not data blocks) in the same time that it would take on a real ZX80/ZX81

PicoZX81 generates realistic load and save sounds and graphics for the 8K ROM. The 4K ROM generates sounds and graphics when saving, which PicoZX81 emulates. The 4K ROM does not generate a load screen. PicoZX81 will show a black screen when the 4K ROM is loading a program

The save sounds generated for both the 4K and 8K ROMs have been recorded as wav files and then successfully loaded into the EightyOne emulator

The ROM is used for program loading if either the filename is specified on the command line, e.g. `LOAD "FILENAME.P"` or (for the ZX81) an empty filename is supplied e.g. `LOAD ""`. If a file extension exists (`.p`, `.o` etc) then it must be supplied. Directories can be specified as part of the filename e.g. `LOAD "SUBDIR/FILENAME.P"`

#### Limitations
The config file is read prior to loading. If the configuration file requires the emulator to be reconfigured so that it requires a reboot (e.g. the computer type or memory size changes) then the ROM will not be used for loading, and the file will be loaded immediately, as if `LoadUsingROM` was set to `OFF`. The ROM is also not used for loading if the file to be loaded is selected by pressing `F2`

When loading, picozx81 will read a `.p`, `.o` or `.p81` file and generate values which the ROM reads through IN statements  
**Note**: A program cannot be loaded directly from a wav file

# Applications Tested
Testing the emulator has been a great way to experience some classic ZX81 games and demos, including many that stretch the ZX81 and ZX80 well beyond what Sinclair may have originally expected. The following have been successfully tested:
## ZX81
### Low res
+ [clkfreq / clckfreq](https://www.zx81.nl/files.html)
  + Runs in the right number of frames (1863) when NTSC not selected
+ [Artic Galaxians](https://www.zx81stuff.org.uk/zx81/tape/Galaxians)
  + The one lo-res game that *had* to work correctly!
+ [3D Monster Maze](http://www.zx81stuff.org.uk/zx81/tape/3DMonsterMaze)
  + An iconic ZX81 game
+ [ZXTEST2](https://sinclairzxworld.com/viewtopic.php?f=6&t=685&p=6120&hilit=zxtest.zip#p6120)
  + This creates an image that periodically moves up and down by 1 pixel on a real ZX81. The emulator replicates this behaviour
+ [Maxtxt](https://bodo4all.fortunecity.ws/zx/maxdemo.html)
  + Creates a 40 by 30 character display
### Pseudo Hi-res
+ [Celebration and Lightning Display Driver](http://www.fruitcake.plus.com/Sinclair/ZX81/NewSoftware/Celebration.htm)
  + Set LowRAM on, and Memory to 48kB to view a colour version of celebration
+ [Z-Xtricator](http://www.zx81stuff.org.uk/zx81/tape/Z-Xtricator)
  + For the bomb (aka "Super Zapper") effects to render correctly `WRX` must be set to Off (see [here](https://www.sinclairzxworld.com/viewtopic.php?p=46499#p46499))
+ [Rocket Man](http://www.zx81stuff.org.uk/zx81/tape/RocketMan)
+ [War Web](http://www.fruitcake.plus.com/Sinclair/ZX81/Archive/PooterGames.htm)
+ [ZX81 Hires Invaders](https://www.perfectlynormalsite.com/hiresinvaders.html)
+ [High Resolution Invaders](http://www.fruitcake.plus.com/Sinclair/ZX81/Archive/OdysseyComputing.htm)
+ [Against The Elements](http://www.fruitcake.plus.com/Sinclair/ZX81/NewSoftware/AgainstTheElements.htm)
  + Set LowRAM on, and Memory to 48kB to view a colour version of celebration
### WRX
+ Dr Beep's amazing collection of [81 1K games](https://www.sinclairzxworld.com/viewtopic.php?f=4&t=552&start=220)
  + A sub-set have been tested. More will be test tested, but it is very easy to become distracted as they are very addictive! So far all appear to be displayed correctly
+ [SplinterGU SInvaders](https://github.com/SplinterGU/SInvaders)
  + A very faithful clone of the arcade Space Invaders
+ [MaxDemo](https://bodo4all.fortunecity.ws/zx/maxdemo.html)
  + Hi-res 320x240 and 40 by 30 characters. Set `Centre` to off to see the full image in 640 by 480 mode
  + The display routine is the most CPU intensive part of the emulator, so this is the best test that the emulator can run at 100% of the speed of "real" hardware
+ [HRDEMO3](https://www.zx81.nl/dload/utils/hrdemo3.p)
+ [HiRes Chess](https://spectrumcomputing.co.uk/entry/32021/ZX81/Hi-res_Chess)
  + All 26 lines are shown
+ [Bi-Plot Demo](http://www.pictureviewerpro.com/hosting/zx81/download/zx81/fred/biplot.zip)
+ [Othello](https://www.sinclairzxworld.com/viewtopic.php?t=1220)
+ [WRX1K1](https://quix.us/timex/rigter/1khires.html)
  + The assembler routine runs in 1K, but the BASIC demo program that draws concentric circles does not, as it requires more than 1K of RAM. This can be verified by using the BASIC listing tool in EightyOne to check the memory address of each line of BASIC. It will run in 2K
+ [zedit](https://forum.tlienhard.com/phpBB3/download/file.php?id=6795)
#### ARX
Set `WRX` to `Off` and `CHR128` to `on`
+ [ARX_Demo](https://www.sinclairzxworld.com/viewtopic.php?f=6&t=5448) Demonstrates ARX graphics
+ [ARX Hangman](https://sinclairzxworld.com/viewtopic.php?p=47308#p47308) Replicates the "off by 1 error" seen on real hardware and discussed [here](https://www.sinclairzxworld.com/viewtopic.php?f=6&t=5448)
### QS UDG Graphics
+ [QS Invaders](http://www.zx81stuff.org.uk/zx81/tape/QSInvaders)
+ [QS Asteroids](http://www.zx81stuff.org.uk/zx81/tape/QSAsteroids)
+ [QS Scramble](http://www.zx81stuff.org.uk/zx81/tape/QSScramble)
### Other UDG graphics
+ [HiRes Galaxians](http://zx81.eu5.org/files/soft/toddy/HR-Galax.zip)
+ [Airport HR](http://zx81.eu5.org/files/soft/toddy/AEROP-HR.zip)
#### CHR128
+ [zedragon](https://github.com/charlierobson/zedragon/blob/master/zedragon.p)
+ [Panic HR](http://zx81.eu5.org/files/soft/toddy/panicohr.zip)
### Sound
+ [Pink Panther Demo](http://zx81.eu5.org/files/soft/toddy/pinkpthr.zip)
+ [Bigg Oil](https://github.com/charlierobson/biggoil)
+ QS Programs
+ [Galaxians](https://sinclairzxworld.com/viewtopic.php?f=4&t=4388)
+ [SInvaders](https://github.com/SplinterGU/SInvaders)
+ [Dancing Demon](https://www.rwapsoftware.co.uk/zx81/software/Demon.zip)
+ [PT3 player](https://www.sinclairzxworld.com/viewtopic.php?p=44220#p44220) and [STC Player](https://www.sinclairzxworld.com/viewtopic.php?f=11&t=4222)
  + Use `config.ini` in [`sounds`](examples/ZX81/Sounds/config.ini)
  + **Note:** The `config.ini` file must be in the same directory as the `.p` file, therefore to run the `splash.p` files supplied with the PT3 players, move them to the same directory as the player and ensure that a copy of the `config.ini` file is in that directory
  + The ZXpand configuration line in the PT3 player, which appears as a `LLIST` command at line 9992, should be deleted
### NTSC
+ [Nova2005](http://web.archive.org/web/20170309171559/http://www.user.dccnet.com/wrigter/index_files/NOVA2005.p)
  + This is written for a 60Hz ZX81. For the second count to be accurate the `NTSC` option must be enabled. If `NTSC` is not selected the clock will run at 50/60 speed. Nova generates up to 26 rows of 34 characters. These are displayed correctly
  + To see the clock when `NTSC` is *not* enabled, `Centre` must be set to `Off`. In this case a vsync bar will be seen at the bottom of the display
+ [Spirograph](http://www.pictureviewerpro.com/hosting/zx81/download/zx81/fred/spiro.zip)
    + For the `QUICK` display mode to function correctly the `NTSC` option must be enabled. Works correctly in `FAST` and `SLOW` mode regardless of the `NTSC` option
### M1NOT
+ [Hot-Z II 64K](http://www.pictureviewerpro.com/hosting/zx81/download/zx81/Hot-Z_II.zip)
    + Requires at least 32kB of RAM. Runs correctly with `M1NOT` set to `On`. As expected, the emulated ZX81 crashes if `M1NOT` is set to `off`
### Chroma 81
To enable chroma support set LowRAM on, and Memory to 48kB
+ [Celebration](http://www.fruitcake.plus.com/Sinclair/ZX81/NewSoftware/Celebration.htm)
+ [Against The Elements](http://www.fruitcake.plus.com/Sinclair/ZX81/NewSoftware/AgainstTheElements.htm)
+ [Colour 3D Monster Maze](http://www.fruitcake.plus.com/Sinclair/ZX81/Chroma/ChromaInterface_Software_NativeColour.htm)
+ [Attribute Mode Test Program](http://www.fruitcake.plus.com/Sinclair/ZX81/Chroma/ChromaInterface_Software_NativeColour.htm)
+ [HiRes Galaxians](http://zx81.eu5.org/files/soft/toddy/HR-Galax.zip)
  + Ensure WRX RAM is disabled
+ [Chroma Slideshow](http://www.fruitcake.plus.com/Sinclair/ZX81/Chroma/ChromaInterface_Software_NativeColour.htm)
  + This works well with `FrameSync` set to `Interlaced`. Note that the program loads a series of image files. A config entry with `FrameSync` set to `Interlaced` needs to be created for each image file
+ [ROCK CRUSH 80](http://www.fruitcake.plus.com/Sinclair/ZX80/FlickerFree/ZX80_RockCrush80.htm)
  + This is a ZX80 game, but the `.p81` file will also run on the ZX81. If `SOUND` is set to `CHROMA` notes played at start-up can be heard
### TV (VSYNC) Sound
Set `SOUND` to either `TV` or `CHROMA`. If `TV` is selected a background tone will be generated during normal operation. This tone is created by VSYNC (ZX81) or VSYNC and HSYNC (ZX80). If `CHROMA` is selected a tone is only generated when vertical sync is lost
+ [Beatles](https://sinclairzxworld.com/download/file.php?id=1600&sid=0c88b58f81635c384508d040b05ffa70)
+ [Anogaia](https://sinclairzxworld.com/download/file.php?id=11084)
+ [Follin3ch](https://sinclairzxworld.com/download/file.php?id=11084)
### 16kB Demos
These really show off the capabilities of the ZX81 and are a good test that the emulator is accurate
#### Without WRX RAM
+ [Multi Scroller Demo](http://yerzmyey.i-demo.pl/multi_scroller_demo.zip)
  + Plays sound, and also demos various text scrolling techniques on a basic 16kB ZX81. This demo requires very specific timing, so shows the accuracy of the emulator
+ [There are no limits](http://www.ag1976.com/prods.html?prodid=55)
#### With WRX RAM
Both generate a display more than 320 pixels wide, so some information is not displayed in 640 by 480 mode (i.e. `FiveSevenSix` is set to `off`)
+ [25thanni](https://bodo4all.fortunecity.ws/zx/25thanni.html)
  + The scrolling "ticker" is more than 320 pixels wide, so all of it is not visible in 640 by 480 mode
+ [rezurrection](https://bodo4all.fortunecity.ws/zx/rezurrection.html)
  + The initial fast horizontal scrolling highlights the non-synchronised screen drawing of the emulator when running in 640 by 480 mode with `FrameSync` set to `Off`, leading to visible tearing
### Interlaced Images
+ [Ilena](https://www.sinclairzxworld.com/viewtopic.php?p=16058)
  + Best viewed with `FrameSync` set to `Interlaced`
## ZX80
+ [ZX80 3K QS DEFENDER](http://www.fruitcake.plus.com/Sinclair/ZX80/FlickerFree/ZX80_Defender.htm)
  + This game generates 32 lines of text. In 640 by 480 mode the emulator only displays 30 line of text. Set `Centre` to `off` so that the score, which is towards the top of the display, is visible. The game is still playable without the bottom of the display being visible. The full display is visible in 720x576 mode (i.e.`FixSevenSix` set to `On`). The QS sound board is emulated. `VTOL` has to be increased to display a stable image. This replicates the behaviour of this program of a "real" ZX80
+ [Breakout](http://www.fruitcake.plus.com/Sinclair/ZX80/FlickerFree/ZX80_Breakout.htm)
+ [Double Breakout](http://www.fruitcake.plus.com/Sinclair/ZX80/SoftwareArchive4K/BeamSoftware.htm)
+ [Kong](http://www.fruitcake.plus.com/Sinclair/ZX80/FlickerFree/ZX80_Kong.htm)
+ [Pacman](http://www.fruitcake.plus.com/Sinclair/ZX80/FlickerFree/ZX80_Pacman.htm)
+ [Metropolis](https://www.sinclairzxworld.com/viewtopic.php?t=2707)
  + This demo was a driver for adding a more accurate ZX80 emulation. It runs correctly for 4K and 8K ROMs with the latest version of picozx81
### Chroma 80
To enable chroma support set LowRAM on, and Memory to 48kB
+ [Colour Mode 1 Demo](http://www.fruitcake.plus.com/Sinclair/ZX80/Chroma/ZX80_ChromaInterface_Software_NativeColour.htm)
+ [ROCK CRUSH 80](http://www.fruitcake.plus.com/Sinclair/ZX80/FlickerFree/ZX80_RockCrush80.htm)
+ [Demo display drivers](http://www.fruitcake.plus.com/Sinclair/ZX80/Chroma/ZX80_ChromaInterface_Software_ExampleHiResDrivers.htm)

## Programs with limitations or artefacts
+ [QS Defenda](http://www.zx81stuff.org.uk/zx81/tape/QSDefenda)
  + This game generates 31 lines of text, one less than ZX80 QS DEFENDER. In 640 by 480 mode the emulator only displays 30 line of text. Set `Centre` to `off` to display the top lines, which include the score. The game is still playable without the bottom line being visible. The full display is visible in 720x576 mode (i.e.`FixSevenSix` set to `On`). The QS sound board is emulated correctly. `VTOL` has to be increased to display a stable image. This replicates the behaviour of this program of a "real" ZX81
+ [Wa-Tor](http://www.pictureviewerpro.com/hosting/zx81/download/zx81/fred/wator.zip)
  + This has a pseudo hi-res introduction screen. Towards the bottom of the screen is the text: "Experiment with predators and prey". On a "real" ZX81 the text is distorted, due to a variation in the time of the horizontal sync pulse. This emulator does not show the distortion
+ [rezurrection](https://bodo4all.fortunecity.ws/zx/rezurrection.html)
  + With default settings, the logo on the final screen "flashes" after the scrolling is complete. This is because the logo is displayed interlaced, at roughly 52Hz. Set `FrameSync` to `Interlaced` to display the final screen correctly without flashes.
  A frame rate adjusted version of the final Rezurrection screen exists in the [Demos](examples/ZX81/Demos) example directory: [head.p](examples/ZX81/Demos/heap.p) and [config.ini](examples/ZX81/Demos/config.ini). When run with `FiveSevenSix` and `FrameSync`  set to `on` or `interlaced` a stable interlaced image can be seen after the scrolling is complete
# Building
**Notes:**
+ Prebuilt executable files for the 7 supported board types can be found [here](uf2/)
+ If a **zip** of the source files is downloaded from GitHub, it will be **incomplete**, as the zip will not contain the submodules. Zip files of the submodules would have to be downloaded separately from GitHub. It is easier to clone recursive the repository, as described in the following section
### To build:
1. Install the Raspberry Pi Pico toolchain and SDK

    Instructions to do this for several operating systems can be found by downloading [this pdf](https://datasheets.raspberrypi.com/pico/getting-started-with-pico.pdf). Chapter 2 covers installation on the Raspberry Pi, chapter 9 describes the process for other operating systems
2. The vga display uses the `pico_scanvideo` library, which is part of the `pico-extras` repository. Clone this repository, including submodules

    `git clone --recursive https://github.com/raspberrypi/pico-extras.git`

3. Clone this repository (i.e. picozx81), including submodules

    `git clone --recursive https://github.com/ikjordan/picozx81.git`

4. create a build directory, move to that directory and build using CMake. By default an executable compatible with the Pimoroni vga board will be created.
This will be named `picozx81_vga.uf2`

    `mkdir build`  
    `cd build`  
    `cmake -DCMAKE_BUILD_TYPE=Release ..`  
    `make`
5. To build for other boards, pass the board type as part of the cmake command. e.g.

| Board | CMake | uf2 name |
| --- | --- | --- |
| Pimoroni DVI |`cmake -DPICO_BOARD=dviboard ..` | `picozx81_dvi.uf2`|
| PicoMiteVGA |`cmake -DPICO_BOARD=picomitevgaboard ..` | `picozx81_picomitevga.uf2`|
| Olimex PICO DVI |`cmake -DPICO_BOARD=olimexpcboard ..` | `picozx81_olimexpc.uf2`|
| Pimoroni VGA |`cmake -DPICO_BOARD=vgaboard ..` | `picozx81_vga.uf2`|
| Custom 332 VGA (similar to MCUME)|`cmake -DPICO_BOARD=vga332board ..`| `picozx81_vga332.uf2`|
| Cytron Maker based 222 VGA with CSYNC (similar to PICOZX)|`cmake -DPICO_BOARD=vgamaker222cboard ..`| `picozx81_vgamaker222c.uf2`|
| PICOZX without LCD |`cmake -DPICO_BOARD=picozxboard ..`| `picozx81_picozx.uf2`|
| PICOZX with LCD |`cmake -DPICOZX_LCD=ON -DPICO_BOARD=picozxboard ..`| `picozx81_picozx_lcd.uf2`|
| PICOZX in Spectrum case |`cmake -DPICO_BOARD=picozxrealboard ..`| `picozx81_picozxreal.uf2`|
| Pimoroni DVI with HDMI sound|`cmake -DHDMI_SOUND=ON -DPICO_BOARD=dviboard ..` | `picozx81_dvi_hdmi_sound.uf2`|
| Olimex PICO DVI with HDMI sound|`cmake -DHDMI_SOUND=ON -DPICO_BOARD=olimexpcboard ..` | `picozx81_olimexpc_hdmi_sound.uf2`|
| Wavesare PiZero with HDMI sound|`cmake -DHDMI_SOUND=ON -DPICO_BOARD=wspizeroboard ..` | `picozx81_wspizero_hdmi_sound.uf2`|
| Waveshare Pico-ResTouch-LCD-2.8|`cmake -DPICO_BOARD=lcdws28board ..`| `picozx81_lcdws28.uf2`|
| Cytron Maker|`cmake -DPICO_BOARD=lcdmakerboard ..`| `picozx81_lcdmaker.uf2`|

**Notes:**
+ The [`buildall`](buildall) script in the root directory of `picozx81` will build `uf2` files for all supported board types

6. Upload the `uf2` file to the pico
7. Populate a micro SD Card with files you wish to run. Optionally add `config.ini` files to the SD Card. See [here](examples) for examples of config files

# Extra Information
## General
+ The original intention of the emulator was to provide an authentic '80s feel. It emulated the hardware that was advertised in the early '80s i.e. QS UDG, Sound, joystick, hi-res mono graphics. It has now been extended to provide emulation of some of the amazing ZX81 developments of recent years, such as [Chroma 81](http://www.fruitcake.plus.com/Sinclair/ZX81/Chroma/ChromaInterface.htm). It supports the loading and saving of memory blocks, using a syntax similar to ZXpand
+ The ["Big Bang"](https://www.sinclairzxworld.com/viewtopic.php?t=2986) ROM is supported, as this accelerates BASIC execution, and runs on the original ZX81 hardware
+ Program debug support is limited to that provided by the ZX81 "in period", i.e. non-existent. It is recommended that one of the PC or Linux based ZX81 emulators with single step and breakpoint support are used to debug Z80 assembly programs
+ To achieve a full speed emulation the Pico is overclocked to 250MHz (640x480) and 270MHz (720x576). There is a very slight risk that this may damage the Pico. However many other applications run the Pico at this frequency. By default the stock voltage is used (1.1V), this has been successfully tested on multiple Picos. If the emulator appears unstable it can be built to use 1.2V, add `-DOVER_CLOCK` to the cmake command
+ The Pico only has 1 USB port. The Pimoroni, Olimex and Waveshare PiZero boards can be powered through a second on board USB power connector, allowing a keyboard to be connected to the Pico using an OTG adaptor
+ To connect more than one peripheral (e.g. a keyboard and joystick) at the same time, a powered USB OTG hub is required. These 3 hubs have been successfully tested. [1](https://www.amazon.co.uk/dp/B083WML1XB), [2](https://www.amazon.co.uk/dp/B078M3Z84Z), [3](https://www.amazon.co.uk/dp/B07Z4RHJ2D). Plug the hub directly into the USB port on the Pico. The USB-A connector on the PICOZX boards can also be used  
**Note:** Testing has shown that all of these hubs can support OTG and power delivery to the Pico simultaneously
+ On rare occasion, some USB keyboards and joysticks fail to be detected when connected via powered hubs. A re-boot of the Pico often results in successful detection
## Board Specific
### Waveshare PiZero
+ The Waveshare PiZero has two USB-C connectors. Use the connector closest to the HDMI connector to provide power. Connect a keyboard to the other USB port using an OTG cable. If necessary, a female micro USB to male USB C adaptor can be used
+ The board can be back powered by some TVs. This can cause the board to not start correctly. If this happens either connect the power before attaching the HDMI cable, or press the reset button on the board
### PicoMiteVGA
+ The PicoMiteVGA board has a PS/2 keyboard socket. Currently this is not supported, a USB keyboard must be used
+ PicomiteVGA only supports 1 level of Red and Blue, so it cannot display the full range of colours that Chroma can generate
+ Some versions of the PicoMiteVGA board have a jumper to select between RGB and GRN mode. Select RGB mode
### PICOZX
+ PICOZX has a bank of 4 extra keys below the SD Card. These act as function keys. `Menu` maps to `F1`, `Reload` to `F2` etc. If shift is pressed 4 is added to the function number. e.g. `shift` + `Menu` gives `F5` (and so displays the keyboard)
+ The PICOZX + LCD code generates LCD output by default. To enable VGA output either hold down the the Fire button at start-up, or enable `VGA` in the config file
+ The PICOZX + LCD shares outputs with VGA. If the board is configured for VGA output, the intensity of the backlight of the LCD will vary with the contents of the VGA display
+ The PICOZX for the ZX-Spectrum case has two extra buttons on the back (in addition to a reset button). Without shift these two buttons will generate `F2` and `F5`. With shift they will generate `F3` and `F6`
+ Use the double shift mechanism to access all menus when using the PICOZX family
+ To enter BOOTSEL mode on the PICOZX for the ZX-Spectrum press and hold the `R` key then press the `F2` menu key. This allows new firmware to be loaded without needing to press the BOOTSEL key on the pico
### Waveshare Pico-ResTouch-LCD-2.8
+ The Waveshare Pico-ResTouch-LCD-2.8 board has a touch controller, but the emulator does not support its use
### Olimex RP2040-PICO-PC
+ The Olimex RP2040-PICO-PC board does not supply 5v to DVI pin 18. This may result in the board not being detected by some TVs. If necessary short the SJ1 connector so 5V is supplied
+ If SJ1 is shorted the board may be back powered by some TVs. This can cause the board to not start correctly. If this happens either connect the power before attaching the HDMI cable, or press the reset button on the board

### Cytron Maker
+ The Cytron Maker Pi Pico has an onboard piezo buzzer. The audio quality is poor, but it can be used instead of speakers. If the buzzer is enabled (using the switch on the maker board) ensure that ACB Stereo is disabled
+ The vgamaker222c build requires the following connections:
<p align="middle">
<img src="images/rgb_222_vga.png" width="50%" />
</p>

#### Use of LCD displays
The Maker board can be used with a range of 320 by 240 LCDs, controlled over the SPI bus, with controllers from either the ILI9341 or ST7789 families. The boards are configured using entries in the `default` section of the config file

The LCD should be connected to the Maker board as follows:
| Function | Name |Pico GPIO Pin|
| --- | --- | --- |
| Backlight| BL| 4 |
| Chip Select | CS | 5 |
| Clock | CLK | 6 |
| Data In | DIN or MOSI | 7 |
| Reset | RST | 8 |
| Data / Command Selection | DC | 9 |
| VCC | 3.3V | 3V3 (OUT) |
| GND | Ground | Any Ground Pin |

Touchscreen functionality is not supported. Any pins used for the touchscreen do not need to be connected

## LCD Configuration Options
All options are set in the `[default]` section of the `config.ini` file in the root directory of the SD Card.
| Item | Description | Default Value |
| --- | --- | --- |
| LCDInvertColour | Inverts the colour of the display. i.e. changes white to black and black to white | False |
| LCDReflect | Defines the horizontal scan direction. Use if the `K` prompt is displayed on the right hand side of the display | False |
| LCDBGR | Set to true if blue displays as red and red displays as blue | False |
| LCDRotate | Rotates the display through 180 degrees | False |
| LCDSkipFrame | Displays every other frame, to reduce bandwidth | False |

 e.g. to set `LCDReflect` to true, add the following to the `[default]` section of the configuration file: `LCDReflect = True`

 **Notes:**
 1. If the configuration appears correct for a display, but no image appears, or the image is not stable, it could be that the display cannot support the SPI bus speed required to display every frame, or that cross talk is occuring between the wires connecting the display. In this case set `LCDFrameSkip = True`
 2. It is recommended to use the Cytron maker board with the Pico pre-soldered, as this version can support higher bus speeds. If the version with a socket for a Pico is used, then the LCD display may not function correctly unless `LCDFrameSkip` is set to `True`
 3. If `LCDFrameSkip` equals `True`, then if `FrameSync` is set to `Interlaced` it will be interpreted as `On`

### Configuration of Tested LCDs

| Display | Controller | Invert Colour | Reflect | Frame Skip | BGR |
| --- | --- | --- | --- | --- | --- |
| [Waveshare 2" LCD](https://www.waveshare.com/wiki/2inch_LCD_Module) | ST7789V |True | False | True | False |
| [Waveshare 2.4" LCD](https://www.waveshare.com/2.4inch-lcd-module.htm) | ILI9341 |False | False | False | True |
| [Waveshare 2.8" LCD](https://www.waveshare.com/2.8inch-resistive-touch-lcd.htm) | ST7789 |True | True | False | False |
| [Generic 3.2" LCD](http://www.lcdwiki.com/3.2inch_SPI_Module_ILI9341_SKU:MSP3218) | ILI9341 |False | False | False | True |

Example `config.ini` settings for these LCDs can be seen [here](examples/config.ini). Uncomment the lines in the section matching the LCD you wish to use
### Rotating the display on the Waveshare Pico-ResTouch-LCD-2.8
By default the display for the Waveshare Pico-ResTouch-LCD-2.8 is configured rotated, so that the usb connection and SD Card is at the bottom of the display. To undo the rotation, so that usb connection and SD Card is at the top, set `LCDRotate = False` in the default section of the config file

## Using 9 pin "Atari" joysticks
With boards with connectors supplied for enough free GPIO pins it is possible to attach a 9 pin connector and then plug-in and use "in period" 9-pin joysticks

### Supported Boards
The lcdmaker, vgamaker222c, picomitevga, pizero and picozx builds support the connection of a 9-pin joystick connector

### Obtaining a 9-pin interface
Solderless 9-Pin connectors can be sourced from e.g. ebay or [amazon](https://www.amazon.co.uk/sourcing-map-Breakout-Connector-Solderless/dp/B07MMMGGXP)
### Connections

| pin number | 1 | 2 | 3 | 4 | 6 | 8 |
| --- | --- | --- | --- | --- | --- | --- |
| lcdmaker | GP20 | GP21 | GP22 | GP26 | GP27 | Ground |
| vgamaker222c | GP20 | GP21 | GP22 | GP26 | GP27 | Ground |
| picomitevga | GP3 | GP4 | GP5 | GP22 | GP26 | Ground |
| pizero | GP11 | GP12 | GP10 | GP15 | GP13 | Ground |

The picozx board has a 9-pin joystick port connector built in
### Enabling the joystick
To enable the nine pin joystick set `NinePinJoystick` to `On` in the `[default]` section of the `config.ini` file in the root directory

It is not necessary to create a `NinePinJoystick` entry to use the joystick port on the picozx board

# Developer Notes
## Acknowledgements
One intention of this project was to show what can be quickly achieved by leveraging other open source projects. The following excellent Open source projects have been used for source code and inspiration:
+ [sz81](https://github.com/SegHaxx/sz81)
+ EightyOne [Releases](https://sourceforge.net/projects/eightyone-sinclair-emulator/) [source](https://github.com/charlierobson/EightyOne)
+ [MCUME](https://github.com/Jean-MarcHarvengt/MCUME)
+ [pico-zxspectrum](https://github.com/fruit-bat/pico-zxspectrum)
+ [Pico DVI](https://github.com/Wren6991/PicoDVI)
+ [Pimoroni SD Card driver](https://github.com/pimoroni/pimoroni-pico/tree/main/drivers/sdcard)
+ [FatFS](http://elm-chan.org/fsw/ff/00index_e.html)
+ Config file parsing [inih](https://github.com/benhoyt/inih)

Thanks to [Paul Farrow](http://www.fruitcake.plus.com/) for information on the Chroma expansion boards and the `.p81` file format
## Use with an original ZX80/ZX81 keyboard
There are not enough unused GPIO pins on the Pimoroni demo boards to allow the direct connection of a ZX80/81 keyboard, but it can be done by using another Pico to convert the ZX80/81 keyboard into a USB keyboard. It may seem excessive to use a whole pico as a keyboard controller, but they are cheap and there is enough space to put the Pimoroni board, plus another Pico, plus a small USB hub into a ZX80 or ZX81 case

Code to convert a ZX8x keyboard to USB can be found at [ZX81_USB_KBD](https://github.com/ikjordan/ZX81_USB_KBD). This code has been used to successfully connect a ZX81 keyboard to this emulator. If the keyboard is the only peripheral, then it can be plugged straight into the USB port of the Pico on the emulator board with the power connected to the USB power socket of the Pimoroni board. If other USB peripherals (such as another keyboard, or a USB joystick) also need to be connected then the ZX80/81 keyboard can be connected via a USB hub

To access the function menus from a ZX80/81 keyboard the `doubleshift` configuration option must be enabled

The picozx board does support keyboard and joystick. This is achieved by using every available GPIO pin, and using VGA222 with CSYNC, together with mono audio

## Performance and constraints
In an ideal world the latest versions of the excellent sz81 or EightyOne emulators would have been ported. An initial port showed that they are too processor intensive for an (overclocked) ARM M0+. An earlier version of sz81 ([2.1.8](https://github.com/ikjordan/sz81_2_1_8)) was used as a basis, with some Z80 timing corrections and back porting of the 207 tstate counter code from the latest sz81 (2.3.12). See [here](#applications-tested) for a list of applications tested

The initial port from sz81 2.3.12 onto the Pico ran at approximately 10% of real time speed. Use of the Z80 emulator originally written for xz80 by Ian Collier, plus optimisation of the ZX81 memory access, display and plot routines allows the emulator to run at 100% of real time speed. The display of a full 320 by 240 image in real time (e.g. [Maxhrg](https://bodo4all.fortunecity.ws/zx/maxdemo.html)) uses approximately 92% of the available CPU clock cycles with sound disabled and 96% with Zonx sound enabled when picozx81 is running with a 640x460 display

The 640x480 display mode uses an overclock to 252MHz. The 720x576 display mode uses an overclock to 270MHz

Corrections to the tstate timings were made for `ld a,n; ld c,n; ld e,n; ld l,n; set n,(hl); res n,(hl);`
## Possible Future Developments
+ Support for USB gamepads as well as joysticks
+ Move to a Pi Zero to greatly increase processing power and use [circle](https://github.com/rsta2/circle) for fast boot times
## Comparison to MCUME
[MCUME](https://github.com/Jean-MarcHarvengt/MCUME/) demonstrated that a Raspberry Pi Pico based ZX80/81 emulator was feasible. The custom VGA RGB 332 board type is similar to the hardware required for MCUME

This emulator offers the following over MCUME:
+ Support for USB keyboards and joysticks
+ Emulation runs at full speed of a 3.25MHz ZX81
+ Ability to save files
+ Ability to load a program without reset
+ Support for Hi-res and pseudo Hi-res graphics
+ Support for multiple DVI, VGA and LCD boards
+ Support for Chroma 80 and Chroma 81
+ Support for programs which use more than 32 columns or 24 rows of characters
+ ZonX, QS and TV Sound emulation
+ Emulated QS UDG
+ 50Hz and 60Hz emulation
+ Emulator display refresh decoupled from Pico display rate
+ Mapping of joystick inputs to specific key presses
+ Hardware configuration associated with specific program files
+ Builds under Linux (MCUME ZX80/81 emulator fails to build in a case sensitive file system)


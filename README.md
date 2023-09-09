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
+ Runs on the [Pimoroni Pico VGA demo board](https://shop.pimoroni.com/products/pimoroni-pico-vga-demo-base),
the [Pimoroni Pico DVI demo board (HDMI)](https://shop.pimoroni.com/products/pimoroni-pico-dv-demo-base), the [PicoMite VGA board](https://geoffg.net/picomitevga.html) and the [Olimex RP2040-PICO-PC](https://www.olimex.com/Products/MicroPython/RP2040-PICO-PC/open-source-hardware)
+ Supports sound over onboard DAC or PWM when available in hardware
+ Provides an immersive full screen experience, with a very fast boot time and no operating system
+ Simultaneous USB keyboard and joystick support (using a powered USB hub)
+ Can be fully controlled from a ZX81 style 40 key keyboard
+ The small form factor makes the board easy to mount in period or reproduction cases. The low cost, relatively low performance and software generated display of the Pico is a 21st century analogue for the ZX80 and ZX81
+ Emulates pseudo and Hi-res graphics
+ Emulates ZonX and Quicksilva sound
+ Emulates user defined graphics, including CHR$128 and QS User Defined Graphics
+ Emulation runs at accurate speed of a 3.25MHz ZX81
+ Emulates European and US configuration (i.e. emulates 50Hz and 60Hz ZX81)
+ Supports larger ZX81 generated displays of over 320 by 240 pixels (40 character width and 30 character height)
+ Load `.p`, `.81`, `.o` and `.80` files from micro SD Card. Save `.p` and `.o` files
+ Can display at 640x480 or 720x576 (for an authentic display on a UK TV)
+ 720x576 can be configured to run at a frame rate to match the "real" ZX81 (~50.65 Hz).
+ An interlaced mode can be selected to display interlaced images with minimal flicker
+ Supports loading and saving of memory blocks using [ZXpand like syntax](https://github.com/charlierobson/ZXpand-Vitamins/wiki/ZXpand---Online-Manual#load)
+ Set-up of emulator (computer type, RAM, Hi-Res graphics, sound, joystick control etc) configurable on a per program basis, using config files
+ Optionally displays graphic of keyboard (taken from [sz81](https://github.com/SegHaxx/sz81)). Can type in code with keyboard visible
+ Can be extended for other board types. Code support included for a custom VGA RGB 332 board similar to that supported by [MCUME](https://github.com/Jean-MarcHarvengt/MCUME)

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
| PicoMite VGA | [`picozx81_picomitevga.uf2`](https://github.com/ikjordan/picozx81/releases/latest/download/picozx81_picomitevga.uf2)|


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
+ To make picozx81 emulate a ZX80, without changing `config.ini`, either a ZX80 program can be loaded, or the computer type `ZX80` can be selected from the Modify menu. Press F6 to bring up the Modify menu
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
| Computer | Selects either ZX81, ZX81x2 or ZX80 | ZX81 | ZX80 assumes 4kB ROM. ZX81x2 selects a ZX81 with the ["Big Bang"](https://www.sinclairzxworld.com/viewtopic.php?t=2986) ROM for faster BASIC |
| Memory | In kB. Starting at 0x4000 | 16 | 1, 2, 3, 4, 16, 32 and 48 allowed |
| WRX | Selects if RAM supports Hi-res graphics | Off | Automatically set to on if Memory is 2kB or less |
| LowRAM | Selects if RAM populated between 0x2000 and 0x3fff| Off | Typically used in conjunction with WRX to create a hires display file in low memory, can also be used for UDG graphics emulation if WRX off|
| M1NOT | Allows machine code to be executed between 0x8000 and 0xbfff| Off |Memory must be set to 32 or 48|
| ExtendFile| Enables the loading and saving of memory blocks for the ZX81, using ZXpand+ syntax|On| See [Loading and Saving Memory Blocks](#loading-and-saving-memory-blocks)|
| Centre | When enabled the usual 32 by 24 character display is centred on screen| On | When in 640 by 480 mode, set to Off for some programs that require the full 320 by 240 pixel display (e.g. [QS Defenda](http://www.zx81stuff.org.uk/zx81/tape/QSDefenda) or [MaxDemo](https://bodo4all.fortunecity.ws/zx/maxdemo.html))|
| FrameSync | Synchronises screen updates to the start of the display frame. Option to synchronise frame pairs for programs that display interlaced images| Off |On reduces "tearing" in programs with horizontal scrolling, at the expense of a possible small lag. Interlaced reduces flickering in programs that display interlaced images|
| CHR128 | Enables emulation of a 128 character user defined graphics board (CHR$128) in Low memory. | Off|When enabled LowRAM is forced to On, WRX and QSUDG are forced to off|
| QSUDG | Enables emulation of the QS user defined graphics board| Off |Memory automatically limited to 16 when selected |
| Sound | Selects sound card (if any) | Off | Quicksilva and ZonX supported |
| ACB | Enables ACB stereo if sound card enabled | Off |  |
| NTSC | Enables emulation of NTSC (60Hz display refresh)| Off | As for the "real" ZX81, SLOW mode is slower when NTSC is selected|
| VTOL | Specifies the tolerance in lines of the emulated TV display detecting vertical sync| 100 | See notes below|

**Notes:**
1. The "real" QS UDG board had a manual switch to enable / disable. In the emulator, if `QSUDG` is selected, it is assumed to be switched on after the first write to the memory mapped address range (0x8400 to 0x87ff)
2. To emulate other standard UDG graphics cards that reside between 0x2000 and 0x3fff set `LowRAM` to `On` and `WRX` to `Off`. This setting is needed to run e.g. [Galaxians with user defined graphics](https://sinclairzxworld.com/viewtopic.php?f=4&t=4388). If emulation of CHR128 UDG graphics is required set `CHR128` to `On`. This setting is needed to run e.g. [zedgragon](https://github.com/charlierobson/zedragon)
3. If `NTSC` is set to `On` and `Centre` is set to `Off` then a black vsync bar will be seen at the bottom of the display for programs that generate a typical 192 line display
4. A higher tolerance value set for `VTOL` results in faster screen stabilisation. As for a real TV, a low tolerance level results in vertical sync being lost for some programs, such as [QS Defenda](http://www.zx81stuff.org.uk/zx81/tape/QSDefenda) and [Nova2005](http://web.archive.org/web/20170309171559/http://www.user.dccnet.com/wrigter/index_files/NOVA2005.p). The default value of 100 emulates a TV that very quickly regains vertical lock. Set the value to 15 to emulate a TV that struggles to maintain vertical lock. Run the [Flicker program](examples/ZX81/flicker.p) to see the effects of PAUSE on lock
5. The "Big Bang" ROM can double the speed of BASIC programs
### Joystick
In addition a USB joystick can be configured to generated key presses

| Item | Description | Default Value |
| --- | --- | --- |
| Left | Keypress when joystick moved left | 5 |
| Right | Keypress when joystick moved right | 8 |
| Up | Keypress when joystick moved up | 7 |
| Down | Keypress when joystick moved down | 6 |
| Button | Keypress when joystick button pressed | 0 |

Notes: ENTER and SPACE can be used to represent the New Line and Space keys, respectively
### Extra configuration options
Six extra options apply across all programs and can only be set in the `[default]` section of the `config.ini` file in the root directory of the SD Card
| Item | Description | Default Value |
| --- | --- | --- |
| FiveSevenSix | Enables the generation of a 720x576p display @ 50Hz `On` , or 720x576p display @ 50.65Hz `Match`. If set to `Off` a 640x480 display @ 60Hz is produced | Off |
| Dir | Sets the initial default directory to load and save programs | / |
| Load | Specifies the name of a program to load automatically on boot in the directory given by `Dir` | "" |
| DoubleShift | Enables the generation of function key presses on a 40 key ZX80 or ZX81 keyboard. See [here](#function-key-menu)| On |
| AllFiles| When set, all files are initially displayed when the [Load Menu](#f2---load) is selected. When off only files with extensions `.p`, `.o`, `.81` and `.80` are initially displayed|Off|
| MenuBorder | Enables a border area (in characters) for the [Load](#f2---load) and [Pause](#f4---pause) menus, useful when using a display with overscan. Range 0 to 4| 1 |

**Note:** By default the European ZX81 generates frames slightly faster than 50Hz (50.65Hz). Setting `FiveSevenSix` to `Match` enables a display mode slightly faster than the 50Hz TV standard, so that better synchronisation between the frame generates by the emulator and frames sent to the monitor can be achieved. If there are issues with a TV or monitor locking to 50.6 Hz, then `FiveSevenSix` can be set to `On` to generate an exact 50Hz frame rate

### Examples
Examples of the `config.ini` files used to test the programs listed in this [section](#applications-tested) can be found [here](examples)
### Editing `config.ini`
The `config.ini` file *cannot* be edited within the emulator. Modify the `config.ini` file using another computer.

After replacing the SD Card into the emulator, the pico *must* be restarted, either via a Power cycle, or by pressing the run / reset button on the board, before any edits will be visible to the emulator
### Need for reset of emulated machine
The emulated machine is always reset if any of the following options are changed:

`Computer` `Memory` `LowRAM` `M1NOT` `QSUDG` `CHR128`

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
+ Press `A` to display all files in the directory. Press `P` to only display files with extensions `.p`, `.o`, `.81` and `.80`
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
Allows the impact of changes to display resolution and frequency to be seen without editing config files. If change are made and the menu is then exited by pressing `Enter` the Pico will reboot and use the new display mode. The changes are *not* written back to the main config files, so any changes will be lost on subsequent reboots
## Loading and saving options
The emulator supports the loading `.p`, `.81`, `.o` and `.80` files from micro SD Card. It can save in `.p` and `.o` format.
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
When emulating a ZX81, extensions are provided to `LOAD` and `SAVE` to support the loading and saving of memory blocks. The syntax is similar to that used by [ZXpand](https://github-wiki-see.page/m/charlierobson/ZXpand-Vitamins/wiki/ZXpand---Online-Manual)

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

# Applications Tested
Testing the emulator has been a great way to experience some classic ZX81 games and demos, including many that stretch the ZX81 and ZX80 well beyond what Sinclair may have originally expected. The following have been successfully tested:
## ZX81
### Low res
+ [clkfreq / clckfreq](https://www.zx81.nl/files.html)
  + Runs in the right number of frames (1863) when NTSC not selected
+ [Artic Galaxians](https://www.zx81stuff.org.uk/zx81/tape/Galaxians)
  + The one lo-res game that *had* to work correctly!
+ [ZXTEST2](https://sinclairzxworld.com/viewtopic.php?f=6&t=685&p=6120&hilit=zxtest.zip#p6120)
  + This creates an image that periodically moves up and down by 1 pixel on a real ZX81. The emulator replicates this behaviour
+ [Maxtxt](https://bodo4all.fortunecity.ws/zx/maxdemo.html)
  + Creates a 40 by 30 character display
### Pseudo Hi-res
+ [Celebration and Lightning Display Driver](http://www.fruitcake.plus.com/Sinclair/ZX81/NewSoftware/Celebration.htm)
+ [Z-Xtricator](http://www.zx81stuff.org.uk/zx81/tape/Z-Xtricator)
  + For the bomb (aka "Super Zapper") effects to render correctly `WRX` must be set to Off (see [here](https://www.sinclairzxworld.com/viewtopic.php?p=46499#p46499))
+ [Rocket Man](http://www.zx81stuff.org.uk/zx81/tape/RocketMan)
+ [War Web](http://www.fruitcake.plus.com/Sinclair/ZX81/Archive/PooterGames.htm)
+ [ZX81 Hires Invaders](https://www.perfectlynormalsite.com/hiresinvaders.html)
+ [High Resolution Invaders](http://www.fruitcake.plus.com/Sinclair/ZX81/Archive/OdysseyComputing.htm)
+ [Against The Elements](http://www.fruitcake.plus.com/Sinclair/ZX81/NewSoftware/AgainstTheElements.htm)
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
### QS UDG Graphics
+ [QS Invaders](http://www.zx81stuff.org.uk/zx81/tape/QSInvaders)
+ [QS Asteroids](http://www.zx81stuff.org.uk/zx81/tape/QSAsteroids) + [QS Scramble](http://www.zx81stuff.org.uk/zx81/tape/QSScramble)
### Other UDG graphics
+ [HiRes Galaxian](http://zx81.eu5.org/files/soft/toddy/HR-Galax.zip)
+ [Airport HR](http://zx81.eu5.org/files/soft/toddy/AEROP-HR.zip)
#### CHR128
+ [zedragon](https://github.com/charlierobson/zedragon/blob/master/zedragon.p)
+ [Panic HR](http://zx81.eu5.org/files/soft/toddy/panicohr.zip)
### Sound
+ [Pink Panther Demo](http://zx81.eu5.org/files/soft/toddy/pinkpthr.zip)
  + When configured for mono, channels A and B destructively interfere at around 2 mins 15 seconds, causing silence for 1 to 2 seconds. Selecting ACB stereo avoids this
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
### 16kB Demos
These really show off the capabilities of the ZX81 and are a good test that the emulator is accurate
#### Without WRX RAM
+ [Multi Scroller Demo](http://yerzmyey.i-demo.pl/multi_scroller_demo.zip)
  + Plays sound, and also demos various text scrolling techniques on a basic 16kB ZX81. This demo requires very specific timing, so shows the accuracy of the emulator
+ [There are no limits](http://www.ag1976.com/prods.html?prodid=55)
#### With WRX RAM
Both generate a display more than 320 pixels wide, so some information is not displayed in 640 by 480 mode (i.e. `FiveSevenSix` is set to `off`)
+ [25thanni](https://bodo4all.fortunecity.ws/zx/25thanni.html)
  + The scrolling "ticker" is more than 320 pixels wide, so all of it is not visible in 640 by 480 mode.
+ [rezurrection](https://bodo4all.fortunecity.ws/zx/rezurrection.html)
  + The initial fast horizontal scrolling highlights the non-synchronised screen drawing of the emulator when running in 640 by 480 mode with `FrameSync` set to `Off`, leading to visible tearing
### Interlaced Images
+ [Ilena](https://www.sinclairzxworld.com/viewtopic.php?p=16058)
  + Best viewed with `FrameSync` set to `Interlaced`
## ZX80
+ [ZX80 3K QS DEFENDER](http://www.fruitcake.plus.com/Sinclair/ZX80/FlickerFree/ZX80_Defender.htm)
  + This game generates 32 lines of text. The bottom two line is not displayed in the emulator, but the game is still playable. The QS sound board is emulated
+ [Breakout](http://www.fruitcake.plus.com/Sinclair/ZX80/FlickerFree/ZX80_Breakout.htm)
+ [Double Breakout](http://www.fruitcake.plus.com/Sinclair/ZX80/SoftwareArchive4K/BeamSoftware.htm)
+ [Kong](http://www.fruitcake.plus.com/Sinclair/ZX80/FlickerFree/ZX80_Kong.htm)
+ [Pacman](http://www.fruitcake.plus.com/Sinclair/ZX80/FlickerFree/ZX80_Pacman.htm)
## Programs with limitations or artefacts
+ [QS Defenda](http://www.zx81stuff.org.uk/zx81/tape/QSDefenda)
  + This game generates 31 lines of text. In 640 by 480 mode the emulator only displays 30 line of text. Set `Centre` to `off` to display the top lines (which includes the score). The game is still playable without the bottom line being visible. The full display is visible in 720x576 mode (i.e.`FixSevenSix` set to `On`). The QS sound board is emulated correctly
+ [Wa-Tor](http://www.pictureviewerpro.com/hosting/zx81/download/zx81/fred/wator.zip)
  + This has a pseudo hi-res introduction screen. Towards the bottom of the screen is the text: "Experiment with predators and prey". On a "real" ZX81 the text is distorted, due to a variation in the time of the horizontal sync pulse. This emulator does not show the distortion
+ [rezurrection](https://bodo4all.fortunecity.ws/zx/rezurrection.html)
  + With default settings, the logo on the final screen "flashes" after the scrolling is complete. This is because the logo is displayed interlaced, at roughly 52Hz. Set `FrameSync` to `Interlaced` to display the final screen correctly without flashes.
  A frame rate adjusted version of the final Rezurrection screen exists in the [Demos](examples/ZX81/Demos) example directory: [head.p](examples/ZX81/Demos/heap.p) and [config.ini](examples/ZX81/Demos/config.ini). When run with `FiveSevenSix` and `FrameSync`  set to `on` or `interlaced` a stable interlaced image can be seen after the scrolling is complete
# Building
**Notes:**
+ Prebuilt executable files for the 5 supported board types can be found [here](uf2/)
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
    `cmake ..`  
    `make`
5. To build for other boards, pass the board type as part of the cmake command. e.g.

| Board | CMake | uf2 name |
| --- | --- | --- |
| Pimoroni DVI |`cmake -DPICO_BOARD=dviboard ..` | `picozx81_dvi.uf2`|
| PicoMite VGA |`cmake -DPICO_BOARD=picomitevgaboard ..` | `picozx81_picomitevga.uf2`|
| Olimex PICO DVI |`cmake -DPICO_BOARD=olimexpcboard ..` | `picozx81_olimexpc.uf2`|
| Pimoroni VGA |`cmake -DPICO_BOARD=vgaboard ..` | `picozx81_vga.uf2`|
| Custom 332 VGA|`cmake -DPICO_BOARD=vga332board ..`| `picozx81_vga332.uf2`|

**Note:** The [`buildall`](buildall) script in the root directory of `picozx81` will build `uf2` files for all supported board types

6. Upload the `uf2` file to the pico
7. Populate a micro SD Card with files you wish to run. Optionally add `config.ini` files to the SD Card. See [here](examples) for examples of config files

# Extra Information
+ The intention of the emulator is to provide an authentic '80s feel. There have been amazing ZX81 developments in recent years, such as ([Chroma 81](http://www.fruitcake.plus.com/Sinclair/ZX81/Chroma/ChromaInterface.htm), [ZXpand+](https://www.rwapsoftware.co.uk/zx812.html) etc). These are not supported. Instead it emulates the hardware that was advertised in the early '80s i.e. QS UDG, Sound, joystick, hi-res mono graphics. However, basic support to load and save memory blocks, using a syntax similar to ZXpand is implemented
+ The ["Big Bang"](https://www.sinclairzxworld.com/viewtopic.php?t=2986) ROM is supported, as this accelerates BASIC execution, and runs on the original ZX81 hardware
+ Program debug support is limited to that provided by the ZX81 "in period", i.e. non-existent. It is recommended that one of the PC or Linux based ZX81 emulators with single step and breakpoint support are used to debug Z80 assembly programs
+ To achieve a full speed emulation the Pico is overclocked to 250MHz (640x480) and 270MHz (720x576). There is a very slight risk that this may damage the Pico. However many other applications run the Pico at this frequency. By default the stock voltage is used (1.1V), this has been successfully tested on multiple Picos. If the emulator appears unstable it can be built to use 1.2V, add `-DOVER_CLOCK` to the cmake command
+ The Pico only has 1 USB port. The Pimoroni and Olimex boards can be powered through a second on board USB power connector, allowing a keyboard to be connected to the Pico using an OTG adaptor
+ To connect more than one peripheral (e.g. a keyboard and joystick) at the same time, a powered USB OTG hub is required. These 3 hubs have been successfully tested. [1](https://www.amazon.co.uk/dp/B083WML1XB), [2](https://www.amazon.co.uk/dp/B078M3Z84Z), [3](https://www.amazon.co.uk/dp/B07Z4RHJ2D). Plug the hub directly into the USB port on the Pico, not the USB power connector on the Pimoroni board  
**Note:** Testing has shown that all of these hubs can support OTG and power delivery to the Pico simultaneously
+ On rare occasion, some USB keyboards and joysticks fail to be detected when connected via powered hubs. A re-boot of the Pico often results in successful detection
+ The PicoMite VGA board has a PS/2 keyboard socket. Currently this is not supported, a USB keyboard must be used
+ The Olimex RP2040-PICO-PC has a stereo audio jack. ~~but the left channel cannot be used with HDMI, so only mono audio through the right channel is possible when using this board~~ The emulator now supports stereo sound on this board
+ The Olimex RP2040-PICO-PC board does not supply 5v to DVI pin 18. This may result in the board not being detected by some TVs. If necessary short the SJ1 connector so 5V is supplied
+ In an ideal world the latest versions of the excellent sz81 or EightyOne emulators would have been ported. An initial port showed that they are too processor intensive for an (overclocked) ARM M0+. An earlier version of sz81 ([2.1.8](https://github.com/ikjordan/sz81_2_1_8)) was used as a basis, with some Z80 timing corrections and back porting of the 207 tstate counter code from the latest sz81 (2.3.12). See [here](#applications-tested) for a list of applications tested

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
## Use with an original ZX80/ZX81 keyboard
There are not enough unused GPIO pins on the Pimoroni demo boards to allow the direct connection of a ZX80/81 keyboard, but it can be done by using another Pico to convert the ZX80/81 keyboard into a USB keyboard. It may seem excessive to use a whole pico as a keyboard controller, but they are cheap and there is enough space to put the Pimoroni board, plus another Pico, plus a small USB hub into a ZX80 or ZX81 case

Code to convert a ZX8x keyboard to USB can be found at [ZX81_USB_KBD](https://github.com/ikjordan/ZX81_USB_KBD). This code has been used to successfully connect a ZX81 keyboard to this emulator. If the keyboard is the only peripheral, then it can be plugged straight into the USB port of the Pico on the emulator board with the power connected to the USB power socket of the Pimoroni board. If other USB peripherals (such as another keyboard, or a USB joystick) also need to be connected then the ZX80/81 keyboard can be connected via a USB hub

To access the function menus from a ZX80/81 keyboard the `doubleshift` configuration option must be enabled
## Performance and constraints
The initial port from sz81 2.3.12 onto the Pico ran at approximately 10% of real time speed. Use of the Z80 emulator originally written for xz80 by Ian Collier, plus optimisation of the ZX81 memory access, display and plot routines allows the emulator to run at 100% of real time speed. The display of a full 320 by 240 image in real time (e.g. [MaxDemo](https://bodo4all.fortunecity.ws/zx/maxdemo.html)) uses approximately 90% of the available CPU clock cycles. An overclock to 250MHz is required

Corrections to the tstate timings were made for `ld a,n; ld c,n; ld e,n; ld l,n; set n,(hl); res n,(hl);`
## Possible Future Developments
+ Add vsync (TV) based sound
+ There are some interesting developments to extend PicoDVI to include audio over the HDMI signal
+ Use the contents of a string variable to supply a file name to the ZX80 load and save commands
+ Support for USB gamepads as well as joysticks
+ Extend the VGA322 board type to support original DB9 joysticks
+ Support for composite video output
+ Move to a Pi Zero to greatly increase processing power and use [circle](https://github.com/rsta2/circle) for fast boot times
## Comparison to MCUME
[MCUME](https://github.com/Jean-MarcHarvengt/MCUME/) demonstrated that a Raspberry Pi Pico based ZX80/81 emulator was feasible. The custom VGA RGB 332 board type is similar to the hardware required for MCUME

This emulator offers the following over MCUME:
+ Support for USB keyboards and joysticks
+ Emulation runs at full speed of a 3.25MHz ZX81
+ Ability to save files
+ Ability to load a program without reset
+ Support for Hi-res and pseudo Hi-res graphics
+ Support for multiple DVI and VGA boards
+ Support for programs which use more than 32 columns or 24 rows of characters
+ ZonX and QS Sound emulation
+ Emulated QS UDG
+ 50Hz and 60Hz emulation
+ Emulator display refresh decoupled from Pico display rate
+ Mapping of joystick inputs to specific key presses
+ Hardware configuration associated with specific program files
+ Builds under Linux (MCUME ZX80/81 emulator fails to build in a case sensitive file system)


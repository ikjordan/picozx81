set(PROJECT picozx81)
cmake_minimum_required(VERSION 3.13)

# Determine the MCU type and configure accordingly
set(PICO_MCU "rp2040" CACHE STRING "Specify Pico MCU type")

if ((${PICO_MCU} STREQUAL "rp2350"))
    set(PICO_PLATFORM "rp2350-arm-s")
else()
    set(PICO_PLATFORM "rp2040")
endif()

# Look for board file in local board directory
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/boards)
if (NOT PICO_BOARD)
    set(PICO_BOARD "vgaboard")
endif()

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(${PROJECT} C CXX ASM)

OPTION(TIME_SPARE "Set to true to measure idle time" OFF)
OPTION(FLASH_LED "Set to blink board led and measure idle time" OFF)
OPTION(OVER_VOLT "Set to true to increase the Pico voltage" OFF)
OPTION(HDMI_SOUND "Set to true to deliver sound over hdmi" OFF)
OPTION(PICOZX_LCD "Set to true to enable LCD for PICOZX" OFF)

# Set to "dviboard" to build for Pimoroni dvi board
# e.g. cmake -DPICO_BOARD=dviboard
# Set to "vga332board" for vga 332 board similar to that used in MCUME
# e.g. cmake -DPICO_BOARD=vga332board
# Set to "picomitevgaboard" for PicoMite vga board
# e.g. cmake -DPICO_BOARD=picomitevgaboard
# Set to "olimexpcboard" for Olimex RP2040-PICO-PC board
# e.g. cmake -DPICO_BOARD=olimexpcboard
# Set to "lcdws28board" for Waveshare Pico-ResTouch-LCD-2.8
# e.g. cmake -DPICO_BOARD=lcdws28board
# Set to "wspizeroboard" for Waveshare PiZero
# e.g. cmake -DPICO_BOARD=wspizeroboard
# Set to "lcdmakerboard" for LCD connected to Cytron Maker board
# e.g. cmake -DPICO_BOARD=lcdmakerboard
# Set to "vgamaker222cboard" for vga 222 with csync based on Cytron Maker board
# e.g. cmake -DPICO_BOARD=vgamaker222cboard
# Set to "picozxboard" for vga 222 with csync based on picozx board
# e.g. cmake -DPICO_BOARD=picozxboard
# Set to "picozxrealboard" for vga 222 with csync based on picozxreal board
# e.g. cmake -DPICO_BOARD=picozxrealboard
#
# To enable HDMI sound add -DHDMI_SOUND
# To enable LCD display in addition to VGA for picozx add -DPICOZX_LCD

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.0.0")
    message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.0.0 (or later) required. Current version is ${PICO_SDK_VERSION_STRING}")
endif()

# Initialize the Pico SDK
pico_sdk_init()

add_subdirectory(fatfs)
add_subdirectory(sdcard)

include_directories(fatfs)
include_directories(sdcard)

include_directories(config)
include_directories(display)
include_directories(usb)
include_directories(inih)

if ((${PICO_BOARD} STREQUAL "dviboard") OR (${PICO_BOARD} STREQUAL "olimexpcboard") OR (${PICO_BOARD} STREQUAL "wspizeroboard"))
    add_subdirectory(PicoDVI/software/libdvi)
    include_directories(PicoDVI/software/libdvi)
    include_directories(PicoDVI/software/include)
elseif ((${PICO_BOARD} STREQUAL "vgamaker222cboard") OR (${PICO_BOARD} STREQUAL "picozxboard") OR (${PICO_BOARD} STREQUAL "picozxrealboard"))
    # Build local scanvideo, to use CSYNC
    add_subdirectory(scanvideo)
    include_directories(scanvideo/pico_scanvideo/include)
    include_directories(scanvideo/pico_scanvideo_dpi/include)
endif()

set(ZX8X_SOURCES
    src/z80.c
    src/zx8x.c
    src/emuapi.cpp
    src/emusound.cpp
    src/emuvideo.cpp
    src/emukeyboard.cpp
    src/loadp.c
    src/pico81.cpp
    src/hid_usb.c
    src/sound.c
    src/menu.cpp)

set(INI_SOURCES
    inih/ini.c)

set(DISPLAY_COMMON_SOURCES
    display/display_common.c)

if ((${PICO_BOARD} STREQUAL "dviboard") OR (${PICO_BOARD} STREQUAL "olimexpcboard") OR (${PICO_BOARD} STREQUAL "wspizeroboard"))
    set(DISPLAY_SOURCES
        display/display_dvi.c
        display/tmds_double.S
        display/tmds_chroma.S)
elseif ((${PICO_BOARD} STREQUAL "lcdws28board") OR (${PICO_BOARD} STREQUAL "lcdmakerboard"))
    set(DISPLAY_SOURCES
        display/display_lcd.c)
elseif ((${PICO_BOARD} STREQUAL "picozxboard") AND (${PICOZX_LCD}))
    set(DISPLAY_SOURCES
        display/display_lcd.c
        display/display_vga.c)
else ()
    set(DISPLAY_SOURCES
        display/display_vga.c)
endif ()

set(USB_SOURCES
    usb/hid_app.c
    usb/hid_host_info.c
    usb/hid_host_joy.c
    usb/hid_host_utils.c
    usb/hid_ri.c
    usb/hid_rip.c)

add_executable(${PROJECT}
               ${ZX8X_SOURCES}
               ${INI_SOURCES}
               ${DISPLAY_COMMON_SOURCES}
               ${DISPLAY_SOURCES}
               ${USB_SOURCES})

# Determine sound system
if ((((${PICO_BOARD} STREQUAL "dviboard") OR (${PICO_BOARD} STREQUAL "olimexpcboard")) AND ${HDMI_SOUND}) OR (${PICO_BOARD} STREQUAL "wspizeroboard"))
    # HDMI - The check also ensures that HDMI_SOUND is ignored if set for non HDMI boards
    target_compile_definitions(${PROJECT} PRIVATE -DSOUND_HDMI)
else()
    if ((${PICO_BOARD} STREQUAL "dviboard") OR (${PICO_BOARD} STREQUAL "vgaboard"))
        # I2S loaded by DMA sound
        target_compile_definitions(${PROJECT} PRIVATE -DI2S)
        # Default to I2S sound. Note that Pimoroni VGA board can also use PWM
        # but values cannot be loaded by DMA, as l and r are on different slices
        target_link_libraries(${PROJECT}
                            hardware_pio
                            hardware_dma)
        pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/src/audio_i2s.pio)
    elseif (NOT (${PICO_BOARD} STREQUAL "olimexpcboard"))
        # DMA driven PWM unless Olimex (which has channels on different slices)
        target_compile_definitions(${PROJECT} PRIVATE -DSOUND_DMA)
        target_link_libraries(${PROJECT} hardware_dma)
        target_link_libraries(${PROJECT} hardware_pwm)
    else()
        # Pure PWM
        target_link_libraries(${PROJECT} hardware_pwm)
    endif()
endif()

# Determine video system and name uf2
set (EXTRA_CHECK "-Wextra")
if ((${PICO_BOARD} STREQUAL "dviboard") OR (${PICO_BOARD} STREQUAL "olimexpcboard") OR (${PICO_BOARD} STREQUAL "wspizeroboard"))
    target_compile_definitions(${PROJECT} PUBLIC
                                -DDVI_1BPP_BIT_REVERSE=1
                                -DDVI_VERTICAL_REPEAT=2)
    target_link_libraries(${PROJECT} libdvi)

    if ((${PICO_BOARD} STREQUAL "wspizeroboard"))
        Set(NAME_ROOT "picozx81_wspizero_hdmi_sound")
        target_compile_definitions(${PROJECT} PUBLIC
                                -DDVI_DEFAULT_SERIAL_CONFIG=waveshare_rp2040_pizero_hdmi_cfg)
    else()
        if (${PICO_BOARD} STREQUAL "dviboard")
            if (${HDMI_SOUND})
                Set(NAME_ROOT "picozx81_dvi_hdmi_sound")
            else()
                Set(NAME_ROOT "picozx81_dvi")
            endif()
            target_compile_definitions(${PROJECT} PUBLIC
                                    -DDVI_DEFAULT_SERIAL_CONFIG=pimoroni_demo_hdmi_cfg)
        else()
            target_compile_definitions(${PROJECT} PUBLIC
                                    -DDVI_DEFAULT_SERIAL_CONFIG=Olimex_RP2040_PICO_PC_cfg
                                    -DDVI_USE_PIO_CLOCK)
            if(${HDMI_SOUND})
                set(NAME_ROOT "picozx81_olimexpc_hdmi_sound")
            else()
            set(NAME_ROOT "picozx81_olimexpc")
            endif()
        endif()
    endif()
elseif ((${PICO_BOARD} STREQUAL "lcdws28board"))
    pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/display/spi_lcd.pio)
    set(NAME_ROOT "picozx81_lcdws28")
elseif ((${PICO_BOARD} STREQUAL "lcdmakerboard"))
    pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/display/spi_lcd.pio)
    set(NAME_ROOT "picozx81_lcdmaker")
else()
    if (${PICO_BOARD} STREQUAL "vgaboard")
        set(NAME_ROOT "picozx81_vga")
    elseif (${PICO_BOARD} STREQUAL "picomitevgaboard")
        set(NAME_ROOT "picozx81_picomitevga")
    elseif (${PICO_BOARD} STREQUAL "vga332board")
        set(NAME_ROOT "picozx81_vga332")
    elseif (${PICO_BOARD} STREQUAL "vgamaker222cboard")
        set(NAME_ROOT "picozx81_vgamaker222c")
    elseif (${PICO_BOARD} STREQUAL "picozxboard")
        if (${PICOZX_LCD})
            pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/display/spi_lcd.pio)
            target_compile_definitions(${PROJECT} PRIVATE -DPICOZX_LCD)
            set(NAME_ROOT "picozx81_picozx_lcd")
        else ()
            set(NAME_ROOT "picozx81_picozx")
        endif ()
    elseif (${PICO_BOARD} STREQUAL "picozxrealboard")
        set(NAME_ROOT "picozx81_picozxreal")
    endif()

    if ((${PICO_BOARD} STREQUAL "vgamaker222cboard") OR (${PICO_BOARD} STREQUAL "picozxboard") OR (${PICO_BOARD} STREQUAL "picozxrealboard"))
        target_link_libraries(${PROJECT} mod_scanvideo_dpi)
    else ()
        target_link_libraries(${PROJECT} pico_scanvideo_dpi)
        set (EXTRA_CHECK "")
    endif()
endif()

string(CONCAT FULL_NAME  ${NAME_ROOT} "_" ${PICO_MCU})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${FULL_NAME})

if ((${PICO_MCU} STREQUAL "rp2350"))
    target_compile_definitions(${PROJECT} PUBLIC -DPICO_2350)
endif()

if (${FLASH_LED})
    target_compile_definitions(${PROJECT} PRIVATE -DFLASH_LED -DTIME_SPARE)
elseif (${TIME_SPARE})
    target_compile_definitions(${PROJECT} PRIVATE -DTIME_SPARE)
endif()

if (${OVER_VOLT})
    target_compile_definitions(${PROJECT} PRIVATE -DOVER_VOLT)
endif()

# always support Chroma
target_compile_definitions(${PROJECT} PRIVATE -DSUPPORT_CHROMA -DLOAD_AND_SAVE)

target_compile_options(${PROJECT} PRIVATE -Wall ${EXTRA_CHECK})

target_link_libraries(${PROJECT}
                      pico_multicore
                      pico_stdlib
                      sdcard
                      fatfs
                      tinyusb_host
                      tinyusb_board)

# Cannot use serial port with Pimoroni vga or picozx boards
if ((${PICO_BOARD} STREQUAL "vgaboard") OR (${PICO_BOARD} STREQUAL "picozxboard") OR (${PICO_BOARD} STREQUAL "picozxrealboard"))
    pico_enable_stdio_uart(${PROJECT} 0)
else()
    pico_enable_stdio_uart(${PROJECT} 1)
endif()
pico_enable_stdio_usb(${PROJECT} 0)

pico_add_extra_outputs(${PROJECT})

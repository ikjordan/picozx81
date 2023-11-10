set(PROJECT picozx81)
cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
include($ENV{PICO_EXTRAS_PATH}/external/pico_extras_import.cmake)

project(${PROJECT} C CXX)

OPTION(TIME_SPARE "Set to true to measure idle time" OFF)
OPTION(OVER_VOLT "Set to true to increase the Pico voltage" OFF)

# Look for board file first in local board directory
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/boards)
if (NOT PICO_BOARD)
    set(PICO_BOARD "vgaboard")
endif()

# Set to "dviboard" to build for Pimoroni dvi board
# e.g. cmake -DPICO_BOARD=dviboard
# Set to "vga332board" for home brew 332 vga board
# e.g. cmake -DPICO_BOARD=vga332board
# Set to "picomitevgaboard" for PicoMite vga board
# e.g. cmake -DPICO_BOARD=picomitevgaboard
# Set to "olimexpcboard" for Olimex RP2040-PICO-PC board
# e.g. cmake -DPICO_BOARD=olimexpcboard

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

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

if ((${PICO_BOARD} STREQUAL "dviboard") OR (${PICO_BOARD} STREQUAL "olimexpcboard"))
    add_subdirectory(PicoDVI/software/libdvi)
    include_directories(PicoDVI/software/libdvi)
    include_directories(PicoDVI/software/include)
endif()

set(ZX8X_SOURCES
    src/Z80.c
    src/zx8x.c
    src/emuapi.cpp
    src/emusound.cpp
    src/emuvideo.cpp
    src/pico81.cpp
    src/hid_usb.c
    src/sound.c
    src/menu.cpp)

set(INI_SOURCES
    inih/ini.c)

if ((${PICO_BOARD} STREQUAL "dviboard") OR (${PICO_BOARD} STREQUAL "olimexpcboard"))
    set(DISPLAY_SOURCES
        display/display_dvi.c
        display/tmds_double.S
        display/tmds_double.h)
elseif ((${PICO_BOARD} STREQUAL "lcdws28board"))
    set(DISPLAY_SOURCES
        display/display_lcd.c)
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
               ${DISPLAY_SOURCES}
               ${USB_SOURCES})

# Determine sound system
if ((${PICO_BOARD} STREQUAL "dviboard") OR (${PICO_BOARD} STREQUAL "vgaboard"))
    # Use I2S loaded by DMA sound
    target_compile_definitions(${PROJECT} PRIVATE -DI2S)
    # Default to I2S sound. Note that Pimoroni VGA board can also use PWM
    # but values cannot be loaded by DMA, as l and r are on different slices
    target_link_libraries(${PROJECT}
                          hardware_pio
                          hardware_dma)
    pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/src/audio_i2s.pio)
elseif (NOT (${PICO_BOARD} STREQUAL "olimexpcboard"))
    # Unless Olimex (which has channels on different slices) use PWM loaded by DMA sound
    target_compile_definitions(${PROJECT} PRIVATE -DSOUND_DMA)
    target_link_libraries(${PROJECT} hardware_dma)
endif()

# Determine video system and name uf2
if ((${PICO_BOARD} STREQUAL "dviboard") OR (${PICO_BOARD} STREQUAL "olimexpcboard"))
    target_compile_definitions(${PROJECT} PUBLIC
                                -DDVI_1BPP_BIT_REVERSE=1
                                -DDVI_VERTICAL_REPEAT=2
                                -DDVI_N_TMDS_BUFFERS=3
                                -DDVI_MONOCHROME_TMDS)
    target_link_libraries(${PROJECT} libdvi)

    if (${PICO_BOARD} STREQUAL "dviboard")
        set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "picozx81_dvi")
        target_compile_definitions(${PROJECT} PUBLIC
                                   -DDVI_DEFAULT_SERIAL_CONFIG=pimoroni_demo_hdmi_cfg)
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "picozx81_olimexpc")
        target_compile_definitions(${PROJECT} PUBLIC
                                   -DDVI_DEFAULT_SERIAL_CONFIG=Olimex_RP2040_PICO_PC_cfg
                                   -DDVI_USE_PIO_CLOCK)
    endif()
elseif ((${PICO_BOARD} STREQUAL "lcdws28board"))
    pico_generate_pio_header(${PROJECT} ${CMAKE_CURRENT_LIST_DIR}/display/st7789_lcd.pio)
    set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "picozx81_lcdws28")
else()
    if (${PICO_BOARD} STREQUAL "vgaboard")
        set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "picozx81_vga")
    elseif (${PICO_BOARD} STREQUAL "picomitevgaboard")
        set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "picozx81_picomitevga")
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "picozx81_vga332")
    endif()
    target_link_libraries(${PROJECT} pico_scanvideo_dpi)
endif()

if (${TIME_SPARE})
target_compile_definitions(${PROJECT} PRIVATE -DTIME_SPARE)
endif()

if (${OVER_VOLT})
target_compile_definitions(${PROJECT} PRIVATE -DOVER_VOLT)
endif()

target_compile_options(${PROJECT} PRIVATE -Wall)

target_link_libraries(${PROJECT}
                      pico_multicore
                      pico_stdlib
                      sdcard
                      fatfs
                      hardware_pwm
                      tinyusb_host
                      tinyusb_board)

# Cannot use serial port with Pimoroni vga card
if (${PICO_BOARD} STREQUAL "vgaboard")
    pico_enable_stdio_uart(${PROJECT} 0)
else()
    pico_enable_stdio_uart(${PROJECT} 1)
endif()
pico_enable_stdio_usb(${PROJECT} 0)

pico_add_extra_outputs(${PROJECT})


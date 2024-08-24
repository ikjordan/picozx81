// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_PICOMITEVGABOARD_H
#define _BOARDS_PICOMITEVGABOARD_H

// For board detection
#define RASPBERRYPI_PICOMITEVGABOARD


#define PICOMITEVGABOARD_VGA_COLOR_PIN_BASE 18
#define PICOMITEVGABOARD_VGA_SYNC_PIN_BASE 16
#define PICOMITEVGABOARD_PWM_L_PIN 6
#define PICOMITEVGABOARD_PWM_R_PIN 7


#ifndef PICO_SCANVIDEO_COLOR_PIN_BASE
#define PICO_SCANVIDEO_COLOR_PIN_BASE PICOMITEVGABOARD_VGA_COLOR_PIN_BASE
#endif

#ifndef PICO_SCANVIDEO_SYNC_PIN_BASE
#define PICO_SCANVIDEO_SYNC_PIN_BASE PICOMITEVGABOARD_VGA_SYNC_PIN_BASE
#endif

#define PICO_SCANVIDEO_ENABLE_CLOCK_PIN 0
#define PICO_SCANVIDEO_COLOR_PIN_COUNT 4
#define PICO_SCANVIDEO_PIXEL_RSHIFT 3
#define PICO_SCANVIDEO_PIXEL_GSHIFT 1
#define PICO_SCANVIDEO_PIXEL_BSHIFT 0
#define PICO_SCANVIDEO_PIXEL_RCOUNT 1
#define PICO_SCANVIDEO_PIXEL_GCOUNT 2
#define PICO_SCANVIDEO_PIXEL_BCOUNT 1

#ifndef PICO_AUDIO_PWM_L_PIN
#define PICO_AUDIO_PWM_L_PIN PICOMITEVGABOARD_PWM_L_PIN
#endif

#ifndef PICO_AUDIO_PWM_R_PIN
#define PICO_AUDIO_PWM_R_PIN PICOMITEVGABOARD_PWM_R_PIN
#endif

#define PICOMITEVGABOARD_SD_CLK_PIN  10
#define PICOMITEVGABOARD_SD_CMD_PIN  11 // MOSI
#define PICOMITEVGABOARD_SD_DAT0_PIN 12 // MISO

#ifndef PICO_SD_CLK_PIN
#define PICO_SD_CLK_PIN PICOMITEVGABOARD_SD_CLK_PIN
#endif

#ifndef PICO_SD_CMD_PIN
#define PICO_SD_CMD_PIN PICOMITEVGABOARD_SD_CMD_PIN
#endif

#ifndef PICO_SD_DAT0_PIN
#define PICO_SD_DAT0_PIN PICOMITEVGABOARD_SD_DAT0_PIN
#endif

#ifndef PICO_SD_DAT_PIN_COUNT
#define PICO_SD_DAT_PIN_COUNT 2
#endif

// 1 or -1
#ifndef PICO_SD_DAT_PIN_INCREMENT
#define PICO_SD_DAT_PIN_INCREMENT 1
#endif
#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif

#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif

#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

//#define NINEPIN_JOYSTICK

#define NINEPIN_UP      3
#define NINEPIN_DOWN    4
#define NINEPIN_LEFT    5
#define NINEPIN_RIGHT   22
#define NINEPIN_BUTTON  26

#define PICO_PICOMITEVGA_BOARD

// default anything we haven't set above
#include "mcu.h"

#endif

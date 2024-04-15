// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_PICOZXBOARD_H
#define _BOARDS_PICOZXBOARD_H

// For board detection
#define RASPBERRYPI_PICOZXBOARD

#define PICOZXBOARD_VGA_COLOR_PIN_BASE 0
#define PICOZXBOARD_VGA_SYNC_PIN_BASE 6

#define PICOZXBOARD_PWM_L_PIN  7
#define PICOZXBOARD_PWM_R_PIN  7

#define PICOZXBOARD_SD_CLK_PIN  10
#define PICOZXBOARD_SD_CMD_PIN  11    // MOSI
#define PICOZXBOARD_SD_DAT0_PIN 12    // MISO
#define PICOZXBOARD_SD_CS_PIN   13


#ifndef PICO_SCANVIDEO_COLOR_PIN_BASE
#define PICO_SCANVIDEO_COLOR_PIN_BASE PICOZXBOARD_VGA_COLOR_PIN_BASE
#endif

#ifndef PICO_SCANVIDEO_SYNC_PIN_BASE
#define PICO_SCANVIDEO_SYNC_PIN_BASE PICOZXBOARD_VGA_SYNC_PIN_BASE
#endif

#define PICO_SCANVIDEO_ENABLE_CLOCK_PIN 0
#define PICO_SCANVIDEO_COLOR_PIN_COUNT 6
#define PICO_SCANVIDEO_PIXEL_RSHIFT 4
#define PICO_SCANVIDEO_PIXEL_GSHIFT 2
#define PICO_SCANVIDEO_PIXEL_BSHIFT 0
#define PICO_SCANVIDEO_PIXEL_RCOUNT 2
#define PICO_SCANVIDEO_PIXEL_GCOUNT 2
#define PICO_SCANVIDEO_PIXEL_BCOUNT 2

#ifndef PICO_AUDIO_PWM_L_PIN
#define PICO_AUDIO_PWM_L_PIN PICOZXBOARD_PWM_L_PIN
#endif

#ifndef PICO_AUDIO_PWM_R_PIN
#define PICO_AUDIO_PWM_R_PIN PICOZXBOARD_PWM_R_PIN
#endif

#ifndef PICO_SD_CLK_PIN
#define PICO_SD_CLK_PIN PICOZXBOARD_SD_CLK_PIN
#endif

#ifndef PICO_SD_CMD_PIN
#define PICO_SD_CMD_PIN PICOZXBOARD_SD_CMD_PIN
#endif

#ifndef PICO_SD_DAT0_PIN
#define PICO_SD_DAT0_PIN PICOZXBOARD_SD_DAT0_PIN
#endif

#ifndef PICO_SD_CS_PIN
#define PICO_SD_CS_PIN PICOZXBOARD_SD_CS_PIN
#endif

#define CP 19, 20, 21, 22, 26, 27, 28
#define RP 8, 9, 14, 15, 16, 17, 18

#define RN 7
#define CN 7
#define CP_SHIFT 19
#define CP_JOIN(a) ((a & 0xF) | ((a >> 3) & (0x7 << 4)))

#define PICO_PICOZX_BOARD

// Maker board has a Pico on it, so default anything we haven't set above
#include "boards/pico.h"

#endif

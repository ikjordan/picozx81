#ifndef _BOARDS_LCDWS28BOARD_H
#define _BOARDS_LCDWS28BOARD_H

// For board detection
#define RASPBERRYPI_LCDWS28BOARD

#define LCDWS28BOARD_SD_CLK_PIN  10
#define LCDWS28BOARD_SD_CMD_PIN  11 // MOSI
#define LCDWS28BOARD_SD_DAT0_PIN 12 // MISO

#define LCDWS28BOARD_LCD_DC_PIN  8
#define LCDWS28BOARD_LCD_CS_PIN  9
#define LCDWS28BOARD_LCD_BL      13
#define LCDWS28BOARD_LCD_RS      15

#define LCDWS28BOARD_SD_CS_PIN   22
#define LCDWS28BOARD_TS_CS_PIN   16

#define LCDWS28BOARD_PWM_L_PIN   28

#ifndef PICO_AUDIO_PWM_L_PIN
#define PICO_AUDIO_PWM_L_PIN LCDWS28BOARD_PWM_L_PIN
#endif

#ifndef PICO_SD_CLK_PIN
#define PICO_SD_CLK_PIN LCDWS28BOARD_SD_CLK_PIN
#endif

#ifndef PICO_SD_CMD_PIN
#define PICO_SD_CMD_PIN LCDWS28BOARD_SD_CMD_PIN
#endif

#ifndef PICO_SD_DAT0_PIN
#define PICO_SD_DAT0_PIN LCDWS28BOARD_SD_DAT0_PIN
#endif

#ifndef PICO_SD_CS_PIN
#define PICO_SD_CS_PIN LCDWS28BOARD_SD_CS_PIN
#endif

#ifndef PICO_LCD_DC_PIN
#define PICO_LCD_DC_PIN LCDWS28BOARD_LCD_DC_PIN
#endif

#ifndef PICO_LCD_CS_PIN
#define PICO_LCD_CS_PIN LCDWS28BOARD_LCD_CS_PIN
#endif

#ifndef PICO_LCD_BL_PIN
#define PICO_LCD_BL_PIN LCDWS28BOARD_LCD_BL
#endif

#ifndef PICO_LCD_RS_PIN
#define PICO_LCD_RS_PIN LCDWS28BOARD_LCD_RS
#endif

#ifndef PICO_TS_TS_PIN
#define PICO_TS_CS_PIN LCDWS28BOARD_TS_CS_PIN
#endif

// No sound output
#define PICO_NO_SOUND

// Share SPI bus with SD Card
#define PICO_SPI_LCD_SD_SHARE

#define PICO_LCDWS28_BOARD

// default anything we haven't set above
#include "mcu.h"

#endif

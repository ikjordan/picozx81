#ifndef _BOARDS_LCDMAKERBOARD_H
#define _BOARDS_LCDMAKERBOARD_H

// For board detection
#define RASPBERRYPI_LCDMAKERBOARD

#define LCDMAKERBOARD_SD_CLK_PIN  10
#define LCDMAKERBOARD_SD_CMD_PIN  11    // MOSI
#define LCDMAKERBOARD_SD_DAT0_PIN 12    // MISO
#define LCDMAKERBOARD_SD_CS_PIN   15

#define LCDMAKERBOARD_LCD_DC_PIN  9
#define LCDMAKERBOARD_LCD_CS_PIN  5
#define LCDMAKERBOARD_LCD_CMD_PIN 7     // MOSI
#define LCDMAKERBOARD_LCD_BL_PIN  4
#define LCDMAKERBOARD_LCD_RS_PIN  8
#define LCDMAKERBOARD_LCD_CLK_PIN 6

#define LCDMAKERBOARD_PWM_L_PIN  18
#define LCDMAKERBOARD_PWM_R_PIN  19

#ifndef PICO_AUDIO_PWM_L_PIN
#define PICO_AUDIO_PWM_L_PIN LCDMAKERBOARD_PWM_L_PIN
#endif

#ifndef PICO_AUDIO_PWM_R_PIN
#define PICO_AUDIO_PWM_R_PIN LCDMAKERBOARD_PWM_R_PIN
#endif

#ifndef PICO_SD_CLK_PIN
#define PICO_SD_CLK_PIN LCDMAKERBOARD_SD_CLK_PIN
#endif

#ifndef PICO_SD_CMD_PIN
#define PICO_SD_CMD_PIN LCDMAKERBOARD_SD_CMD_PIN
#endif

#ifndef PICO_SD_DAT0_PIN
#define PICO_SD_DAT0_PIN LCDMAKERBOARD_SD_DAT0_PIN
#endif

#ifndef PICO_SD_CS_PIN
#define PICO_SD_CS_PIN LCDMAKERBOARD_SD_CS_PIN
#endif

#ifndef PICO_LCD_DC_PIN
#define PICO_LCD_DC_PIN LCDMAKERBOARD_LCD_DC_PIN
#endif

#ifndef PICO_LCD_CS_PIN
#define PICO_LCD_CS_PIN LCDMAKERBOARD_LCD_CS_PIN
#endif

#ifndef PICO_LCD_BL_PIN
#define PICO_LCD_BL_PIN LCDMAKERBOARD_LCD_BL_PIN
#endif

#ifndef PICO_LCD_RS_PIN
#define PICO_LCD_RS_PIN LCDMAKERBOARD_LCD_RS_PIN
#endif

#ifndef PICO_LCD_CMD_PIN
#define PICO_LCD_CMD_PIN LCDMAKERBOARD_LCD_CMD_PIN
#endif

#ifndef PICO_LCD_CLK_PIN
#define PICO_LCD_CLK_PIN LCDMAKERBOARD_LCD_CLK_PIN
#endif

#define PICO_LCDMAKER_BOARD

// Cytron maker board has a Pico on it, so default anything we haven't set above
#include "boards/pico.h"

#endif

/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_DVIBOARD_H
#define _BOARDS_DVIBOARD_H

// For board detection
#define RASPBERRYPI_DVIBOARD

// Audio pins. I2S BCK, LRCK are on the same pins as PWM L/R.
// - When outputting I2S, PWM sees BCK and LRCK, which should sound silent as
//   they are constant duty cycle, and above the filter cutoff
#define DVIBOARD_I2S_DIN_PIN 26
#define DVIBOARD_I2S_BCK_PIN 27
#define DVIBOARD_I2S_LRCK_PIN 28

#define DVIBOARD_SD_CLK_PIN 5
#define DVIBOARD_SD_CMD_PIN 18
#define DVIBOARD_SD_DAT0_PIN 19

#define DVIBOARD_BUTTON_A_PIN 0
#define DVIBOARD_BUTTON_B_PIN 6
#define DVIBOARD_BUTTON_C_PIN 11

#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 0
#endif

#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 0
#endif

#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 1
#endif

#ifndef PICO_SD_CLK_PIN
#define PICO_SD_CLK_PIN DVIBOARD_SD_CLK_PIN
#endif

#ifndef PICO_SD_CMD_PIN
#define PICO_SD_CMD_PIN DVIBOARD_SD_CMD_PIN
#endif

#ifndef PICO_SD_DAT0_PIN
#define PICO_SD_DAT0_PIN DVIBOARD_SD_DAT0_PIN
#endif

// 1 or 4
#ifndef PICO_SD_DAT_PIN_COUNT
#define PICO_SD_DAT_PIN_COUNT 4
#endif

// 1 or -1
#ifndef PICO_SD_DAT_PIN_INCREMENT
#define PICO_SD_DAT_PIN_INCREMENT 1
#endif

#ifndef PICO_AUDIO_I2S_DATA_PIN
#define PICO_AUDIO_I2S_DATA_PIN DVIBOARD_I2S_DIN_PIN
#endif
#ifndef PICO_AUDIO_I2S_CLOCK_PIN_BASE
#define PICO_AUDIO_I2S_CLOCK_PIN_BASE DVIBOARD_I2S_BCK_PIN
#endif

#define PICO_DVI_BOARD

// dviboard has a Pico on it, so default anything we haven't set above
#include "boards/pico.h"

#endif

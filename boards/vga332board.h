/*
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_VGA332BOARD_H
#define _BOARDS_VGA332BOARD_H

// For board detection
#define RASPBERRYPI_VGA332BOARD


#define VGA332BOARD_VGA_COLOR_PIN_BASE 2
#define VGA332BOARD_VGA_SYNC_PIN_BASE 10
#define VGA332BOARD_PWM_L_PIN 28


#ifndef PICO_SCANVIDEO_COLOR_PIN_BASE
#define PICO_SCANVIDEO_COLOR_PIN_BASE VGA332BOARD_VGA_COLOR_PIN_BASE
#endif

#ifndef PICO_SCANVIDEO_SYNC_PIN_BASE
#define PICO_SCANVIDEO_SYNC_PIN_BASE VGA332BOARD_VGA_SYNC_PIN_BASE
#endif

#define PICO_SCANVIDEO_ENABLE_CLOCK_PIN 0
#define PICO_SCANVIDEO_COLOR_PIN_COUNT 8
#define PICO_SCANVIDEO_PIXEL_RSHIFT 5
#define PICO_SCANVIDEO_PIXEL_GSHIFT 2
#define PICO_SCANVIDEO_PIXEL_BSHIFT 0
#define PICO_SCANVIDEO_PIXEL_RCOUNT 3
#define PICO_SCANVIDEO_PIXEL_GCOUNT 3
#define PICO_SCANVIDEO_PIXEL_BCOUNT 2

#ifndef PICO_AUDIO_PWM_L_PIN
#define PICO_AUDIO_PWM_L_PIN VGA332BOARD_PWM_L_PIN
#endif

#define PICO_VGA332_BOARD

// vga332board has a Pico on it, so default anything we haven't set above
#include "boards/pico.h"

#endif

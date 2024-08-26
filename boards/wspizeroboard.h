// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_WSPIZEROBOARD_H
#define _BOARDS_WSPIZEROBOARD_H

// For board detection
#define WAVESHARE_BOARDS_PIZEROBOARD

#define WSPIZEROBOARD_SD_CLK_PIN 18
#define WSPIZEROBOARD_SD_CMD_PIN 19
#define WSPIZEROBOARD_SD_DAT0_PIN 20

#ifndef PICO_DEFAULT_UART
#define PICO_DEFAULT_UART 1
#endif

#ifndef PICO_DEFAULT_UART_TX_PIN
#define PICO_DEFAULT_UART_TX_PIN 4
#endif

#ifndef PICO_DEFAULT_UART_RX_PIN
#define PICO_DEFAULT_UART_RX_PIN 5
#endif

#ifndef PICO_SD_CLK_PIN
#define PICO_SD_CLK_PIN WSPIZEROBOARD_SD_CLK_PIN
#endif

#ifndef PICO_SD_CMD_PIN
#define PICO_SD_CMD_PIN WSPIZEROBOARD_SD_CMD_PIN
#endif

#ifndef PICO_SD_DAT0_PIN
#define PICO_SD_DAT0_PIN WSPIZEROBOARD_SD_DAT0_PIN
#endif

#ifndef PICO_SD_DAT_PIN_COUNT
#define PICO_SD_DAT_PIN_COUNT 2     // SO CS is 21
#endif

// 1 or -1
#ifndef PICO_SD_DAT_PIN_INCREMENT
#define PICO_SD_DAT_PIN_INCREMENT 1
#endif

#define WSPIZEROBOARD_PWM_R_PIN 9   // Not used
#define WSPIZEROBOARD_PWM_L_PIN 9   // Not used

#ifndef PICO_AUDIO_PWM_L_PIN
#define PICO_AUDIO_PWM_L_PIN WSPIZEROBOARD_PWM_L_PIN
#endif

#ifndef PICO_AUDIO_PWM_R_PIN
#define PICO_AUDIO_PWM_R_PIN WSPIZEROBOARD_PWM_R_PIN
#endif

#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (16 * 1024 * 1024)
#endif

#define NINEPIN_JOYSTICK

#define NINEPIN_UP      11
#define NINEPIN_DOWN    12
#define NINEPIN_LEFT    10
#define NINEPIN_RIGHT   15
#define NINEPIN_BUTTON  13

#define PICO_WSPIZERO_BOARD

// default anything we haven't set above
#include "mcu.h"

#endif

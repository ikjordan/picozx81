// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_WSPIZEROBOARD_H
#define _BOARDS_WSPIZEROBOARD_H

// For board detection
#define RASPBERRYPI_BOARDS_WSPIZEROBOARD


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

#define NINEPIN_JOYSTICK

#define NINEPIN_UP      11
#define NINEPIN_DOWN    10
#define NINEPIN_LEFT    13
#define NINEPIN_RIGHT   15
#define NINEPIN_BUTTON  12

#define PICO_WSPIZERO_BOARD

// ws pizero has a Pico on it, so default anything we haven't set above
#include "boards/pico.h"

#endif

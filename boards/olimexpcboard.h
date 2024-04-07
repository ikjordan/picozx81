// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_OLIMEXPCBOARD_H
#define _BOARDS_OLIMEXPCBOARD_H

// For board detection
#define RASPBERRYPI_BOARDS_OLIMEXPCBOARD


#define OLIMEXPCBOARD_SD_CLK_PIN 6
#define OLIMEXPCBOARD_SD_CMD_PIN 7
#define OLIMEXPCBOARD_SD_DAT0_PIN 4

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
#define PICO_SD_CLK_PIN OLIMEXPCBOARD_SD_CLK_PIN
#endif

#ifndef PICO_SD_CMD_PIN
#define PICO_SD_CMD_PIN OLIMEXPCBOARD_SD_CMD_PIN
#endif

#ifndef PICO_SD_DAT0_PIN
#define PICO_SD_DAT0_PIN OLIMEXPCBOARD_SD_DAT0_PIN
#endif

// 1 or 4
#ifndef PICO_SD_DAT_PIN_COUNT
#define PICO_SD_DAT_PIN_COUNT 15
#endif

// 1 or -1
#ifndef PICO_SD_DAT_PIN_INCREMENT
#define PICO_SD_DAT_PIN_INCREMENT 1
#endif

#define OLIMEXPCBOARD_PWM_R_PIN 27
#define OLIMEXPCBOARD_PWM_L_PIN 28

#ifndef PICO_AUDIO_PWM_L_PIN
#define PICO_AUDIO_PWM_L_PIN OLIMEXPCBOARD_PWM_L_PIN
#endif

#ifndef PICO_AUDIO_PWM_R_PIN
#define PICO_AUDIO_PWM_R_PIN OLIMEXPCBOARD_PWM_R_PIN
#endif


#define PICO_OLIMEXPC_BOARD

// dviboard has a Pico on it, so default anything we haven't set above
#include "boards/pico.h"

#endif

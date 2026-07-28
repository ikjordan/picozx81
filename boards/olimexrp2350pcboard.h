// -----------------------------------------------------
// NOTE: THIS HEADER IS ALSO INCLUDED BY ASSEMBLER SO
//       SHOULD ONLY CONSIST OF PREPROCESSOR DIRECTIVES
// -----------------------------------------------------

#ifndef _BOARDS_OLIMEXRP2350PCBOARD_H
#define _BOARDS_OLIMEXRP2350PCBOARD_H

// For board detection
#define RASPBERRYPI_BOARDS_OLIMEXRP2350PCBOARD

#define PICO_DEFAULT_LED_PIN            25

#define OLIMEXRP2350PCBOARD_SD_CLK_PIN  10
#define OLIMEXRP2350PCBOARD_SD_CMD_PIN  11
#define OLIMEXRP2350PCBOARD_SD_DAT0_PIN 24
#define OLIMEXRP2350PCBOARD_SD_CS_PIN   9

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
#define PICO_SD_CLK_PIN OLIMEXRP2350PCBOARD_SD_CLK_PIN
#endif

#ifndef PICO_SD_CMD_PIN
#define PICO_SD_CMD_PIN OLIMEXRP2350PCBOARD_SD_CMD_PIN
#endif

#ifndef PICO_SD_DAT0_PIN
#define PICO_SD_DAT0_PIN OLIMEXRP2350PCBOARD_SD_DAT0_PIN
#endif

#ifndef PICO_SD_CS_PIN
#define PICO_SD_CS_PIN OLIMEXRP2350PCBOARD_SD_CS_PIN
#endif

#define OLIMEXRP2350PCBOARD_PWM_R_PIN 27
#define OLIMEXRP2350PCBOARD_PWM_L_PIN 26

#ifndef PICO_AUDIO_PWM_L_PIN
#define PICO_AUDIO_PWM_L_PIN OLIMEXRP2350PCBOARD_PWM_L_PIN
#endif

#ifndef PICO_AUDIO_PWM_R_PIN
#define PICO_AUDIO_PWM_R_PIN OLIMEXRP2350PCBOARD_PWM_R_PIN
#endif

#define NINEPIN_JOYSTICK

#define NINEPIN_UP      38      // UEXT2 Pin 3
#define NINEPIN_DOWN    39      // UEXT2 Pin 4
// Skip 5 and 6 as they have external pull ups
#define NINEPIN_LEFT    44      // UEXT2 Pin 7
#define NINEPIN_RIGHT   43      // UEXT2 Pin 8
#define NINEPIN_BUTTON  42      // UEXT2 Pin 9
                                // UEXT2 Pin 2 is Ground

// I2C
#define PICO_DEFAULT_I2C         0                               
#define PICO_DEFAULT_I2C_SDA_PIN 32
#define PICO_DEFAULT_I2C_SCL_PIN 33

#define PICO_ES8311_ADDR         0x18

// I2S
#define PICO_CODEC_PWR_DIS_PIN   22   // Olimex: active HIGH disables codec power
#define PICO_MCLK_PIN            23
#define PICO_I2S_DOUT_PIN        28   // ES8311 ASDOUT -> RP2350
#define PICO_I2S_LRCK_PIN        29
#define PICO_I2S_BCLK_PIN        30

#define PICO_OLIMEXRP2350PC_BOARD

// default anything we haven't set above
#include "mcu.h"

// Ensure we enable the extra features on a 2350B
#undef  PICO_RP2350A
#define PICO_RP2350A 0


#endif

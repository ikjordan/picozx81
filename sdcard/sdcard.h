#ifndef _SDCARD_H_
#define _SDCARD_H_
#include "pico/config.h"

#define SDCARD_PIO              pio1

#if defined(PICO_VGA332_BOARD)

#define SDCARD_PIN_SPI0_CS      PICO_DEFAULT_SPI_CSN_PIN
#define SDCARD_PIN_SPI0_SCK     PICO_DEFAULT_SPI_SCK_PIN
#define SDCARD_PIN_SPI0_MOSI    PICO_DEFAULT_SPI_TX_PIN
#define SDCARD_PIN_SPI0_MISO    PICO_DEFAULT_SPI_RX_PIN

#elif (defined(PICO_VGA_BOARD) || defined(PICO_DVI_BOARD) || defined(PICO_PICOMITEVGA_BOARD) || defined(PICO_OLIMEXPC_BOARD) || defined(PICO_WSPIZERO_BOARD))

#define SDCARD_PIN_SPI0_CS      (PICO_SD_CMD_PIN + PICO_SD_DAT_PIN_COUNT)
#define SDCARD_PIN_SPI0_SCK     PICO_SD_CLK_PIN
#define SDCARD_PIN_SPI0_MOSI    PICO_SD_CMD_PIN
#define SDCARD_PIN_SPI0_MISO    PICO_SD_DAT0_PIN

#else // PICO_LCDWS28_BOARD, PICO_LCDMAKER_BOARD, PICO_VGAMAKER222C_BOARD
#define SDCARD_PIN_SPI0_CS      PICO_SD_CS_PIN
#define SDCARD_PIN_SPI0_SCK     PICO_SD_CLK_PIN
#define SDCARD_PIN_SPI0_MOSI    PICO_SD_CMD_PIN
#define SDCARD_PIN_SPI0_MISO    PICO_SD_DAT0_PIN
#endif

#endif // _SDCARD_H_

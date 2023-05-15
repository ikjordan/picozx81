#ifndef _SDCARD_H_
#define _SDCARD_H_
#include "pico/config.h"

#define SDCARD_PIO              pio1
#define SDCARD_PIO_SM           0

#if defined(PICO_VGA332_BOARD)

#define SDCARD_PIN_SPI0_CS      PICO_DEFAULT_SPI_CSN_PIN
#define SDCARD_PIN_SPI0_SCK     PICO_DEFAULT_SPI_SCK_PIN
#define SDCARD_PIN_SPI0_MOSI    PICO_DEFAULT_SPI_TX_PIN
#define SDCARD_PIN_SPI0_MISO    PICO_DEFAULT_SPI_RX_PIN

#elif (defined(PICO_VGA_BOARD) || defined(PICO_DVI_BOARD))

#define SDCARD_PIN_SPI0_CS      (PICO_SD_CMD_PIN + PICO_SD_DAT_PIN_COUNT)
#define SDCARD_PIN_SPI0_SCK     PICO_SD_CLK_PIN
#define SDCARD_PIN_SPI0_MOSI    PICO_SD_CMD_PIN
#define SDCARD_PIN_SPI0_MISO    PICO_SD_DAT0_PIN

#endif

#if 0
#ifndef SDCARD_SPI_BUS
#define SDCARD_SPI_BUS spi0
#endif

#ifndef SDCARD_PIN_SPI0_CS
#define SDCARD_PIN_SPI0_CS     22
#endif

#ifndef SDCARD_PIN_SPI0_SCK
#define SDCARD_PIN_SPI0_SCK    18
#endif

#ifndef SDCARD_PIN_SPI0_MOSI
#define SDCARD_PIN_SPI0_MOSI   19
#endif

#ifndef SDCARD_PIN_SPI0_MISO 
#define SDCARD_PIN_SPI0_MISO   16
#endif
#endif
#endif // _SDCARD_H_

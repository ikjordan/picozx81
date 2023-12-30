#ifndef _DISPLAY_PRIV_H_
#define _DISPLAY_PRIV_H_
#if !(defined (PICO_DVI_BOARD) || defined (PICO_OLIMEXPC_BOARD) || defined (PICO_LCD_CS_PIN))
#include "pico/scanvideo.h"
#endif

typedef struct
{
  unsigned int  width;
  unsigned int  height;
  unsigned int  bitDepth;
  uint16_t      palette[4];
  uint8_t  __attribute__((aligned(4)))  pixel_data[];
} KEYBOARD_PIC;

#if !(defined (PICO_DVI_BOARD) || defined (PICO_OLIMEXPC_BOARD) || defined (PICO_LCD_CS_PIN))
#define BLACK PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0, 0)
#define WHITE PICO_SCANVIDEO_PIXEL_FROM_RGB8(255, 255, 255)
#define RED PICO_SCANVIDEO_PIXEL_FROM_RGB8(255, 0, 0)
#define BLUE PICO_SCANVIDEO_PIXEL_FROM_RGB8(0, 0, 255)
#define YELLOW PICO_SCANVIDEO_PIXEL_FROM_RGB8(255, 255, 0)
#elif (defined (PICO_LCD_CS_PIN))
#define WHITE  0xfff
#define BLUE   0x00f
#define YELLOW 0xff0
#define RED    0xf00
#define BLACK  0x000
#else
#define WHITE 0
#define RED 1
#define YELLOW 1
#define BLUE 2
#define BLACK 3
#endif

#endif

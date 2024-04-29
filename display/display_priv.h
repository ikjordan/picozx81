#ifndef _DISPLAY_PRIV_H_
#define _DISPLAY_PRIV_H_
#if !(defined (PICO_DVI_BOARD) || defined (PICO_OLIMEXPC_BOARD) || defined (PICO_WSPIZERO_BOARD) || defined (PICO_LCD_CS_PIN))
#include "pico/scanvideo.h"
#endif
#include "pico/sync.h"

typedef struct
{
  unsigned int  width;
  unsigned int  height;
  unsigned int  bitDepth;
  uint16_t      palette[4];
  uint8_t  __attribute__((aligned(4)))  pixel_data[];
} KEYBOARD_PIC;

// Keyboard display
extern const KEYBOARD_PIC ZX80KYBD;
extern const KEYBOARD_PIC ZX81KYBD;
#ifdef PICO_PICOZX_BOARD
extern const KEYBOARD_PIC ZX80KYBD_LCD;
extern const KEYBOARD_PIC ZX81KYBD_LCD;
#endif
extern const KEYBOARD_PIC* keyboard;
extern bool showKeyboard;

// Synchronisation primitives
extern mutex_t next_frame_mutex;
extern semaphore_t display_initialised;

// Display blank screen
extern bool blank;
extern uint16_t blank_colour;

// Buffers to display
extern uint8_t* curr_buff;            // main display buffer being displayed
extern uint8_t* cbuffer;              // chroma buffer being displayed

// Functions used by display specific driver
#if (defined PICO_PICOZX_BOARD)
extern void core1_main_vga(void);
extern void core1_main_lcd(void);

#else
extern void core1_main(void);
#endif
extern void displayAllocateBuffers(uint16_t minBuffByte, uint16_t width, uint16_t height);
extern void displayStartCommon(void);
extern void newFrame(void);


#if (defined PICO_VGA_BOARD)
#define PICO_SCANVIDEO_PIXEL_FROM_RGB(r, g, b) ((((b)>>3u)<<PICO_SCANVIDEO_PIXEL_BSHIFT)|(((g)>>3u)<<PICO_SCANVIDEO_PIXEL_GSHIFT)|(((r)>>3u)<<PICO_SCANVIDEO_PIXEL_RSHIFT))
#elif (defined PICO_PICOMITEVGA_BOARD)
#define PICO_SCANVIDEO_PIXEL_FROM_RGB(r, g, b) ((((b)>>7u)<<PICO_SCANVIDEO_PIXEL_BSHIFT)|(((g)>>6u)<<PICO_SCANVIDEO_PIXEL_GSHIFT)|(((r)>>7u)<<PICO_SCANVIDEO_PIXEL_RSHIFT))
#elif (defined PICO_VGA332_BOARD)
#define PICO_SCANVIDEO_PIXEL_FROM_RGB(r, g, b) ((((b)>>5u)<<PICO_SCANVIDEO_PIXEL_BSHIFT)|(((g)>>5u)<<PICO_SCANVIDEO_PIXEL_GSHIFT)|(((r)>>6u)<<PICO_SCANVIDEO_PIXEL_RSHIFT))
#elif ((defined PICO_VGAMAKER222C_BOARD) || (defined PICO_PICOZX_BOARD) || (defined PICO_PICOZXREAL_BOARD))
#define PICO_SCANVIDEO_PIXEL_FROM_RGB(r, g, b) ((((b)>>6u)<<PICO_SCANVIDEO_PIXEL_BSHIFT)|(((g)>>6u)<<PICO_SCANVIDEO_PIXEL_GSHIFT)|(((r)>>6u)<<PICO_SCANVIDEO_PIXEL_RSHIFT))
#endif

#if (defined PICO_SCANVIDEO_PIXEL_FROM_RGB)
#define BLACK PICO_SCANVIDEO_PIXEL_FROM_RGB(0, 0, 0)
#define WHITE PICO_SCANVIDEO_PIXEL_FROM_RGB(255, 255, 255)
#define RED PICO_SCANVIDEO_PIXEL_FROM_RGB(255, 0, 0)
#define BLUE PICO_SCANVIDEO_PIXEL_FROM_RGB(0, 0, 255)
#define YELLOW PICO_SCANVIDEO_PIXEL_FROM_RGB(255, 255, 0)
#elif (defined PICO_LCD_CS_PIN)
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

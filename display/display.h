#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include "pico.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void displayInitialise(void);
extern void displaySetBuffer(uint8_t* buff, uint16_t x_off, uint16_t y_off, uint16_t stride);
extern void displayStart(void);
extern void displayBlank(bool black);
extern void displayGetCurrent(uint8_t** buff, uint16_t* x_off, uint16_t* y_off, uint16_t* stride);
extern bool displayIsBlank(bool* isBlack);
extern void displayShowKeyboard(bool zx81);
extern void displayHideKeyboard(void);

#ifdef __cplusplus
}
#endif

#endif
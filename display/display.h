#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include "pico.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint displayInitialise(bool fiveSevenSix, uint16_t minBuffByte, uint16_t* pixelWidth,
                              uint16_t* pixelHeight, uint16_t* strideBit);
extern void displayStart(void);

extern void displayGetFreeBuffer(uint8_t** buff);
extern void displayBuffer(uint8_t* buff, bool sync, bool free);
extern void displayGetCurrentBuffer(uint8_t** buff);
extern void displaySetInterlace(bool on);

extern void displayBlank(bool black);
extern bool displayIsBlank(bool* isBlack);

extern void displayShowKeyboard(bool zx81);
extern void displayHideKeyboard(void);


#ifdef __cplusplus
}
#endif

#endif
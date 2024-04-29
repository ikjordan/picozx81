#ifndef _DISPLAY_H_
#define _DISPLAY_H_
#include "pico.h"
#ifdef SOUND_HDMI
#include "audio_ring.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t audioRate;
} DisplayExtraInfoHDMI_T;

typedef struct
{
    bool invertColour;  // Invert colours on display
    bool skipFrame;     // Skip every other frame
    bool rotate;        // Rotate display by 180 degrees
    bool reflect;       // Change horizontal scan direction
    bool bgr;           // use bgr instead of rgb pixel order
} DisplayExtraInfoLCD_T;

typedef struct
{
    union
    {
        DisplayExtraInfoHDMI_T hdmi;
        DisplayExtraInfoLCD_T lcd;
    } info;
} DisplayExtraInfo_T;

#ifdef PICOZX_LCD
extern bool useLCD;
#endif

/*
    fivesevensix    if true request resolution 720x576, else 640x480
    match           if true request refresh frequency of PAL ZX81 (50.65 Hz)
    minBuffByte     Minimum end of line buffer size, in bytes
    pixel width     Returns the actual number of horizontal pixels assigned
    pixel height    Returns the actual number of vertical pixels assigned
    stridebit       The actual number of bits between start of adjacent lines (pixel width + bufferbits)
    info            Extra information for HDMI audio and LCD displays
 */

/* Display specific, defined for all display types */
extern uint displayInitialise(bool fiveSevenSix, bool match, uint16_t minBuffByte, uint16_t* pixelWidth,
                              uint16_t* pixelHeight, uint16_t* strideBit, DisplayExtraInfo_T* info);
extern void displayStart(void);

extern bool displayShowKeyboard(bool zx81);

/* Common */
extern void displayGetFreeBuffer(uint8_t** buff);
extern void displayBuffer(uint8_t* buff, bool sync, bool free, bool chroma);
extern void displayGetCurrentBuffer(uint8_t** buff);
extern void displayGetChromaBuffer(uint8_t** chroma, uint8_t* buff);
extern void displayResetChroma(void);
extern void displaySetInterlace(bool on);

extern void displayBlank(bool black);
extern bool displayIsBlank(bool* isBlack);

extern bool displayHideKeyboard(void);

#ifdef PICO_SPI_LCD_SD_SHARE
extern void displayRequestSPIBus(void);
extern void displayGrantSPIBus(void);
#endif

#ifdef SOUND_HDMI
void getAudioRing(audio_ring_t** ring);
#endif

#ifdef __cplusplus
}
#endif

#endif

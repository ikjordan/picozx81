#ifndef EMUVIDEO_H
#define EMUVIDEO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    uint16_t width;         // width of screen in pixels
    uint16_t height;        // Height of screen in pixels
    uint16_t length;        // Size of buffer, including padding
    uint16_t stride_bit;    // bits in one line including padding
    uint16_t stride_byte;   // bytes in one line including padding
    uint16_t start_x;       // X Offset in bits to first pixel to display, without centring
    uint16_t end_x;         // X Offset in bits to last pixel to display, without centring
    uint16_t start_y;       // Y Offset in lines to first line to display
    uint16_t end_y;         // Y Offset in lines to last line to display
    int16_t  adjust_x;      // Pixels to skip before start to display line
    int16_t  offset;        // Offset in bits to convert from raster to display coords
    uint16_t padding;       // Padding per line in bytes
} Display_T;

extern Display_T disp;

extern uint emu_VideoInit(void);
extern void emu_VideoSetInterlace(void);
extern bool emu_VideoWriteBitmap(const char* file_name, const uint8_t* pixel_data, const uint8_t* chroma_data);

#ifdef __cplusplus
}
#endif

#endif

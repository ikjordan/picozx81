#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "emuvideo.h"
#include "common.h"

#include "display.h"
#include "chars.h"

static char ascii2zx[96]=
{
   0, 0,11,12,13, 0, 0,11,16,17,23,21,26,22,27,24,
  28,29,30,31,32,33,34,35,36,37,14,25,19,20,18,15,
  23,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,
  53,54,55,56,57,58,59,60,61,62,63,16,24,17,11,22,
  11,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,
  53,54,55,56,57,58,59,60,61,62,63,16,24,17,11, 0
};

static uint8_t* charScreen = 0;
static uint8_t* charChroma = 0;
static bool zx80font = false;

void charSetScreenPtr(uint8_t* screen)
{
    charScreen = screen;
}

void charSetChromaPtr(uint8_t* screen)
{
    charChroma = screen;
}

void charSetScreenFont(bool zx80)
{
    zx80font = zx80;
}

void charSetConvert(bool zx80)
{
    if (zx80)
    {
        ascii2zx['"' - 32] = 0x01;
        ascii2zx['-' - 32] = 0x12;
        ascii2zx['+' - 32] = 0x13;
        ascii2zx['*' - 32] = 0x14;
        ascii2zx['/' - 32] = 0x15;
        ascii2zx['=' - 32] = 0x16;
        ascii2zx['>' - 32] = 0x17;
        ascii2zx['<' - 32] = 0x18;
        ascii2zx[13] = 0x12;
        ascii2zx[63] = 0x12;
    }
    else
    {
        ascii2zx['"' - 32] = 0x0B;
        ascii2zx['-' - 32] = 0x16;
        ascii2zx['+' - 32] = 0x15;
        ascii2zx['*' - 32] = 0x17;
        ascii2zx['/' - 32] = 0x18;
        ascii2zx['=' - 32] = 0x14;
        ascii2zx['>' - 32] = 0x12;
        ascii2zx['<' - 32] = 0x13;
    }
}

// Inverts a row
void charXorRow(uint32_t row)
{
    uint8_t* screen = charScreen;
    screen += row * disp.stride_bit;
    for (uint32_t i=0; i<disp.stride_bit; ++i)
    {
        *screen++ ^= 0xff;
    }
}

// Write string to screen, terminate string at screen edge
void charWriteString(const char* s, uint32_t col, uint32_t row)
{
    charWriteInvertString(s, col, row, false);
}

// Write string to screen, terminate string at screen edge, optionally inverting characters
void charWriteInvertString(const char* s, uint32_t col, uint32_t row, bool invert)
{
    if ((col < (uint32_t)(disp.width>>3)) && (row < (uint32_t)(disp.height>>3)))
    {
        unsigned int len = strlen(s);

        len = len > ((disp.width>>3)-col) ? ((disp.width>>3)-col) : len;
        for (uint32_t i=0; i<len; ++i)
        {
            if (invert)
            {
                charInvertChar(s[i], col+i, row);
            }
            else
            {
                charWriteChar(s[i], col+i, row);
            }
        }
    }
}

// Write one character to screen
void charWriteChar(char c, uint32_t col, uint32_t row)
{
    uint8_t* pos = charScreen + row * disp.stride_bit + col;
    uint16_t offset = zx80font ? 0x0e00 : 0x1e00;   // Start of characters in ROM

    // Convert from ascii to ZX
    if ((c >= 32) && (c < 128))
    {
        offset += (ascii2zx[c-32] << 3);
    }

    // Find the offset in the ROM
    for (uint32_t i=0; i<8; ++i)
    {
        *pos = mem[offset+i];
        pos += disp.stride_byte;
    }
}

void charInvertChar(char c, uint32_t col, uint32_t row)
{
    uint8_t* pos = charScreen + row * disp.stride_bit + col;
    uint16_t offset = zx80font ? 0x0e00 : 0x1e00;   // Start of characters in ROM

    // Convert from ascii to ZX
    if ((c >= 32) && (c < 128))
    {
        offset += (ascii2zx[c-32] << 3);
    }

    // Find the offset in the ROM
    for (uint32_t i=0; i<8; ++i)
    {
        *pos = (mem[offset+i] ^ 0xff);
        pos += disp.stride_byte;
    }

    // Update chroma foreground and background, so inverse char is visible
    if (charChroma)
    {
        pos = charChroma + row * disp.stride_bit + col;
        for (uint32_t i=0; i<8; ++i)
        {
            *pos = 0xf0;
            pos += disp.stride_byte;
        }
    }
}

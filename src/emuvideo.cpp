/*******************************************************************
 Video
*******************************************************************/
#include <stdio.h>
#include <memory.h>
#include "pico.h"
#include "emuapi.h"
#include "emusound.h"
#include "emuvideo.h"
#include "display.h"

// Sizes for 640 by 480
#define DISPLAY_START_PIXEL_640 46  // X Offset to first pixel with no centring
#define DISPLAY_START_Y_640     24  // Y Offset to first pixel with no centring
#define DISPLAY_ADJUST_X_640     4  // The number of pixels to adjust in X dimension to centre the display

// Sizes for 720 by 576
#define DISPLAY_START_PIXEL_720 24  // X Offset to first pixel with no centring
#define DISPLAY_START_Y_720      0  // Y Offset to first pixel with no centring
#define DISPLAY_ADJUST_X_720     2  // The number of pixels to adjust in X dimension to centre the display

#include <stdio.h>
#include <stdint.h>

#pragma pack(push, 1)  // Ensures that the structures are packed without padding
// BMP File Header Structure
typedef struct {
    int8_t   signature[2];   // 'BM'
    uint32_t file_size;     // Size of the file
    uint16_t reserved1;     // Reserved field
    uint16_t reserved2;     // Reserved field
    uint32_t data_offset;   // Offset where pixel data begins
} BMPFileHeader;

// BMP Info Header Structure
typedef struct {
    uint32_t header_size;       // Size of this header (40 bytes)
    int32_t  width;             // Image width
    int32_t  height;            // Image height
    uint16_t planes;            // Number of color planes (must be 1)
    uint16_t bits_per_pixel;    // Bits per pixel (8 for indexed color)
    uint32_t compression;       // Compression method (0 for none)
    uint32_t image_size;        // Image size (0 for no compression)
    int32_t  x_resolution;      // Horizontal resolution (pixels per meter)
    int32_t  y_resolution;      // Vertical resolution (pixels per meter)
    uint32_t colors_used;       // Number of colors in the color table (256 for 8-bit color)
    uint32_t important_colors;  // Important colors (0 means all colors are important)
} BMPInfoHeader;

// Color table entry structure (4 bytes per color)
typedef struct {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
} RGBQuad;
#pragma pack(pop)               // Reset the packing alignment to default

Display_T disp;                 // Dimension information for the display

uint emu_VideoInit(void)
{
    bool match = false;
    DisplayExtraInfo_T extra;

    bool five = (emu_576Requested() != OFF);

    if (five)
    {
        match = (emu_576Requested() != ON);
    }

#ifdef PICO_LCD_CS_PIN
    extra.info.lcd.invertColour = emu_lcdInvertColourRequested();
    extra.info.lcd.skipFrame = emu_lcdSkipFrameRequested();
    extra.info.lcd.rotate = emu_lcdRotateRequested();
    extra.info.lcd.reflect = emu_lcdReflectRequested();
    extra.info.lcd.bgr = emu_lcdBGRRequested();
#else
    extra.info.hdmi.audioRate = emu_sndGetSampleRate();
#endif

    uint clock = displayInitialise(five, match, 1, &disp.width,
                                   &disp.height, &disp.stride_bit, &extra);

    if (disp.width >= 360)      // As use double pixels
    {
        disp.start_x = DISPLAY_START_PIXEL_720;
        disp.start_y = DISPLAY_START_Y_720;
        disp.adjust_x = DISPLAY_ADJUST_X_720;
    }
    else
    {
        disp.start_x = DISPLAY_START_PIXEL_640;
        disp.start_y = DISPLAY_START_Y_640;
        disp.adjust_x = DISPLAY_ADJUST_X_640;
    }

    // Initialise the rest of the structure
    disp.stride_byte = disp.stride_bit >> 3;
    disp.end_x = disp.start_x + disp.width;
    disp.end_y = disp.height + disp.start_y;
    disp.offset = -(disp.stride_bit * disp.start_y) - disp.start_x;
    disp.padding = (disp.stride_bit - disp.width) >> 3;
    disp.length = disp.stride_byte * disp.height;

    return clock;
}

void emu_VideoSetInterlace(void)
{
    displaySetInterlace(emu_FrameSyncRequested() == SYNC_ON_INTERLACED);
}

#define FOURBPP 16

static void write_file_header(void)
{
    BMPFileHeader file_header;

    // Set File Header
    file_header.signature[0] = 'B';
    file_header.signature[1] = 'M';
    file_header.file_size = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + (FOURBPP * sizeof(RGBQuad)) + ((disp.width * disp.height) >> 1);
    file_header.reserved1 = 0;
    file_header.reserved2 = 0;
    file_header.data_offset = sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + (FOURBPP * sizeof(RGBQuad));

    emu_FileWriteBytes(&file_header, sizeof(BMPFileHeader));
}

static void write_info_header(void)
{
    BMPInfoHeader info_header;

    // Set Info Header
    info_header.header_size = sizeof(BMPInfoHeader);
    info_header.width = disp.width;
    info_header.height = disp.height;
    info_header.planes = 1;                 // Always 1 for BMP
    info_header.bits_per_pixel = 4;         // 4-bit indexed color
    info_header.compression = 0;            // No compression
    info_header.image_size = (disp.width * disp.height) >> 1;  // Image size (in bytes)
    info_header.x_resolution = 2835;        // 72 DPI * 39.3701 (conversion to pixels per metre)
    info_header.y_resolution = 2835;        // Same for vertical resolution
    info_header.colors_used = FOURBPP;      // 16 colours in the colour table
    info_header.important_colors = 0;       // 0 means all colours are important

    // Write headers to the file
    emu_FileWriteBytes(&info_header, sizeof(BMPInfoHeader));
}

static void write_colour_table(void)
{
    RGBQuad colour_table[FOURBPP] = { {0x00, 0x00, 0x00, 0x00},
                                      {0xaa, 0x00, 0x00, 0x00},
                                      {0x00, 0x00, 0xaa, 0x00},
                                      {0xaa, 0x00, 0xaa, 0x00},
                                      {0x00, 0xaa, 0x00, 0x00},
                                      {0xaa, 0xaa, 0x00, 0x00},
                                      {0x00, 0xaa, 0xaa, 0x00},
                                      {0xaa, 0xaa, 0xaa, 0x00},
                                      {0x00, 0x00, 0x00, 0x00},
                                      {0xff, 0x00, 0x00, 0x00},
                                      {0x00, 0x00, 0xff, 0x00},
                                      {0xff, 0x00, 0xff, 0x00},
                                      {0x00, 0xff, 0x00, 0x00},
                                      {0xff, 0xff, 0x00, 0x00},
                                      {0x00, 0xff, 0xff, 0x00},
                                      {0xff, 0xff, 0xff, 0x00}};

    // Write the color table (16 entries, 4 bytes each)
    emu_FileWriteBytes(colour_table, sizeof(colour_table));
}

static void write_pixel_data(const uint8_t* pixel_data, const uint8_t* chroma_data)
{
    uint8_t line_bytes[disp.width >> 1];

    // Write the data, one line at a time, from bottom to top
    pixel_data += disp.stride_byte * (disp.height -1);
    if (chroma_data) chroma_data += disp.stride_byte * (disp.height -1);

    for (int i = 0; i < disp.height; ++i)
    {
        // Populate the line: 8 pixels generate 4 bytes
        for (int j = 0; j < (disp.width >> 1); j += 4)
        {
            uint8_t pixels = *pixel_data++;
            uint8_t chroma = chroma_data ? *chroma_data++ : 0xf0;
            uint8_t background = (chroma & 0xf0) >> 4;
            uint8_t foreground = chroma & 0x0f;
            for (int k = 0; k < 4; ++k)
            {
                line_bytes[j + k] = (((pixels << (k << 1)) & 0x80) ? foreground : background) << 4;
                line_bytes[j + k] += (((pixels << (k << 1)) & 0x40) ? foreground : background);
            }
        }
        emu_FileWriteBytes(line_bytes, disp.width >> 1);
        pixel_data -= (2 * disp.stride_byte - disp.padding);    // Move to start of previous line
        if (chroma_data) chroma_data -= (2 * disp.stride_byte - disp.padding);
    }
}

bool emu_VideoWriteBitmap(const char* file_name, const uint8_t* pixel_data, const uint8_t* chroma_data)
{
    if (emu_FileOpen(file_name, "w"))
    {
        printf("In emu_VideoWriteBitmap: writing BMP file: %s\n", file_name);
        write_file_header();
        write_info_header();
        write_colour_table();                       // Write the colour palette
        write_pixel_data(pixel_data, chroma_data);  // Write the pixel data

        // close file
        emu_FileClose();
    }
    else
    {
        printf("In emu_VideoWriteBitmap: Error creating BMP file: %s\n", file_name);
        return false;
    }
    return true;
}


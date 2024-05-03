#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#include "display.h"
#include "display_priv.h"

// Union to allow variable length data manipulation
typedef union
{
    uint16_t i16[8];
    uint32_t i32[4];
    uint64_t i64[2];
} Fill_u;

static uint16_t PIXEL_WIDTH = 0;
static uint16_t BYTE_WIDTH = 0;
static uint16_t HEIGHT = 0;

static const uint16_t MIN_RUN = 3;

static uint16_t keyboard_x = 0;
static uint16_t keyboard_y = 0;

static uint16_t stride = 0;

// Do not make const - as want to keep in RAM
static uint16_t colour_table[16] = {
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0x00, 0x00, 0x00),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0x00, 0x00, 0xaa),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0xaa, 0x00, 0x00),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0xaa, 0x00, 0xaa),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0x00, 0xaa, 0x00),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0x00, 0xaa, 0xaa),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0xaa, 0xaa, 0x00),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0xaa, 0xaa, 0xaa),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0x00, 0x00, 0x00),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0x00, 0x00, 0xff),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0xff, 0x00, 0x00),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0xff, 0x00, 0xff),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0x00, 0xff, 0x00),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0x00, 0xff, 0xff),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0xff, 0xff, 0x00),
    PICO_SCANVIDEO_PIXEL_FROM_RGB(0xff, 0xff, 0xff)
};

// Define timing here due to sdk issue with h_pulse length
const scanvideo_timing_t vga_timing_640x480_60_default1 =
{
    .clock_freq = 25000000,
    .h_active = 640,
    .v_active = 480,

    .h_front_porch = 16,
    .h_pulse = 96,
    .h_total = 800,
#if ((defined PICO_VGAMAKER222C_BOARD) || (defined PICO_PICOZX_BOARD) || (defined PICO_PICOZXREAL_BOARD))
    .h_sync_polarity = 5,
#else
    .h_sync_polarity = 1,
#endif

    .v_front_porch = 10,
    .v_pulse = 2,
    .v_total = 525,
    .v_sync_polarity = 1,

    .enable_clock = 0,
    .clock_polarity = 0,

    .enable_den = 0
};

const scanvideo_mode_t vga_mode_320x240_60d =
{
    .default_timing = &vga_timing_640x480_60_default1,
    .pio_program = &video_24mhz_composable,
    .width = 320,
    .height = 240,
    .xscale = 2,
    .yscale = 2,
};

const scanvideo_timing_t vga_timing_720x576_50 =
{
    .clock_freq = 27000000,
    .h_active = 720,
    .v_active = 576,
    .h_front_porch = 12,
    .h_pulse = 64,
    .h_total = 864,
#if ((defined PICO_VGAMAKER222C_BOARD) || (defined PICO_PICOZX_BOARD) || (defined PICO_PICOZXREAL_BOARD))
    .h_sync_polarity = 5,
#else
    .h_sync_polarity = 1,
#endif
    .v_front_porch = 5,
    .v_pulse = 5,
    .v_total = 625,
    .v_sync_polarity = 1,
    .enable_clock = 0,
    .clock_polarity = 0,
    .enable_den = 0
};

const scanvideo_mode_t vga_mode_360x288_50 =
{
    .default_timing = &vga_timing_720x576_50,
    .pio_program = &video_24mhz_composable,
    .width = 360,
    .height = 288,
    .xscale = 2,
    .yscale = 2,
};

// 576p 50Hz accelerated by 625/617 * 50 -> 50.65 Hz
const scanvideo_timing_t vga_timing_720x576_51 =
{
    .clock_freq = 27000000,
    .h_active = 720,
    .v_active = 576,
    .h_front_porch = 12,
    .h_pulse = 64,
    .h_total = 864,
#if ((defined PICO_VGAMAKER222C_BOARD) || (defined PICO_PICOZX_BOARD) || (defined PICO_PICOZXREAL_BOARD))
    .h_sync_polarity = 5,
#else
    .h_sync_polarity = 1,
#endif
    .v_front_porch = 5,
    .v_pulse = 5,
    .v_total = 617,
    .v_sync_polarity = 1,
    .enable_clock = 0,
    .clock_polarity = 0,
    .enable_den = 0
};

const scanvideo_mode_t vga_mode_360x288_51 =
{
    .default_timing = &vga_timing_720x576_51,
    .pio_program = &video_24mhz_composable,
    .width = 360,
    .height = 288,
    .xscale = 2,
    .yscale = 2,
};

static const scanvideo_mode_t* video_mode = 0;

//
// Private interface
//

static int32_t populate_line(uint8_t* display_line, uint8_t* colour_line, uint32_t* buff);
static int32_t populate_blank_line(uint16_t colour, uint32_t* buff);
static int32_t populate_keyboard_line(int linenum, uint16_t bcolour, uint32_t* buff);
static int32_t populate_mixed_line(uint8_t* display_line, uint8_t* colour_line, int linenum, uint32_t* buff);
static void render_loop();
static Fill_u expand_display(uint8_t disp, uint8_t colours);

//
// Public functions
//

    // Determine the video mode
#ifndef PICOZX_LCD
uint displayInitialise
#else
uint displayInitialiseVGA
#endif
                      (bool fiveSevenSix, bool match, uint16_t minBuffByte, uint16_t* pixelWidth,
                       uint16_t* pixelHeight, uint16_t* strideBit, DisplayExtraInfo_T* info)
{
    (void)info;

    mutex_init(&next_frame_mutex);

    // Determine the video mode
    video_mode = (!fiveSevenSix) ? &vga_mode_320x240_60d : (match) ? &vga_mode_360x288_51 : &vga_mode_360x288_50;

    PIXEL_WIDTH = video_mode->width;
    HEIGHT = video_mode->height;
    BYTE_WIDTH = PIXEL_WIDTH >> 3;

    // Is padding requested? For VGA can pad a single byte
    stride = minBuffByte + BYTE_WIDTH;

    // Allocate the buffers
    displayAllocateBuffers(minBuffByte, stride, HEIGHT);

    // Return the values
    *pixelWidth = PIXEL_WIDTH;
    *pixelHeight = HEIGHT;
    *strideBit = stride << 3;

    // Set default to blank screen and return clock speed
    blank = true;

    return video_mode->default_timing->clock_freq / 100;
}

#ifndef PICOZX_LCD
void displayStart(void)
#else
void displayStartVGA(void)
#endif
{
    displayStartCommon();
}

#ifndef PICOZX_LCD
bool displayShowKeyboard(bool zx81)
#else
bool displayShowKeyboardVGA(bool zx81)
#endif
{
    bool previous = showKeyboard;

    if (!showKeyboard)
    {
        keyboard = zx81 ? &ZX81KYBD : &ZX80KYBD;
        keyboard_x = (video_mode->width - keyboard->width) >> 2;    // Centre (>>1), then 2 pixels per byte (>>1)
        keyboard_y = (HEIGHT - keyboard->height) >> 1;
        showKeyboard = true;
    }
    return previous;
}

//
// Private functions
//

/* Displays a line of keyboard against a blank background of the supplied colour */
static int32_t __not_in_flash_func(populate_keyboard_line)(int linenum, uint16_t bcolour, uint32_t* buff)
{
    // Need to interlace commands with first 2 pixels at start of buffer line
    // both of these will be black or white
    const uint8_t* pd = &keyboard->pixel_data[(linenum - keyboard_y) * (keyboard->width>>2)];

    buff[0] = COMPOSABLE_RAW_RUN | (bcolour << 16);

    // Note pixel length +1 as have final black pixel
    buff[1] = (PIXEL_WIDTH + 1 - MIN_RUN) | (bcolour << 16);

    // Now have further pixels of black or white
    for (int i=2; i < (keyboard_x+1); ++i)
    {
        buff[i] = (bcolour | bcolour << 16);
    }

    // Now have the picture
    for (unsigned int i = keyboard_x+1; i < ((keyboard->width>>1)+keyboard_x+1); i+=2)
    {
        buff[i] = keyboard->palette[(*pd & 0xc0) >> 6];
        buff[i] += keyboard->palette[(*pd & 0x30) >> 4] << 16;
        buff[i+1] = keyboard->palette[(*pd & 0x0c) >> 2];
        buff[i+1] += keyboard->palette[(*pd++ & 0x03)] << 16;
    }

    // Now have a further pixels of black or white
    for (int i=(keyboard->width>>1)+keyboard_x+1; i <= (PIXEL_WIDTH >> 1); ++i)
    {
        buff[i] = (bcolour | bcolour << 16);
    }

    // Must end with a black pixel
    buff[(PIXEL_WIDTH >> 1) + 1] = BLACK | (COMPOSABLE_EOL_ALIGN << 16);
    return (PIXEL_WIDTH >> 1) + 2;
}

/* Display a line that consists of pixel and (optionally) colour information with keyboard overlayed */
static int32_t __not_in_flash_func(populate_mixed_line)(uint8_t* display_line, uint8_t* colour_line, int linenum, uint32_t* buff)
{
    // Need to interlace commands with first 2 pixels at start of buffer line
    Fill_u pixels;
    uint16_t colours = (colour_line) ? 0x0 : 0xf0;  // Black foreground, white background

    // Extract the data for the first 8 pixels
    pixels = expand_display(display_line[0], colours ? colours : colour_line[0]);

    // interlace the first two
    buff[0] = COMPOSABLE_RAW_RUN          | (pixels.i16[0] << 16);

    // Note pixel length +1 as have final black pixel
    buff[1] = (PIXEL_WIDTH + 1 - MIN_RUN) | (pixels.i16[1] << 16);

    // Populate the rest of the pixels in the first byte
    buff[2] = pixels.i32[1];
    buff[3] = pixels.i32[2];
    buff[4] = pixels.i32[3];

    // Process the next 24 / 44 pixels up to a byte boundary
    uint64_t* dest = (uint64_t*)(&buff[5]);
    for (int i=1; i<(keyboard_x>>2); ++i)
    {
        pixels = expand_display(display_line[i], colours ? colours : colour_line[i]);
        *dest++ = pixels.i64[0];
        *dest++ = pixels.i64[1];
    }

    // Process any remaining pixels that are not in a full byte
    if (keyboard_x & 0x3)
    {
        pixels = expand_display(display_line[keyboard_x>>2], colours ? colours : colour_line[keyboard_x>>2]);

        for (int i = 0; i < (keyboard_x & 0x3); ++i)
        {
            // Add 1 to account for interlaced messages at start
            buff[keyboard_x - (keyboard_x & 0x3) + 1 + i] = pixels.i32[i];
        }
    }

    // Process the keyboard
    const uint8_t* pd = &keyboard->pixel_data[(linenum - keyboard_y) * (keyboard->width>>2)];

    for (unsigned int i = keyboard_x+1; i < ((keyboard->width>>1)+keyboard_x+1); i+=2)
    {
        buff[i] = keyboard->palette[(*pd & 0xc0) >> 6];
        buff[i] += keyboard->palette[(*pd & 0x30) >> 4] << 16;
        buff[i+1] = keyboard->palette[(*pd & 0x0c) >> 2];
        buff[i+1] += keyboard->palette[(*pd++ & 0x03)] << 16;
    }

    // Process the pixels after the keyboard
    // Start with any that are not part of a full byte
    if (keyboard_x & 0x3)
    {
        pixels = expand_display(display_line[(keyboard->width>>3)+(keyboard_x>>2)],
                                colours ? colours : colour_line[(keyboard->width>>3)+(keyboard_x>>2)]);

        for (int i = 0; i < (keyboard_x & 0x3); ++i)
        {
            // Add 1 to account for interlaced messages at start
            buff[(keyboard->width>>1)+keyboard_x+1+i] = pixels.i32[4-(keyboard_x&0x3)+i];
        }
    }

    // Now aligned to a byte boundary
    dest = (uint64_t*)(&buff[(keyboard->width>>1)+keyboard_x+1+(keyboard_x & 0x3)]);
    for (int i=BYTE_WIDTH-(keyboard_x>>2); i<BYTE_WIDTH; ++i)
    {
        pixels = expand_display(display_line[i], colours ? colours : colour_line[i]);
        *dest++ = pixels.i64[0];
        *dest++ = pixels.i64[1];
    }

    // Must end with a black pixel
    buff[(PIXEL_WIDTH >> 1) + 1] = BLACK | (COMPOSABLE_EOL_ALIGN << 16);
    return (PIXEL_WIDTH >> 1) + 2;
}

/* Display a line that consists of pixel and (optionally) colour information */
static int32_t __not_in_flash_func(populate_line)(uint8_t* display_line, uint8_t* colour_line, uint32_t* buff)
{
    // Need to interlace commands with first 2 pixels at start of buffer line
    Fill_u pixels;
    uint16_t colours = (colour_line) ? 0x0 : 0xf0;  // Black foreground, white background

    // Extract the data for the first 8 pixels
    pixels = expand_display(display_line[0], colours ? colours : colour_line[0]);

    // interlace the first two
    buff[0] = COMPOSABLE_RAW_RUN          | (pixels.i16[0] << 16);

    // Note pixel length +1 as have final black pixel
    buff[1] = (PIXEL_WIDTH + 1 - MIN_RUN) | (pixels.i16[1] << 16);

    // Populate the rest of the pixels in the first byte
    buff[2] = pixels.i32[1];
    buff[3] = pixels.i32[2];
    buff[4] = pixels.i32[3];

    // Process the remaining 320/8 - 1 bytes
    uint64_t* dest = (uint64_t*)(&buff[5]);
    for (int i=1; i<BYTE_WIDTH; ++i)
    {
        pixels = expand_display(display_line[i], colours ? colours : colour_line[i]);
        *dest++ = pixels.i64[0];
        *dest++ = pixels.i64[1];
    }

    // Must end with a black pixel
    buff[(PIXEL_WIDTH >> 1) + 1] = BLACK | (COMPOSABLE_EOL_ALIGN << 16);
    return (PIXEL_WIDTH >> 1) + 2;
}

/* Display a solid line with the supplied colour */
static int32_t __not_in_flash_func(populate_blank_line)(uint16_t colour, uint32_t* buff)
{
    buff[0] = COMPOSABLE_COLOR_RUN    | (colour << 16);
    buff[1] = (PIXEL_WIDTH - MIN_RUN) | (COMPOSABLE_RAW_1P <<16);
    buff[2] = BLACK | (COMPOSABLE_EOL_ALIGN << 16);

    return 3;
}

static void __not_in_flash_func(render_loop)()
{
    while (true)
    {
        struct scanvideo_scanline_buffer *buf = scanvideo_begin_scanline_generation(true);  // Wait to acquire a scanline
        unsigned int line_num = scanvideo_scanline_number(buf->scanline_id);                // The scanline

        if (line_num == 0)
        {
            // Check if need to display new image
            newFrame();
        }

        uint8_t* current = curr_buff;    // As disp_index can change at any time
        uint8_t* cbuf = cbuffer;

        if (showKeyboard && (line_num >= keyboard_y) && (line_num <(keyboard_y + keyboard->height)))
        {
            if (blank)
            {
                buf->data_used = populate_keyboard_line(line_num, blank_colour, buf->data);
            }
            else
            {
                buf->data_used = populate_mixed_line(&current[stride * line_num], cbuf ? &cbuf[stride * line_num] : cbuf,
                                                     line_num, buf->data);
            }
        }
        else
        {
            if (blank)
            {
                buf->data_used = populate_blank_line(blank_colour, buf->data);
            }
            else
            {
                buf->data_used = populate_line(&current[stride * line_num], cbuf ? &cbuf[stride * line_num] : cbuf,
                                               buf->data);
            }
        }
        buf->status = SCANLINE_OK;
        scanvideo_end_scanline_generation(buf);
    }
}

static Fill_u __not_in_flash_func(expand_display)(uint8_t disp, uint8_t colours)
{
    Fill_u result;
    uint16_t foreground = colour_table[colours & 0xf];
    uint16_t background = colour_table[colours >> 4];

    for (int i = 0; i < 8; ++i)
    {
        result.i16[i] = ((disp << i) & 0x80) ? foreground : background;
    }
    return result;
}
#if (defined PICOZX_LCD)
void core1_main_vga(void)
#else
void core1_main(void)
#endif
{
    scanvideo_setup(video_mode);
    scanvideo_timing_enable(true);
    sem_release(&display_initialised);

    render_loop();
}

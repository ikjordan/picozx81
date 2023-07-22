#include <stdio.h>
#include "pico/stdlib.h"
#include "pico.h"
#include "pico/scanvideo.h"
#include "pico/scanvideo/composable_scanline.h"
#include "pico/multicore.h"
#include "pico/sync.h"
#include "display.h"
#include "zx80bmp.h"
#include "zx81bmp.h"

typedef struct
{
    uint8_t* buff;          // pointer to start of buffer
    uint16_t x;             // offset into line (in bytes) for first pixel to display
    uint16_t y;             // First line to display
    uint16_t stride;        // Number of bytes in display line
} Display_t;

// Unions to allow variable length data manipulation
typedef union
{
    uint16_t i16[256][8];
    uint64_t i64[256][2];
} Look_u;

typedef union
{
    uint16_t i16[8];
    uint32_t i32[4];
    uint64_t i64[2];
} Fill_u;


static const uint16_t PIXEL_WIDTH = 320;
static const uint16_t WIDTH = PIXEL_WIDTH>>3;
static const uint16_t HEIGHT = 240;
static const uint16_t MIN_RUN = 3;

static volatile uint16_t disp_index;
static volatile bool blank = true;
static volatile uint16_t blank_colour = BLACK;

static Display_t disp[2];
static Look_u lookup;
static semaphore_t display_initialised;

static bool showKeyboard = false;
static const KEYBOARD_PIC* keyboard = &ZX81KYBD;
static uint16_t keyboard_x = 0;
static uint16_t keyboard_y = 0;

// Define timing here due to sdk issue with h_pulse length
const scanvideo_timing_t vga_timing_640x480_60_default1 =
    {
        .clock_freq = 25000000,

        .h_active = 640,
        .v_active = 480,

        .h_front_porch = 16,
        .h_pulse = 96,
        .h_total = 800,
        .h_sync_polarity = 1,

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

#define XGA_MODE vga_mode_320x240_60d

//
// Private interface
//

static int32_t populate_line(uint8_t* display_line, uint32_t* buff);
static int32_t populate_blank_line(uint16_t colour, uint32_t* buff);
static int32_t populate_keyboard_line(int linenum, uint16_t bcolour, uint32_t* buff);
static int32_t populate_mixed_line(uint8_t* display_line, int linenum, uint32_t* buff);
static void render_loop();
static void initialise1bppLookup();
static void core1_main();

//
// Public functions
//

void displayInitialise(void)
{
    initialise1bppLookup();
    blank = true;
}

void displayGetCurrent(uint8_t** buff, uint16_t* x_off, uint16_t* y_off, uint16_t* stride)
{
    *buff = disp[disp_index].buff;
    *x_off = disp[disp_index].x;
    *y_off = disp[disp_index].y;
    *stride = disp[1-disp_index].stride;
}

void displaySetBuffer(uint8_t* buff, uint16_t x_off, uint16_t y_off, uint16_t stride)
{
    disp[1-disp_index].buff = buff;
    disp[1-disp_index].x = x_off;
    disp[1-disp_index].y = y_off;
    disp[1-disp_index].stride = stride;
    disp_index = 1 - disp_index;
    blank = false;
}

void displayStart(void)
{
    // create a semaphore to be posted when video init is complete
    sem_init(&display_initialised, 0, 1);

    // launch all the video on core 1, so it isn't affected by USB handling on core 0
    multicore_launch_core1(core1_main);

    sem_acquire_blocking(&display_initialised);
}

void displayShowKeyboard(bool zx81)
{
    keyboard = zx81 ? &ZX81KYBD : &ZX80KYBD;
    keyboard_x = (PIXEL_WIDTH - keyboard->width)>>2;    // Centre (>>1), then 2 pixels per byte (>>1)
    keyboard_y = (HEIGHT - keyboard->height)>>1;
    showKeyboard = true;
}

void displayHideKeyboard(void)
{
    showKeyboard = false;
}

void displayBlank(bool black)
{
    blank_colour = black ? BLACK : WHITE;
    blank = true;
}

bool displayIsBlank(bool* isBlack)
{
    *isBlack = (blank_colour == BLACK);
    return blank;
}

//
// Private functions
//

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

static int32_t __not_in_flash_func(populate_mixed_line)(uint8_t* display_line, int linenum, uint32_t* buff)
{
    // Need to interlace commands with first 2 pixels at start of buffer line
    Fill_u first;

    // Extract the data for the first 8 pixels
    first.i64[0] = lookup.i64[display_line[0]][0];
    first.i64[1] = lookup.i64[display_line[0]][1];

    // interlace the first two
    buff[0] = COMPOSABLE_RAW_RUN          | (first.i16[0] << 16);

    // Note pixel length +1 as have final black pixel
    buff[1] = (PIXEL_WIDTH + 1 - MIN_RUN) | (first.i16[1] << 16);

    // Populate the rest of the pixels in the first byte
    buff[2] = first.i32[1];
    buff[3] = first.i32[2];
    buff[4] = first.i32[3];

    // Process the next 24 pixels
    uint64_t* dest = (uint64_t*)(&buff[5]);
    for (int i=1; i<(keyboard_x>>2); ++i)
    {
        *dest++ = lookup.i64[display_line[i]][0];
        *dest++ = lookup.i64[display_line[i]][1];
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
    dest = (uint64_t*)(&buff[(keyboard->width>>1)+keyboard_x+1]);

    for (int i=WIDTH-(keyboard_x>>2); i<WIDTH; ++i)
    {
        *dest++ = lookup.i64[display_line[i]][0];
        *dest++ = lookup.i64[display_line[i]][1];
    }

    // Must end with a black pixel
    buff[(PIXEL_WIDTH >> 1) + 1] = BLACK | (COMPOSABLE_EOL_ALIGN << 16);
    return (PIXEL_WIDTH >> 1) + 2;
}

static int32_t __not_in_flash_func(populate_line)(uint8_t* display_line, uint32_t* buff)
{
    // Need to interlace commands with first 2 pixels at start of buffer line
    Fill_u first;

    // Extract the data for the first 8 pixels
    first.i64[0] = lookup.i64[display_line[0]][0];
    first.i64[1] = lookup.i64[display_line[0]][1];

    // interlace the first two
    buff[0] = COMPOSABLE_RAW_RUN          | (first.i16[0] << 16);

    // Note pixel length +1 as have final black pixel
    buff[1] = (PIXEL_WIDTH + 1 - MIN_RUN) | (first.i16[1] << 16);

    // Populate the rest of the pixels in the first byte
    buff[2] = first.i32[1];
    buff[3] = first.i32[2];
    buff[4] = first.i32[3];

    // Process the remaining 320/8 - 1 bytes
    uint64_t* dest = (uint64_t*)(&buff[5]);
    for (int i=1; i<WIDTH; ++i)
    {
        *dest++ = lookup.i64[display_line[i]][0];
        *dest++ = lookup.i64[display_line[i]][1];
    }

    // Must end with a black pixel
    buff[(PIXEL_WIDTH >> 1) + 1] = BLACK | (COMPOSABLE_EOL_ALIGN << 16);
    return (PIXEL_WIDTH >> 1) + 2;
}

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
	    unsigned int line_num = scanvideo_scanline_number(buf->scanline_id);                         // The scanline

        uint16_t index = disp_index;    // As disp_index can change at any time
        if (showKeyboard && (line_num >= keyboard_y) && (line_num <(keyboard_y + keyboard->height)))
        {
            if (blank)
            {
                buf->data_used = populate_keyboard_line(line_num, blank_colour, buf->data);
            }
            else
            {
                buf->data_used = populate_mixed_line(&disp[index].buff[disp[index].stride * (line_num + disp[index].y) + disp[index].x],
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
                buf->data_used = populate_line(&disp[index].buff[disp[index].stride * (line_num + disp[index].y) + disp[index].x],
                                               buf->data);
            }
        }
        buf->status = SCANLINE_OK;
        scanvideo_end_scanline_generation(buf);
    }
}

static void initialise1bppLookup()
{
    for (int i=0; i<256; ++i)
    {
        for (int j=0; j<8; ++j)
        {
            // If the bit is set, then the byte will be black
            lookup.i16[i][j] = ((i<<j)&0x80) ? BLACK : WHITE;
        }
    }
}

static void core1_main()
{
    scanvideo_setup(&XGA_MODE);
    scanvideo_timing_enable(true);
    sem_release(&display_initialised);

    render_loop();
}

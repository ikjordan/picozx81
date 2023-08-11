#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "dvi.h"
#include "dvi_serialiser.h"
#include "common_dvi_pin_configs.h"
#include "tmds_double.h"
#include "display.h"
#include "zx80bmp.h"
#include "zx81bmp.h"

#define DVI_TIMING dvi_timing_720x576p_51hz
#define __dvi_const(x) __not_in_flash_func(x)

// Define a slightly higher frame rate, as the ZX81 also produces a display
// approx 0.5% faster than 50%
// The increase in frame rate is achieved by reducing the back porch
// Adjusting the timing to 50 * 625 / (625 - 39 + 31) = 50.65 Hz
const struct dvi_timing __dvi_const(dvi_timing_720x576p_51hz) =
{
    .h_sync_polarity   = false,
    .h_front_porch     = 12,
    .h_sync_width      = 64,
    .h_back_porch      = 68,
    .h_active_pixels   = 720,

    .v_sync_polarity   = false,
    .v_front_porch     = 5,
    .v_sync_width      = 5,
    .v_back_porch      = 31,        // Note that standard is 39
    .v_active_lines    = 576,

    .bit_clk_khz       = 270000
};

static struct dvi_inst dvi0;
static const struct dvi_timing* video_mode = 0;

static uint16_t PIXEL_WIDTH = 0;
static uint16_t HEIGHT = 0;

static bool showKeyboard = false;
static const KEYBOARD_PIC* keyboard = &ZX81KYBD;
static uint16_t keyboard_x = 0;
static uint16_t keyboard_y = 0;
static uint16_t keyboard_right = 0;
static uint16_t keyboard_to_fill = 0;

#include "display_common.c"

//
// Private interface
//

static void render_loop();

//
// Public functions
//

uint displayInitialise(bool fiveSevenSix, uint16_t minBuffByte, uint16_t* pixelWidth,
                       uint16_t* pixelHeight, uint16_t* strideBit)
{
    mutex_init(&next_frame_mutex);

    // Determine the video mode
    video_mode = fiveSevenSix ? &dvi_timing_720x576p_51hz : &dvi_timing_640x480p_60hz;

    PIXEL_WIDTH = video_mode->h_active_pixels >> 1;
    HEIGHT = video_mode->v_active_lines >> 1;
    uint16_t byte_width = PIXEL_WIDTH >> 3;

    // Is padding requested? For DVI need to pad to 4 bytes
    uint16_t startPad = ((minBuffByte + 3) >> 2) << 2;
    uint16_t linePad = (4 - (byte_width % 4)) % 4;

    if (linePad < minBuffByte)
    {
        linePad += (((minBuffByte - linePad) >> 2) + 1) << 2;
    }

    stride = linePad + byte_width;

    // Allocate the buffers
    for (int i=0; i<MAX_FREE; ++i)
    {
        free_buff[i] = (uint8_t*)malloc(startPad + stride * HEIGHT)
                         + startPad;
    }

    free_count = MAX_FREE;

    // Return the values
    *pixelWidth = PIXEL_WIDTH;
    *pixelHeight = HEIGHT;
    *strideBit = stride << 3;

    // Default to blank screen and return clock speed
    blank = true;

    return video_mode->bit_clk_khz;
}

void displayShowKeyboard(bool zx81)
{
    keyboard = zx81 ? &ZX81KYBD : &ZX80KYBD;
    keyboard_x = (PIXEL_WIDTH - keyboard->width)>>1;
    keyboard_y = (HEIGHT - keyboard->height)>>1;
    keyboard_right = (keyboard_x & 0xffe0) + keyboard->width;
    keyboard_to_fill = keyboard_x + (keyboard_x & 0x1f);

    showKeyboard = true;
}

void displayHideKeyboard(void)
{
    showKeyboard = false;
}

//
// Private functions
//
static void __not_in_flash_func(render_loop)()
{
    while (true)
    {
        newFrame();

        // 1 pixel generates 1 word = 4 bytes of tmds
        for (uint y = 0; y < HEIGHT; ++y)
        {
            uint8_t* buff = curr_buff;    // As curr_buff can change at any time
			const uint32_t *linebuf = (const uint32_t*)&buff[stride * y];
			uint32_t *tmdsbuf;
            queue_remove_blocking_u32(&dvi0.q_tmds_free, &tmdsbuf);
            if (showKeyboard && (y >= keyboard_y) && (y <(keyboard_y + keyboard->height)))
            {
                if (blank)
                {
                    uint32_t tm = (blank_colour == BLACK) ? 0x7fd00 : 0xbfe00;

                    // 32 words of blank
                    for (int i=0; i<keyboard_x; i++)
                    {
                        tmdsbuf[i] = tm;
                    }

                    // 256 words of keyboard
                    tmds_double_2bpp((const uint32_t*)&keyboard->pixel_data[(y - keyboard_y) * (keyboard->width>>2)],
                                     &tmdsbuf[keyboard_x],
                                     keyboard->width<<1);

                    // 32 more words of blank
                    for (int i=(PIXEL_WIDTH - keyboard_x); i<PIXEL_WIDTH; i++)
                    {
                        tmdsbuf[i] = tm;
                    }
                }
                else
                {
                    // 32 words of display at 320, 52 words at 360
                    tmds_double_1bpp(linebuf, tmdsbuf, keyboard_x<<1);

                    // Now do the end, as we need to encode from a 32 bit aligned boundary

                    // 32 more words of display at 320, 52 more words at 360
                    // Calculate start in pixels - to get to 32 bit words need to shift 5 times
                    tmds_double_1bpp(&linebuf[keyboard_right>>5], &tmdsbuf[keyboard_right], keyboard_to_fill<<1);

                    // Insert 256 words of keyboard
                    tmds_double_2bpp((const uint32_t*)&keyboard->pixel_data[(y - keyboard_y) * (keyboard->width>>2)],
                                     &tmdsbuf[keyboard_x],
                                     keyboard->width<<1);
                }
            }
            else
            {
                if (blank)
                {
                    uint32_t tm = (blank_colour == BLACK) ? 0x7fd00 : 0xbfe00;

                    for (int i=0; i<PIXEL_WIDTH; i++)
                    {
                        tmdsbuf[i] = tm;
                    }
                }
                else
                {
                    tmds_double_1bpp(linebuf, tmdsbuf, video_mode->h_active_pixels);
                }
            }
            queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
        }
    }
}

static void core1_main()
{
	dvi0.timing = video_mode;
	dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
	dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());
	dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);

	dvi_start(&dvi0);
    sem_release(&display_initialised);

    render_loop();
}

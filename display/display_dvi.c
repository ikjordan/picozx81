#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
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

// Define a slightly higher frame rate, as in normal operation the
// ZX81 also produces a display approximately 1.3% faster than 50 Hz
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

static const KEYBOARD_PIC* keyboard = &ZX81KYBD;
static uint16_t keyboard_x = 0;
static uint16_t keyboard_y = 0;
static uint16_t keyboard_right = 0;
static uint16_t keyboard_to_fill = 0;

#ifdef SOUND_HDMI
static const int hdmi_n[3] = {4096, 6272, 6144};
static uint16_t  rate  = 32000;     // Default audio rate
#define AUDIO_BUFFER_SIZE   (0x1<<8) // Must be power of 2
audio_sample_t      audio_buffer[AUDIO_BUFFER_SIZE];
#endif

#include "display_common.c"

//
// Private interface
//

static void render_loop();

//
// Public functions
//

uint displayInitialise(bool fiveSevenSix, bool match, uint16_t minBuffByte, uint16_t* pixelWidth,
                       uint16_t* pixelHeight, uint16_t* strideBit, DisplayExtraInfo_T* info)
{
#ifndef SOUND_HDMI
    (void)info;
#else
    if (info)
    {
        rate = info->info.hdmi.audioRate;
    }
#endif

    mutex_init(&next_frame_mutex);

    // Determine the video mode
    video_mode = (!fiveSevenSix) ? &dvi_timing_640x480p_60hz : (match) ? &dvi_timing_720x576p_51hz : &dvi_timing_720x576p_50hz;

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

void displayStart(void)
{
    dvi0.timing = video_mode;
    dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
    dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());

    // Grant high bus priority to the second core so display has priority
    hw_set_bits(&bus_ctrl_hw->priority, BUSCTRL_BUS_PRIORITY_PROC1_BITS);

#ifdef SOUND_HDMI
    // HDMI Audio related
    int offset = rate == 48000 ? 2 : (rate == 44100) ? 1 : 0;
    int cts = dvi0.timing->bit_clk_khz*hdmi_n[offset]/(rate/100)/128;
    dvi_get_blank_settings(&dvi0)->top    = 0;
    dvi_get_blank_settings(&dvi0)->bottom = 0;
    dvi_audio_sample_buffer_set(&dvi0, audio_buffer, AUDIO_BUFFER_SIZE);
    dvi_set_audio_freq(&dvi0, rate, cts, hdmi_n[offset]);
    increase_write_pointer(&dvi0.audio_ring,AUDIO_BUFFER_SIZE -1);

    //printf("HP: %u BP: %u Rate: %u CTS: %i\n", video_mode->h_active_pixels, video_mode->v_back_porch, rate, cts);
#endif
    displayStartCommon();
}

bool displayShowKeyboard(bool zx81)
{
    bool previous = showKeyboard;

    if (!showKeyboard)
    {
        keyboard = zx81 ? &ZX81KYBD : &ZX80KYBD;
        keyboard_x = (PIXEL_WIDTH - keyboard->width)>>1;
        keyboard_y = (HEIGHT - keyboard->height)>>1;
        keyboard_right = (keyboard_x & 0xffe0) + keyboard->width;
        keyboard_to_fill = keyboard_x + (keyboard_x & 0x1f);
        showKeyboard = true;
    }
    return previous;
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
    dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);
    dvi_start(&dvi0);
    sem_release(&display_initialised);

    render_loop();
}

#ifdef SOUND_HDMI
void getAudioRing(audio_ring_t** ring)
{
    *ring = &dvi0.audio_ring;
}
#endif


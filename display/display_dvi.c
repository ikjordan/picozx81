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

typedef struct {
    uint8_t* buff;          // pointer to start of buffer
    uint16_t x;             // offset into line (in bytes) for first pixel to display
    uint16_t y;             // First line to display
    uint16_t stride;        // Number of bytes in display line
} Display_t;

static const uint16_t PIXEL_WIDTH = 320;
static const uint16_t FRAME_WIDTH = PIXEL_WIDTH << 1;
static const uint16_t HEIGHT = 240;

static volatile uint16_t disp_index;
static volatile bool blank = true;
static volatile uint16_t blank_colour = BLACK;

static Display_t disp[2];
static semaphore_t display_initialised;

static bool showKeyboard = false;
static const KEYBOARD_PIC* keyboard = &ZX81KYBD;
static uint16_t keyboard_x = 0;
static uint16_t keyboard_y = 0;

static struct dvi_inst dvi0;

#define MODE_640x480_60Hz
#define DVI_TIMING dvi_timing_640x480p_60hz

//
// Private interface
//

static void render_loop();
static void core1_main();

//
// Public functions
//

void displayInitialise(void)
{
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
    keyboard_x = (PIXEL_WIDTH - keyboard->width)>>1;
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
static void __not_in_flash_func(render_loop)()
{
    while (true)
    {
        // 1 pixel generates 1 word = 4 bytes of tmds
        for (uint y = 0; y < HEIGHT; ++y)
        {
            uint16_t index = disp_index;    // As disp_index can change at any time
			const uint32_t *linebuf = (const uint32_t*)&disp[index].buff[disp[index].stride * (y + disp[index].y) + disp[index].x];
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
                    // 32 words of display
                    tmds_double_1bpp(linebuf, tmdsbuf, keyboard_x<<1);

                    // 256 words of keyboard
                    tmds_double_2bpp((const uint32_t*)&keyboard->pixel_data[(y - keyboard_y) * (keyboard->width>>2)],
                                     &tmdsbuf[keyboard_x],
                                     keyboard->width<<1);

                    // 32 more words of display
                    // Calculate start in pixels - to get to words need to shift 5 times
                    tmds_double_1bpp(&linebuf[(keyboard_x + keyboard->width)>>5], &tmdsbuf[keyboard_x + keyboard->width], keyboard_x<<1);
                }
            }
            else
            {
                if (blank)
                {
                    uint32_t tm = (blank_colour == BLACK) ? 0x7fd00 : 0xbfe00;

                    for (int i=0; i<(FRAME_WIDTH >> 1); i++)
                    {
                        tmdsbuf[i] = tm;
                    }
                }
                else
                {
                    tmds_double_1bpp(linebuf, tmdsbuf, FRAME_WIDTH);
                }
            }
            queue_add_blocking_u32(&dvi0.q_tmds_valid, &tmdsbuf);
        }
    }
}

static void core1_main()
{
	dvi0.timing = &DVI_TIMING;
	dvi0.ser_cfg = DVI_DEFAULT_SERIAL_CONFIG;
	dvi_init(&dvi0, next_striped_spin_lock_num(), next_striped_spin_lock_num());
	dvi_register_irqs_this_core(&dvi0, DMA_IRQ_0);

	dvi_start(&dvi0);
    sem_release(&display_initialised);

    render_loop();
}

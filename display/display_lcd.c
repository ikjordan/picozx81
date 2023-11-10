#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "hardware/vreg.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "st7789_lcd.pio.h"

#include "display.h"
#include "zx80bmp.h"
#include "zx81bmp.h"

#define __dvi_const(x) __not_in_flash_func(x)

// To avoid cluttering emuapi.h
extern volatile bool sdAccess;
extern void emu_setLCDPIOSM(PIO pio, uint sm);

#define PIXEL_WIDTH 320
#define HEIGHT      240

static const KEYBOARD_PIC* keyboard = &ZX81KYBD;
static uint16_t keyboard_x = 0;
static uint16_t keyboard_y = 0;
static uint16_t keyboard_right = 0;
static uint16_t keyboard_to_fill = 0;

PIO pio = pio1;
uint sm;
#define SERIAL_CLK_DIV 2.0f

// Format: cmd length (including cmd byte), post delay in units of 5 ms, then cmd payload
// Note the delays have been shortened a little
static const uint8_t st7789_init_seq[] = {
        1, 20, 0x01,                        // Software reset
        1, 10, 0x11,                        // Exit sleep mode
        2, 2, 0x3a, 0x55,                   // Set colour mode to 16 bit
        2, 0, 0x36, 0x60,                   // Set MADCTR: row then column, refresh is bottom to top ????
        5, 0, 0x2a, 0x00, 0x00, PIXEL_WIDTH >> 8, PIXEL_WIDTH & 0xff,   // CASET: column addresses
        5, 0, 0x2b, 0x00, 0x00, HEIGHT >> 8, HEIGHT & 0xff,             // RASET: row addresses
        1, 2, 0x21,                         // Inversion on, then 10 ms delay (supposedly a hack?)
        1, 2, 0x13,                         // Normal display on, then 10 ms delay
        1, 2, 0x29,                         // Main screen turn on, then wait 500 ms
        0                                   // Terminate list
};

#include "display_common.c"

//
// Private interface
//

static void render_loop();

//
// Public functions
//

uint displayInitialise(bool fiveSevenSix, bool match, uint16_t minBuffByte, uint16_t* pixelWidth,
                       uint16_t* pixelHeight, uint16_t* strideBit)
{
    mutex_init(&next_frame_mutex);

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

    return 252000;
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

static inline void lcd_set_dc_cs(bool dc, bool cs)
{
    sleep_us(1);
    gpio_put_masked((1u << PICO_LCD_DC_PIN) | (1u << PICO_LCD_CS_PIN), !!dc << PICO_LCD_DC_PIN | !!cs << PICO_LCD_CS_PIN);
    sleep_us(1);
}

static inline void lcd_write_cmd(PIO pio, uint sm, const uint8_t *cmd, size_t count)
{
    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(0, 0);
    st7789_lcd_put(pio, sm, *cmd++);
    if (count >= 2)
    {
        st7789_lcd_wait_idle(pio, sm);
        lcd_set_dc_cs(1, 0);
        for (size_t i = 0; i < count - 1; ++i)
        {
            st7789_lcd_put(pio, sm, *cmd++);
        }
    }
    st7789_lcd_wait_idle(pio, sm);
    lcd_set_dc_cs(1, 1);
}

static inline void lcd_init(PIO pio, uint sm, const uint8_t *init_seq)
{
    const uint8_t *cmd = init_seq;
    while (*cmd)
    {
        lcd_write_cmd(pio, sm, cmd + 2, *cmd);
        sleep_ms(*(cmd + 1) * 5);
        cmd += *cmd + 2;
    }
}

static inline void st7789_start_pixels(PIO pio, uint sm)
{
    uint8_t cmd = 0x2c; // RAMWR
    lcd_write_cmd(pio, sm, &cmd, 1);
    lcd_set_dc_cs(1, 0);
}

static void __not_in_flash_func(render_loop)()
{
    while (true)
    {
        st7789_lcd_wait_idle(pio, sm);
        lcd_set_dc_cs(1, 1);

        // Wait for new frame semaphore here
        newFrame();

        if (!sdAccess)
        {
            st7789_start_pixels(pio, sm);

            // 1 pixel generates a 16 bit word
            for (uint y = 0; y < HEIGHT; ++y)
            {
                // Check to see if allowed to write
                if (!sdAccess)
                {
                    uint8_t* buff = curr_buff;    // As curr_buff can change at any time
                    const uint8_t *linebuf = &buff[stride * y];

                    if (showKeyboard && (y >= keyboard_y) && (y <(keyboard_y + keyboard->height)))
                    {
                        if (blank)
                        {
                            // 32 pixels of blank
                            for (int i=0; i<keyboard_x; ++i)
                            {
                                st7789_lcd_put(pio, sm, blank_colour >> 8);
                                st7789_lcd_put(pio, sm, blank_colour & 0xff);
                            }

                            // 256 pixels of keyboard, 2 bits per pixel
                            for (int i=0; i<(keyboard->width >> 2); ++i)
                            {
                                uint8_t byte = keyboard->pixel_data[(y - keyboard_y) * (keyboard->width>>2) + i];
                                for (int x=6; x>=0; x-=2)
                                {
                                    uint16_t colour = keyboard->palette[(byte >> x) & 0x03];

                                    st7789_lcd_put(pio, sm, colour >> 8);
                                    st7789_lcd_put(pio, sm, colour & 0xff);
                                }
                            }

                            // 32 more pixels of blank
                            for (int i=(PIXEL_WIDTH - keyboard_x); i<PIXEL_WIDTH; ++i)
                            {
                                st7789_lcd_put(pio, sm, blank_colour >> 8);
                                st7789_lcd_put(pio, sm, blank_colour & 0xff);
                            }
                        }
                        else
                        {
                            // 32 pixels of screen
                            for (int x = 0; x < (keyboard_x >> 3); ++x)
                            {
                                uint8_t byte = linebuf[x];

                                for (int i = 7; i >= 0; --i)
                                {
                                    uint16_t colour = (byte & (0x1 << i)) ? BLACK : WHITE;
                                    st7789_lcd_put(pio, sm, colour >> 8);
                                    st7789_lcd_put(pio, sm, colour & 0xff);
                                }
                            }

                            // 256 pixels of keyboard, 2 bits per pixel
                            for (int i=0; i<(keyboard->width >> 2); ++i)
                            {
                                uint8_t byte = keyboard->pixel_data[(y - keyboard_y) * (keyboard->width>>2) + i];

                                for (int x=6; x>=0; x-=2)
                                {
                                    uint16_t colour = keyboard->palette[(byte >> x) & 0x03];

                                    st7789_lcd_put(pio, sm, colour >> 8);
                                    st7789_lcd_put(pio, sm, colour & 0xff);
                                }
                            }

                            // 32 more pixels of screen
                            for (int x=((PIXEL_WIDTH - keyboard_x) >> 3); x<(PIXEL_WIDTH >> 3); ++x)
                            {
                                uint8_t byte = linebuf[x];

                                for (int i = 7; i >= 0; --i)
                                {
                                    uint16_t colour = (byte & (0x1 << i)) ? BLACK : WHITE;
                                    st7789_lcd_put(pio, sm, colour >> 8);
                                    st7789_lcd_put(pio, sm, colour & 0xff);
                                }
                            }
                        }
                    }
                    else
                    {
                        if (blank)
                        {
                            for (int x=0; x<PIXEL_WIDTH; x++)
                            {
                                st7789_lcd_put(pio, sm, blank_colour >> 8);
                                st7789_lcd_put(pio, sm, blank_colour & 0xff);
                            }
                        }
                        else
                        {
                            for (int x = 0; x < (PIXEL_WIDTH >> 3); ++x)
                            {
                                uint8_t byte = linebuf[x];

                                for (int i = 7; i >= 0; --i)
                                {
                                    uint16_t colour = (byte & (0x1 << i)) ? BLACK : WHITE;
                                    st7789_lcd_put(pio, sm, colour >> 8);
                                    st7789_lcd_put(pio, sm, colour & 0xff);
                                }
                            }
                        }
                    }
                }
                else
                {
                    // Release chip
                    st7789_lcd_wait_idle(pio, sm);
                    lcd_set_dc_cs(1, 1);
                }
            }
        }
    }
}

static void core1_main()
{
    sm = pio_claim_unused_sm(pio, true);    // Will panic if no sm available
    emu_setLCDPIOSM(pio, sm);

    uint offset = pio_add_program(pio, &st7789_lcd_program);
    st7789_lcd_program_init(pio, sm, offset, PICO_SD_CMD_PIN, PICO_SD_CLK_PIN, SERIAL_CLK_DIV);

    gpio_init(PICO_LCD_DC_PIN);
    gpio_init(PICO_LCD_RS_PIN);
    gpio_init(PICO_LCD_BL_PIN);
    gpio_set_dir(PICO_LCD_DC_PIN, GPIO_OUT);
    gpio_set_dir(PICO_LCD_RS_PIN, GPIO_OUT);
    gpio_set_dir(PICO_LCD_BL_PIN, GPIO_OUT);

    gpio_put(PICO_LCD_RS_PIN, 1);
    lcd_init(pio, sm, st7789_init_seq);
    gpio_put(PICO_LCD_BL_PIN, 1);

    sem_release(&display_initialised);

    render_loop();
}

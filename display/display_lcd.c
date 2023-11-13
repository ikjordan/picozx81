#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "st7789_lcd.pio.h"
#include "sdcard.h"

#include "display.h"
#include "zx80bmp.h"
#include "zx81bmp.h"

#define PIXEL_WIDTH 320
#define HEIGHT      240

static const KEYBOARD_PIC* keyboard = &ZX81KYBD;
static uint16_t keyboard_x = 0;
static uint16_t keyboard_y = 0;
static uint16_t keyboard_right = 0;
static uint16_t keyboard_to_fill = 0;

static semaphore_t frame_sync;
static repeating_timer_t timer;
static int32_t period;

static PIO pio = SDCARD_PIO;
static uint sm;
#ifdef PICO_SPI_LCD_SD_SHARE
static volatile bool allowedSPI = true;
#endif

#define SERIAL_CLK_DIV 2.0f
#define CLOCK_SPEED_KHZ 250000

// Format: cmd length (including cmd byte), post delay in units of 5 ms, then cmd payload
// Note the delays have been shortened a little
static const uint8_t st7789_init_seq[] = {
        1, 20, 0x01,                        // Software reset
        1, 10, 0x11,                        // Exit sleep mode
        2, 2, 0x3a, 0x53,                   // Set colour mode to 12 bit
#ifdef PICO_LDC_CONTROLLER_ST7789
        2, 0, 0x36, 0x60,                   // Set MADCTL
#else // ILI9341
        2, 0, 0x36, 0x28,                   // Set MADCTL
#endif
        5, 0, 0x2a, 0x00, 0x00, PIXEL_WIDTH >> 8, PIXEL_WIDTH & 0xff,   // CASET: column addresses
        5, 0, 0x2b, 0x00, 0x00, HEIGHT >> 8, HEIGHT & 0xff,             // RASET: row addresses
#ifdef PICO_LDC_CONTROLLER_ST7789
        1, 2, 0x21,                         // Inversion on, then 10 ms delay
#endif
        1, 2, 0x13,                         // Normal display on, then 10 ms delay
        1, 2, 0x29,                         // Main screen turn on, then wait 10 ms
        0                                   // Terminate list
};

#include "display_common.c"

//
// Private interface
//

static void render_loop();
static bool timer_callback(repeating_timer_t *rt);

//
// Public functions
//

uint displayInitialise(bool fiveSevenSix, bool match, uint16_t minBuffByte, uint16_t* pixelWidth,
                       uint16_t* pixelHeight, uint16_t* strideBit, uint16_t audio_rate)
{
    (void)audio_rate;

    // fiveSevenSix value used to set refresh rate, but not the resolution
    // fiveSevenSix false = 60 Hz
    // fiveSevenSix true & match true = 50.65 Hz
    // fiveSevenSix true & match false = 50 Hz
    if (fiveSevenSix)
    {
        period = match ? 19745 : 20000; //50.65 or 50 Hz
    }
    else
    {
        period = 16667; // 60 Hz
    }

    mutex_init(&next_frame_mutex);

    // Is padding requested? For LCD can pad a single byte
    stride = minBuffByte + (PIXEL_WIDTH >> 3);

        // Allocate the buffers
    for (int i=0; i<MAX_FREE; ++i)
    {
        free_buff[i] = (uint8_t*)malloc(minBuffByte + stride * HEIGHT)
                         + minBuffByte;
    }

    free_count = MAX_FREE;

    // Return the values
    *pixelWidth = PIXEL_WIDTH;
    *pixelHeight = HEIGHT;
    *strideBit = stride << 3;

    // Default to blank screen and return clock speed
    blank = true;

    return CLOCK_SPEED_KHZ;
}

void displayStart(void)
{
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

#ifdef PICO_SPI_LCD_SD_SHARE
// Request SPI bus from the display and wait until available
void displayRequestSPIBus(void)
{
    allowedSPI = false;
    while (!gpio_get_out_level(PICO_LCD_CS_PIN));
    pio_sm_set_enabled(pio, sm, false);
}

// Return the SPI bus to the display
void displayGrantSPIBus(void)
{
    pio_sm_set_enabled(pio, sm, true);
    allowedSPI = true;
}
#endif
//
// Private functions
//

static inline void lcd_set_dc_cs(bool dc, bool cs)
{
    sleep_us(1);
    gpio_put_masked((1u << PICO_LCD_DC_PIN) | (1u << PICO_LCD_CS_PIN), !!dc << PICO_LCD_DC_PIN | !!cs << PICO_LCD_CS_PIN);
    sleep_us(1);
}

static inline void lcd_write_cmd(const uint8_t *cmd, size_t count)
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

static inline void lcd_init(const uint8_t *init_seq)
{
    const uint8_t *cmd = init_seq;
    while (*cmd)
    {
        lcd_write_cmd(cmd + 2, *cmd);
        sleep_ms(*(cmd + 1) * 5);
        cmd += *cmd + 2;
    }
}

static inline void st7789_start_pixels(PIO pio, uint sm)
{
    uint8_t cmd = 0x2c; // RAMWR
    lcd_write_cmd(&cmd, 1);
    lcd_set_dc_cs(1, 0);
}

static void __not_in_flash_func(render_loop)()
{
    st7789_start_pixels(pio, sm);

    while (true)
    {
        sem_acquire_blocking(&frame_sync);
        newFrame();

#ifdef PICO_SPI_LCD_SD_SHARE
        // Check to see if allowed to access SPI bus
        if (allowedSPI)
        {
            // Ensure LCD has bus
            gpio_put(PICO_LCD_CS_PIN, 0);
#endif
            // 1 pixel generates a 12 bit word - so 2 pixels are 3 bytes
            for (uint y = 0; y < HEIGHT; ++y)
            {
                uint8_t* buff = curr_buff;    // As curr_buff can change at any time
                const uint8_t *linebuf = &buff[stride * y];

                if (showKeyboard && (y >= keyboard_y) && (y <(keyboard_y + keyboard->height)))
                {
                    if (blank)
                    {
                        // 32 pixels of blank
                        uint32_t twoblank = (blank_colour << 12) | blank_colour;

                        for (int i=0; i<(keyboard_x>>1); ++i)
                        {
                            st7789_lcd_put(pio, sm, (twoblank >> 16) & 0xff);
                            st7789_lcd_put(pio, sm, (twoblank >> 8) & 0xff);
                            st7789_lcd_put(pio, sm, twoblank & 0xff);
                        }

                        // 256 pixels of keyboard, 2 bits per pixel
                        for (int i=0; i<(keyboard->width >> 2); ++i)
                        {
                            uint8_t byte = keyboard->pixel_data[(y - keyboard_y) * (keyboard->width>>2) + i];
                            for (int x=6; x>=2; x-=4)
                            {
                                uint32_t colour = (keyboard->palette[(byte >> x) & 0x03] << 12) +
                                                   keyboard->palette[(byte >> (x-2)) & 0x03];

                                st7789_lcd_put(pio, sm, (colour >> 16) & 0xff);
                                st7789_lcd_put(pio, sm, (colour >> 8) & 0xff);
                                st7789_lcd_put(pio, sm, colour & 0xff);
                            }
                        }

                        // 32 more pixels of blank
                        for (int i=((PIXEL_WIDTH - keyboard_x)>>1); i<(PIXEL_WIDTH>>1); ++i)
                        {
                            st7789_lcd_put(pio, sm, (twoblank >> 16) & 0xff);
                            st7789_lcd_put(pio, sm, (twoblank >> 8) & 0xff);
                            st7789_lcd_put(pio, sm, twoblank & 0xff);
                        }
                    }
                    else
                    {
                        // 32 pixels of screen
                        for (int x = 0; x < (keyboard_x >> 3); ++x)
                        {
                            uint8_t byte = linebuf[x];

                            int count = 7;

                            for (int j=0; j<4; j++)
                            {
                                uint32_t twobits = (byte & (0x1 << count--)) ? BLACK : WHITE;
                                twobits = twobits << 12;
                                twobits |= (byte & (0x1 << count--)) ? BLACK : WHITE;

                                st7789_lcd_put(pio, sm, (twobits >> 16) & 0xff);
                                st7789_lcd_put(pio, sm, (twobits >> 8) & 0xff);
                                st7789_lcd_put(pio, sm, twobits & 0xff);
                            }
                        }

                        // 256 pixels of keyboard, 2 bits per pixel
                        for (int i=0; i<(keyboard->width >> 2); ++i)
                        {
                            uint8_t byte = keyboard->pixel_data[(y - keyboard_y) * (keyboard->width>>2) + i];
                            for (int x=6; x>=2; x-=4)
                            {
                                uint32_t colour = (keyboard->palette[(byte >> x) & 0x03] << 12) +
                                                   keyboard->palette[(byte >> (x-2)) & 0x03];

                                st7789_lcd_put(pio, sm, (colour >> 16) & 0xff);
                                st7789_lcd_put(pio, sm, (colour >> 8) & 0xff);
                                st7789_lcd_put(pio, sm, colour & 0xff);
                            }
                        }

                        // 32 more pixels of screen
                        for (int x=((PIXEL_WIDTH - keyboard_x) >> 3); x<(PIXEL_WIDTH >> 3); ++x)
                        {
                            uint8_t byte = linebuf[x];

                            int count = 7;

                            for (int j=0; j<4; j++)
                            {
                                uint32_t twobits = (byte & (0x1 << count--)) ? BLACK : WHITE;
                                twobits = twobits << 12;
                                twobits |= (byte & (0x1 << count--)) ? BLACK : WHITE;

                                st7789_lcd_put(pio, sm, (twobits >> 16) & 0xff);
                                st7789_lcd_put(pio, sm, (twobits >> 8) & 0xff);
                                st7789_lcd_put(pio, sm, twobits & 0xff);
                            }
                        }
                    }
                }
                else
                {
                    if (blank)
                    {
                        uint32_t twobits = (blank_colour << 12) | blank_colour;

                        for (int x=0; (x<PIXEL_WIDTH>>1); x++)
                        {
                            st7789_lcd_put(pio, sm, (twobits >> 16) & 0xff);
                            st7789_lcd_put(pio, sm, (twobits >> 8) & 0xff);
                            st7789_lcd_put(pio, sm, twobits & 0xff);
                        }
                    }
                    else
                    {
                        for (int x = 0; x < (PIXEL_WIDTH >> 3); ++x)
                        {
                            uint8_t byte = linebuf[x];

                            int count = 7;

                            for (int j=0; j<4; j++)
                            {
                                uint32_t twobits = (byte & (0x1 << count--)) ? BLACK : WHITE;
                                twobits = twobits << 12;
                                twobits |= (byte & (0x1 << count--)) ? BLACK : WHITE;

                                st7789_lcd_put(pio, sm, (twobits >> 16) & 0xff);
                                st7789_lcd_put(pio, sm, (twobits >> 8) & 0xff);
                                st7789_lcd_put(pio, sm, twobits & 0xff);
                            }
                        }
                    }
                }
            }
#ifdef PICO_SPI_LCD_SD_SHARE
        }
        else
        {
            // Release SPI bus
            st7789_lcd_wait_idle(pio, sm);
            lcd_set_dc_cs(1, 1);
        }
#endif
    }
}

static void core1_main()
{
    sm = pio_claim_unused_sm(pio, true);    // Will panic if no sm available

    sem_init(&frame_sync, 0 , 1);

    // Set a timer to drive frame rate
    if (!add_repeating_timer_us((-period), timer_callback, NULL, &timer))
    {
        printf("Failed to add timer\n");
        exit(-1);
    }

    uint offset = pio_add_program(pio, &st7789_lcd_program);
#ifndef PICO_LCD_CLK_PIN
    st7789_lcd_program_init(pio, sm, offset, PICO_SD_CMD_PIN, PICO_SD_CLK_PIN, SERIAL_CLK_DIV);
#else
    gpio_init(PICO_LCD_CMD_PIN);
    gpio_init(PICO_LCD_CLK_PIN);
    gpio_set_dir(PICO_LCD_CMD_PIN, GPIO_OUT);
    gpio_set_dir(PICO_LCD_CLK_PIN, GPIO_OUT);
    st7789_lcd_program_init(pio, sm, offset, PICO_LCD_CMD_PIN, PICO_LCD_CLK_PIN, SERIAL_CLK_DIV);
#endif
    gpio_init(PICO_LCD_DC_PIN);
    gpio_init(PICO_LCD_RS_PIN);
    gpio_init(PICO_LCD_BL_PIN);
    gpio_set_dir(PICO_LCD_DC_PIN, GPIO_OUT);
    gpio_set_dir(PICO_LCD_RS_PIN, GPIO_OUT);
    gpio_set_dir(PICO_LCD_BL_PIN, GPIO_OUT);

    gpio_put(PICO_LCD_RS_PIN, 1);
    lcd_init(st7789_init_seq);
    gpio_put(PICO_LCD_BL_PIN, 1);

    sem_release(&display_initialised);

    render_loop();
}

static bool timer_callback(repeating_timer_t *rt)
{
    // Trigger new frame
    sem_release(&frame_sync);
    return true;
}

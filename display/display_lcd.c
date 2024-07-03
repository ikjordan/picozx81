#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "hardware/irq.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "spi_lcd.pio.h"
#include "sdcard.h"

#include "display.h"
#include "display_priv.h"

#define PIXEL_WIDTH 320
#define HEIGHT      240

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

static bool invert = false;     // Invert colours on display
static bool skip = false;       // Skip every other frame
static bool rotate = false;     // Rotate display by 180 degrees
static bool reflect = false;    // Change horizontal scan direction
static bool bgr = false;        // use bgr instead of rgb

static uint16_t stride;

// Defined here to allow for case where VGA and LCD on same hardware
#define BLACK_LCD  0x0000
#define BLUE_LCD   0x000f
#define RED_LCD    0x0f00
#define YELLOW_LCD 0x0ff0
#define WHITE_LCD  0x0fff

// Do not make const - as want to keep in RAM
static uint16_t colour_table[16] = {
    BLACK_LCD,
    0x000a,
    0x0a00,
    0x0a0a,
    0x00a0,
    0x00aa,
    0x0aa0,
    0x0aaa,
    BLACK_LCD,
    BLUE_LCD,
    RED_LCD,
    0x0f0f,
    0x00f0,
    0x00ff,
    YELLOW_LCD,
    WHITE_LCD
};

//
// Private interface
//

static void render_loop();
static bool timer_callback(repeating_timer_t *rt);
static inline void lcd_set_dc_cs(bool dc, bool cs);
static inline void lcd_write_cmd(const uint8_t cmd, const uint8_t *data, size_t count, size_t delay);
static inline void lcd_init(void);
static inline void lcd_start_pixels(void);

//
// Public functions
//
#include <stdio.h>

#ifndef PICOZX_LCD
uint displayInitialise
#else
bool useLCD = false;
uint displayInitialiseLCD
#endif
                      (bool fiveSevenSix, bool match, uint16_t minBuffByte, uint16_t* pixelWidth,
                       uint16_t* pixelHeight, uint16_t* strideBit, DisplayExtraInfo_T* info)
{
    if (info)
    {
        invert = info->info.lcd.invertColour;
        skip = info->info.lcd.skipFrame;
        rotate = info->info.lcd.rotate;
        reflect = info->info.lcd.reflect;
        bgr = info->info.lcd.bgr;
        reflect = rotate ? !reflect : reflect;
    }

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

    if (skip) period <<= 1;

    mutex_init(&next_frame_mutex);

    // Is padding requested? For LCD can pad a single byte
    stride = minBuffByte + (PIXEL_WIDTH >> 3);

    // Allocate the buffers
    displayAllocateBuffers(minBuffByte, stride, HEIGHT);

    // Return the values
    *pixelWidth = PIXEL_WIDTH;
    *pixelHeight = HEIGHT;
    *strideBit = stride << 3;

    // Default to blank screen and return clock speed
    blank = true;

    return CLOCK_SPEED_KHZ;
}

#ifndef PICOZX_LCD
void displayStart(void)
#else
void displayStartLCD(void)
#endif
{
    printf("Invert %s\n", invert ? "True": "false");
    printf("Skip %s\n", skip ? "True": "false");
    printf("Rotate %s\n", rotate ? "True": "false");
    printf("Reflect %s\n", reflect ? "True": "false");
    printf("BGR %s\n", bgr ? "True": "false");

    displayStartCommon();
}

#ifndef PICOZX_LCD
bool displayShowKeyboard(bool ROM8K)
#else
bool displayShowKeyboardLCD(bool ROM8K)
#endif
{
    bool previous = showKeyboard;

    if (!showKeyboard)
    {
#ifdef PICOZX_LCD
        if (useLCD)
        {
            keyboard = ROM8K ? &ZX81KYBD_LCD : &ZX80KYBD_LCD;
        }
        else
#endif
        {
            keyboard = ROM8K ? &ZX81KYBD : &ZX80KYBD;
        }
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

static inline void lcd_write_cmd(const uint8_t cmd, const uint8_t *data, size_t count, size_t delay)
{
    spi_lcd_wait_idle();
    lcd_set_dc_cs(0, 0);
    spi_lcd_put(cmd);
    if (count)
    {
        spi_lcd_wait_idle();
        lcd_set_dc_cs(1, 0);
        for (size_t i = 0; i < count; ++i)
        {
            spi_lcd_put(data[i]);
        }
    }
    spi_lcd_wait_idle();
    lcd_set_dc_cs(1, 1);
    sleep_ms(delay);
}

static inline void lcd_init(void)
{
    // Iterate through initialisation sequence
    uint8_t data[4];

    // reset
    lcd_write_cmd(0x01, NULL, 0, 100);

    // Exit sleep mode
    lcd_write_cmd(0x11, NULL, 0, 50);

    // Set colour mode to 12 bits
    data[0] = 0x33;
    lcd_write_cmd(0x3a, data, 1, 10);

    // Set scan order
    data[0] = 0x20;
    data[0] |= bgr ? 0x8 : 0;
    data[0] |= rotate ? 0x40 : 0;
    data[0] |= reflect ? 0x80 : 0;
    lcd_write_cmd(0x36, data, 1, 10);

    // Column size
    data[0] = 0x0;
    data[1] = 0x0;
    data[2] = PIXEL_WIDTH >> 8;
    data[3] = PIXEL_WIDTH & 0xff;
    lcd_write_cmd(0x2a, data, 4, 0);

    // Row size
    data[0] = 0x0;
    data[1] = 0x0;
    data[2] = HEIGHT >> 8;
    data[3] = HEIGHT & 0xff;
    lcd_write_cmd(0x2b, data, 4, 0);

    if (invert)
    {
        lcd_write_cmd(0x21, NULL, 0, 10);
    }

    // Display on
    lcd_write_cmd(0x13, NULL, 0, 10);
    lcd_write_cmd(0x29, NULL, 0, 10);
}

static inline void lcd_start_pixels(void)
{
    lcd_write_cmd(0x2C, NULL, 0, 0);    // RAMWR
    lcd_set_dc_cs(1, 0);
}

static void __not_in_flash_func(render_loop)()
{
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
            lcd_start_pixels();

            // 1 pixel generates a 12 bit word - so 2 pixels are 3 bytes
            for (uint y = 0; y < HEIGHT; ++y)
            {
                uint8_t* buff = curr_buff;    // As curr_buff can change at any time
#ifdef SUPPORT_CHROMA
                uint8_t* cbuff = cbuffer;
#else
                uint8_t* cbuff = 0;
#endif
                uint8_t* linebuf = &buff[stride * y];
                uint8_t* clinebuf = cbuff ? &cbuff[stride * y] : 0;

                if (showKeyboard && (y >= keyboard_y) && (y <(keyboard_y + keyboard->height)))
                {
                    if (blank)
                    {
                        // 32 pixels of blank
#ifndef PICOZX_LCD
                        uint32_t twoblank = (blank_colour << 12) | blank_colour;
#else
                        uint32_t twoblank = (blank_colour == WHITE) ? (WHITE_LCD << 12) | WHITE_LCD : (BLACK_LCD << 12) | BLACK_LCD;
#endif
                        for (int i=0; i<(keyboard_x>>1); ++i)
                        {
                            spi_lcd_put((twoblank >> 16) & 0xff);
                            spi_lcd_put((twoblank >> 8) & 0xff);
                            spi_lcd_put(twoblank & 0xff);
                        }

                        // 256 pixels of keyboard, 2 bits per pixel
                        for (int i=0; i<(keyboard->width >> 2); ++i)
                        {
                            uint8_t byte = keyboard->pixel_data[(y - keyboard_y) * (keyboard->width>>2) + i];
                            for (int x=6; x>=2; x-=4)
                            {
                                uint32_t colour = (keyboard->palette[(byte >> x) & 0x03] << 12) +
                                                   keyboard->palette[(byte >> (x-2)) & 0x03];

                                spi_lcd_put((colour >> 16) & 0xff);
                                spi_lcd_put((colour >> 8) & 0xff);
                                spi_lcd_put(colour & 0xff);
                            }
                        }

                        // 32 more pixels of blank
                        for (int i=((PIXEL_WIDTH - keyboard_x)>>1); i<(PIXEL_WIDTH>>1); ++i)
                        {
                            spi_lcd_put((twoblank >> 16) & 0xff);
                            spi_lcd_put((twoblank >> 8) & 0xff);
                            spi_lcd_put(twoblank & 0xff);
                        }
                    }
                    else
                    {
                        // 32 pixels of screen
                        for (int x = 0; x < (keyboard_x >> 3); ++x)
                        {
                            uint8_t byte = linebuf[x];
                            uint16_t foreground = cbuff ? colour_table[clinebuf[x] & 0xf] : BLACK_LCD;
                            uint16_t background = cbuff ? colour_table[clinebuf[x] >> 4] : WHITE_LCD;

                            int count = 7;

                            for (int j=0; j<4; j++)
                            {
                                uint32_t twobits = (byte & (0x1 << count--)) ? foreground : background;
                                twobits = twobits << 12;
                                twobits |= (byte & (0x1 << count--)) ? foreground : background;

                                spi_lcd_put((twobits >> 16) & 0xff);
                                spi_lcd_put((twobits >> 8) & 0xff);
                                spi_lcd_put(twobits & 0xff);
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

                                spi_lcd_put((colour >> 16) & 0xff);
                                spi_lcd_put((colour >> 8) & 0xff);
                                spi_lcd_put(colour & 0xff);
                            }
                        }

                        // 32 more pixels of screen
                        for (int x=((PIXEL_WIDTH - keyboard_x) >> 3); x<(PIXEL_WIDTH >> 3); ++x)
                        {
                            uint8_t byte = linebuf[x];
                            uint16_t foreground = cbuff ? colour_table[clinebuf[x] & 0xf] : BLACK_LCD;
                            uint16_t background = cbuff ? colour_table[clinebuf[x] >> 4] : WHITE_LCD;

                            int count = 7;

                            for (int j=0; j<4; j++)
                            {
                                uint32_t twobits = (byte & (0x1 << count--)) ? foreground : background;
                                twobits = twobits << 12;
                                twobits |= (byte & (0x1 << count--)) ? foreground : background;

                                spi_lcd_put((twobits >> 16) & 0xff);
                                spi_lcd_put((twobits >> 8) & 0xff);
                                spi_lcd_put(twobits & 0xff);
                            }
                        }
                    }
                }
                else
                {
                    if (blank)
                    {
#ifndef PICOZX_LCD
                        uint32_t twobits = (blank_colour << 12) | blank_colour;
#else
                        uint32_t twobits = (blank_colour == WHITE) ? (WHITE_LCD << 12) | WHITE_LCD : (BLACK_LCD << 12) | BLACK_LCD;
#endif
                        for (int x=0; (x<PIXEL_WIDTH>>1); x++)
                        {
                            spi_lcd_put((twobits >> 16) & 0xff);
                            spi_lcd_put((twobits >> 8) & 0xff);
                            spi_lcd_put(twobits & 0xff);
                        }
                    }
                    else
                    {
                        // Two bits of screen data gives 24 bits of ldc data = 3 bytes
                        for (int x = 0; x < (PIXEL_WIDTH >> 3); ++x)
                        {
                            uint8_t byte = linebuf[x];
                            uint16_t foreground = cbuff ? colour_table[clinebuf[x] & 0xf] : BLACK_LCD;
                            uint16_t background = cbuff ? colour_table[clinebuf[x] >> 4] : WHITE_LCD;
                            int count = 7;

                            for (int j=0; j<4; j++)
                            {
                                uint32_t twobits = (byte & (0x1 << count--)) ? foreground : background;
                                twobits = twobits << 12;
                                twobits |= (byte & (0x1 << count--)) ? foreground : background;

                                spi_lcd_put((twobits >> 16) & 0xff);
                                spi_lcd_put((twobits >> 8) & 0xff);
                                spi_lcd_put(twobits & 0xff);
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
            spi_lcd_wait_idle();
            lcd_set_dc_cs(1, 1);
        }
#endif
    }
}

#if (defined PICOZX_LCD)
void core1_main_lcd(void)
#else
void core1_main(void)
#endif
{
    sm = pio_claim_unused_sm(pio, true);    // Will panic if no sm available

    sem_init(&frame_sync, 0 , 1);

    // Set a timer to drive frame rate
    if (!add_repeating_timer_us((-period), timer_callback, NULL, &timer))
    {
        printf("Failed to add timer\n");
        exit(-1);
    }

    uint offset = pio_add_program(pio, &spi_lcd_program);
#ifndef PICO_LCD_CLK_PIN
    spi_lcd_program_init(pio, sm, offset, PICO_SD_CMD_PIN, PICO_SD_CLK_PIN, (SERIAL_CLK_DIV * (skip ? 2: 1)));
#else
    gpio_init(PICO_LCD_CMD_PIN);
    gpio_init(PICO_LCD_CLK_PIN);
    gpio_set_dir(PICO_LCD_CMD_PIN, GPIO_OUT);
    gpio_set_dir(PICO_LCD_CLK_PIN, GPIO_OUT);
    spi_lcd_program_init(pio, sm, offset, PICO_LCD_CMD_PIN, PICO_LCD_CLK_PIN, (SERIAL_CLK_DIV * (skip ? 2: 1)));
#endif
    gpio_init(PICO_LCD_DC_PIN);
#ifdef PICO_LCD_RS_PIN
    gpio_init(PICO_LCD_RS_PIN);
#endif
    gpio_init(PICO_LCD_BL_PIN);
    gpio_set_dir(PICO_LCD_DC_PIN, GPIO_OUT);

#ifdef PICO_LCD_RS_PIN
    gpio_set_dir(PICO_LCD_RS_PIN, GPIO_OUT);
#endif
    gpio_set_dir(PICO_LCD_BL_PIN, GPIO_OUT);

#ifdef PICO_LCD_RS_PIN
    gpio_put(PICO_LCD_RS_PIN, 1);
#endif
    lcd_init();
    gpio_put(PICO_LCD_BL_PIN, 1);

    sem_release(&display_initialised);

    render_loop();
}

static bool  __not_in_flash_func(timer_callback)(repeating_timer_t *rt)
{
    // Trigger new frame
    sem_release(&frame_sync);
    return true;
}

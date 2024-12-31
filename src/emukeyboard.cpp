#include "pico.h"
#include <stdlib.h>
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "tusb.h"
#include "hid_usb.h"
#include "emuapi.h"
#include "emukeyboard.h"

#if ((defined PICO_PICOZX_BOARD) || (defined PICO_PICOZXREAL_BOARD))
#include "hardware/clocks.h"

#if (defined PICO_PICOZXREAL_BOARD)
#include "pico/bootrom.h"
#endif

static void gp_keyboard_initialise(void);
static inline void device_keyscan_row(void);
static bool timer_callback(repeating_timer_t *rt);

static repeating_timer_t timer;
static int32_t period = TP;

static uint8_t cp[] = {CP};                      // Column pins
static uint8_t rp[] = {RP};                      // Row pins
static uint8_t rs[RN];                           // Last result for row

static uint8_t kbits[2][RN][CN] =
    // Keyboard mappings
#if defined PICO_PICOZX_BOARD
    {
        {
            { HID_KEY_ENTER, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_UP, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_DOWN, 0, 0 }, // shift & period at end
            { HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_6, HID_KEY_7 },
            { HID_KEY_8, HID_KEY_9, HID_KEY_0, HID_KEY_Q, HID_KEY_W, HID_KEY_E, HID_KEY_R },
            { HID_KEY_T, HID_KEY_Y, HID_KEY_U, HID_KEY_I, HID_KEY_O, HID_KEY_P, HID_KEY_A },
            { HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_G, HID_KEY_H, HID_KEY_J, HID_KEY_K },
            { HID_KEY_L, HID_KEY_ENTER, HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B },
            { HID_KEY_N, HID_KEY_M, HID_KEY_SPACE, HID_KEY_F1, HID_KEY_F2, HID_KEY_F3, HID_KEY_F4 },
        },
        {
            { HID_KEY_ENTER, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_UP, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_DOWN, 0, 0 }, // shift & period at end
            { HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_6, HID_KEY_7 },
            { HID_KEY_8, HID_KEY_9, HID_KEY_0, HID_KEY_Q, HID_KEY_W, HID_KEY_E, HID_KEY_R },
            { HID_KEY_T, HID_KEY_Y, HID_KEY_U, HID_KEY_I, HID_KEY_O, HID_KEY_P, HID_KEY_A },
            { HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_G, HID_KEY_H, HID_KEY_J, HID_KEY_K },
            { HID_KEY_L, HID_KEY_ENTER, HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B },
            { HID_KEY_N, HID_KEY_M, HID_KEY_SPACE, HID_KEY_F5, HID_KEY_F6, HID_KEY_F7, HID_KEY_F8 },
        }
    };
#else
    {
        {
            { HID_KEY_ENTER, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_UP, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_DOWN, HID_KEY_F5, HID_KEY_F2, 0 },
            { HID_KEY_B, HID_KEY_H, HID_KEY_V, HID_KEY_Y, HID_KEY_6, HID_KEY_G, HID_KEY_T, HID_KEY_5 },
            { HID_KEY_N, HID_KEY_J, HID_KEY_C, HID_KEY_U, HID_KEY_7, HID_KEY_F, HID_KEY_R, HID_KEY_4 },
            { HID_KEY_M, HID_KEY_K, HID_KEY_X, HID_KEY_I, HID_KEY_8, HID_KEY_D, HID_KEY_E, HID_KEY_3 },
            { HID_KEY_PERIOD, HID_KEY_L, HID_KEY_Z, HID_KEY_O, HID_KEY_9, HID_KEY_S, HID_KEY_W, HID_KEY_2 },
            { HID_KEY_SPACE, HID_KEY_ENTER, 0, HID_KEY_P, HID_KEY_0, HID_KEY_A, HID_KEY_Q, HID_KEY_1 }
        },
        {
            { HID_KEY_ENTER, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_UP, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_DOWN, HID_KEY_F3, HID_KEY_F6, 0 },
            { HID_KEY_B, HID_KEY_H, HID_KEY_V, HID_KEY_Y, HID_KEY_6, HID_KEY_G, HID_KEY_T, HID_KEY_5 },
            { HID_KEY_N, HID_KEY_J, HID_KEY_C, HID_KEY_U, HID_KEY_7, HID_KEY_F, HID_KEY_R, HID_KEY_4 },
            { HID_KEY_M, HID_KEY_K, HID_KEY_X, HID_KEY_I, HID_KEY_8, HID_KEY_D, HID_KEY_E, HID_KEY_3 },
            { HID_KEY_PERIOD, HID_KEY_L, HID_KEY_Z, HID_KEY_O, HID_KEY_9, HID_KEY_S, HID_KEY_W, HID_KEY_2 },
            { HID_KEY_SPACE, HID_KEY_ENTER, 0, HID_KEY_P, HID_KEY_0, HID_KEY_A, HID_KEY_Q, HID_KEY_1 }
        }
    };
#endif
#endif

void emu_KeyboardInitialise(uint8_t* keyboard)
{
    hidInitialise(keyboard);

#if ((defined PICO_PICOZX_BOARD) || (defined PICO_PICOZXREAL_BOARD))
    static bool first = true;

    gp_keyboard_initialise();

    if (first)
    {
        // Only initialise the device keyboard timer once
        first = false;

        // if a read is missed, no need to catch up, so use positive time
        if (!add_repeating_timer_us(period, timer_callback, NULL, &timer))
        {
            printf("Failed to add timer\n");
            exit(-1);
        }
    }
#endif
}

void emu_KeyboardScan(void* data)
{
#if ((defined PICO_PICOZX_BOARD) || (defined PICO_PICOZXREAL_BOARD))
    hid_keyboard_report_t* report = (hid_keyboard_report_t*)data;
    int set = 0;
#if (defined PICO_PICOZX_BOARD)
    // parse, first row
    // Handle shift
    if (rs[0] & 0x20)
    {
        report->modifier |= 0x22;
        set = 1;
    }

    // Determine how many key slots left
    int used = 0;
    while (report->keycode[used] != HID_KEY_NONE)
    {
        if (++used == 6) return;
    }

    // Check main keys attached to row 0
    if (rs[0] & 0x40)
    {
        report->keycode[used] = HID_KEY_PERIOD;
        if (++used == 6) return;
    }

    // Process remaining rows
    for (int i=1; i<RN; ++i)
    {
        for (int j=0; j<CN; ++j)
        {
            if (rs[i] & (0x1<<j))
            {
                report->keycode[used] = kbits[set][i][j];
                if (++used == 6) return;
            }
        }
    }
#else
    // Determine shift state
    if (rs[5] & 0x04)
    {
        report->modifier |= 0x22;
        set = 1;
    }

    // Determine how many key slots left
    int used = 0;
    while (report->keycode[used] != HID_KEY_NONE)
    {
        if (++used == 6) return;
    }

    // Handle row 0 menu keys
    for (int j=5; j<CN; ++j)
    {
        if (rs[0] & (0x1<<j))
        {
            report->keycode[used] = kbits[set][0][j];
            if (++used == 6) return;
        }

        // Check to see if menu button has been pressed, together with R key
        if ((rs[0] & 0x40) && (rs[2] & 0x40))
        {
            // Reset the board, so new firmware can be loaded
            reset_usb_boot(0, 0);
        }
    }

    // Process remaining rows - have to skip shift
    for (int i=1; i<RN; ++i)
    {
        for (int j=0; j<CN; ++j)
        {
            if ((rs[i] & (0x1<<j)) && (kbits[set][i][j] !=0))
            {
                report->keycode[used] = kbits[set][i][j];
                if (++used == 6) return;
            }
        }
    }
#endif
#else
    (void)(data);
#endif
}

#if ((defined PICO_PICOZX_BOARD) || (defined PICO_PICOZXREAL_BOARD))
bool emu_KeyboardFire(void)
{
    // Ensure keyboard and joypad is initialised
    gp_keyboard_initialise();

    // Read the first line, which contains the fire button
    device_keyscan_row();

    return ((rs[0] & 0x01) != 0);
}

static void gp_keyboard_initialise(void)
{
    static bool first = true;
    if (first)
    {
        // Only initialise the device keyboard once
        first = false;
        for(int i = 0; i < RN; ++i)
        {
            gpio_init(rp[i]);
            gpio_set_dir(rp[i], GPIO_IN);
            gpio_disable_pulls(rp[i]);
        }

        for(int i = 0; i < CN; ++i)
        {
            gpio_init(cp[i]);
            gpio_set_dir(cp[i], GPIO_IN);
            gpio_pull_up(cp[i]);
        }

        // Set the first row to output, ready for first read
        gpio_set_dir(rp[0], GPIO_OUT);
        gpio_put(rp[0], 0);
    }
}

static inline void  __not_in_flash_func(device_keyscan_row)(void)
{
    static uint32_t ri = 0;

    uint32_t a = ~(gpio_get_all() >> CP_SHIFT);
    rs[ri] = CP_JOIN(a);

    // Revert line read
    gpio_set_dir(rp[ri], GPIO_IN);
    gpio_disable_pulls(rp[ri]);

    if (++ri >= RN)
    {
        ri = 0;
    }

    // Prepare next line
    gpio_set_dir(rp[ri], GPIO_OUT);
    gpio_put(rp[ri], 0);
}

static bool  __not_in_flash_func(timer_callback)(repeating_timer_t *rt)
{
    device_keyscan_row();
    return true;
}
#endif

bool emu_KeyboardUpdate(uint8_t* special)
{
  bool ret = hidReadUsbKeyboard(special, emu_DoubleShiftRequested());

  if (!ret)
  {
#if ((defined PICO_PICOZX_BOARD) || (defined PICO_PICOZXREAL_BOARD))    // Handle device joystick
    emu_JoystickDeviceParse(((rs[0] & 0x04) != 0),
                            ((rs[0] & 0x10) != 0),
                            ((rs[0] & 0x02) != 0),
                            ((rs[0] & 0x08) != 0),
                            ((rs[0] & 0x01) != 0));
#endif
    emu_JoystickParse();
  }
  return ret;
}

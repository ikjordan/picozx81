#include "pico.h"
#include <stdlib.h>
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>
#include "tusb.h"
#include "hid_usb.h"
#include "emuapi.h"
#include "emukeyboard.h"

#ifdef PICO_PICOZX_BOARD
static void device_keyscan_row(void);
static void reset_keyscan_row(void);

static uint8_t cp[] = {CP};                      // Column pins
static uint8_t rp[] = {RP};                      // Row pins
static uint8_t rs[RN];                           // Last result for row

static uint8_t kbits[7][7] =
  // Normal mappings
  {
    { HID_KEY_ENTER, HID_KEY_ARROW_LEFT, HID_KEY_ARROW_UP, HID_KEY_ARROW_RIGHT, HID_KEY_ARROW_DOWN, 0, 0 },
    { HID_KEY_1, HID_KEY_2, HID_KEY_3, HID_KEY_4, HID_KEY_5, HID_KEY_6, HID_KEY_7 },
    { HID_KEY_8, HID_KEY_9, HID_KEY_0, HID_KEY_Q, HID_KEY_W, HID_KEY_E, HID_KEY_R },
    { HID_KEY_T, HID_KEY_Y, HID_KEY_U, HID_KEY_I, HID_KEY_O, HID_KEY_P, HID_KEY_A },
    { HID_KEY_S, HID_KEY_D, HID_KEY_F, HID_KEY_G, HID_KEY_H, HID_KEY_J, HID_KEY_K },
    { HID_KEY_L, HID_KEY_ENTER, HID_KEY_Z, HID_KEY_X, HID_KEY_C, HID_KEY_V, HID_KEY_B },
    { HID_KEY_N, HID_KEY_M, HID_KEY_SPACE, HID_KEY_F2, HID_KEY_F1, HID_KEY_F3, HID_KEY_F5 },
  };

#endif

void emu_KeyboardInitialise(uint8_t* keyboard)
{
    hidInitialise(keyboard);

#ifdef PICO_PICOZX_BOARD
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
    uint32_t row = rp[0];
    gpio_set_dir(row, GPIO_OUT);
    gpio_put(row, 0);
#endif
}

void emu_KeyboardScan(void* data)
{
#ifdef PICO_PICOZX_BOARD
    hid_keyboard_report_t* report = (hid_keyboard_report_t*)data;

    // scan the first row and parse, as the joystick takes time
    device_keyscan_row();

    // Handle shift
    if (rs[0] & 0x20)
    {
        report->modifier |= 0x22;
        rs[0] &= (~0x20);
    }

    // Determine how many key slots left
    int used = 0;
    while (report->keycode[used] != HID_KEY_NONE)
    {
        if (++used == 6)
        {
            reset_keyscan_row();
            return;
        }
    }

    // Check parts of row 0
    if (rs[0] & 0x40)
    {
        report->keycode[used++] = HID_KEY_PERIOD;
        if (used == 6)
        {
            reset_keyscan_row();
            return;
        }
    }

    sleep_us(9);

    for (int i=1; i<RN; ++i)
    {
        sleep_us(1);
        device_keyscan_row();
    }

    // Append remaining key presses
    for (int i=1; i<RN; ++i)
    {
        for (int j=0; j<CN; ++j)
        {
            if (rs[i] & (0x1<<j))
            {
                report->keycode[used++] = kbits[i][j];
                if (used == 6) return;
            }
        }
    }
#endif
}

#ifdef PICO_PICOZX_BOARD
static uint32_t g_ri = 0;

static void device_keyscan_row(void)
{
    uint32_t a = ~(gpio_get_all() >> CP_SHIFT);
    uint32_t r = CP_JOIN(a);
    uint32_t index_now = g_ri;

    // Revert line read
    gpio_put(rp[g_ri], 1);
    gpio_set_dir(rp[g_ri], GPIO_IN);
    gpio_disable_pulls(rp[g_ri]);

    if (++g_ri >= RN)
    {
        g_ri = 0;
    }

    // Prepare next line
    gpio_set_dir(rp[g_ri], GPIO_OUT);
    gpio_put(rp[g_ri], 0);

    // Save what was read
    // do at end to give time for next line to stablise
    rs[index_now] = (uint8_t)r;
}

static void reset_keyscan_row(void)
{
    g_ri = 0;
    gpio_set_dir(rp[g_ri], GPIO_OUT);
    gpio_put(rp[g_ri], 0);
}
#endif

bool emu_KeyboardUpdate(uint8_t* special)
{
  bool ret = hidReadUsbKeyboard(special, emu_DoubleShiftRequested());

  if (!ret)
  {
#ifdef PICO_PICOZX_BOARD
    // Handle device joystick
    emu_JoystickDeviceParse(((rs[0] & 0x04) != 0),
                            ((rs[0] & 0x10) != 0),
                            ((rs[0] & 0x02) != 0),
                            ((rs[0] & 0x08) != 0),
                            ((rs[0] & 0x01) != 0));

#else
    emu_JoystickParse();
#endif
  }
  return ret;
}

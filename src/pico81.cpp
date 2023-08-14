#include "pico.h"
#include "pico/stdlib.h"
#include "tusb.h"

#include "iopins.h"
#include "emuapi.h"
#include "emusound.h"
#include "emuvideo.h"
#include "zx8x.h"
#include "display.h"
#include "menu.h"

static void mainLoop(void);

int main(void)
{
#ifdef OVER_VOLT
    vreg_set_voltage(VREG_VOLTAGE_1_20)
#endif

    // Initialise sd card and read config
    emu_init();

    // Initialise display to get clock, using config
    uint clock = emu_VideoInit(emu_576Requested());

    // Set the clock, driven by the video rate
    set_sys_clock_khz(clock, true);
    stdio_init_all();

    // start the display generation
    displayBlank(false);
    displayStart();

    // Set the autoload name (if any)
    z8x_Start(emu_GetLoadName());

    // initialise usb
    tuh_init(BOARD_TUH_RHPORT);
    tuh_task();

    // Set up the emulator
    // This also set up the 50 Hz clock
    while (1)
    {
        z8x_Init();
        mainLoop();
    }
}

static void mainLoop(void)
{
    bool kdisp = false;
    while (1)
    {
        uint8_t s;
        tuh_task();

        if (emu_UpdateKeyboard(&s))
        {
            if (s==HID_KEY_F5)
            {
                if (!kdisp)
                {
                    displayShowKeyboard(!emu_ZX80Requested());
                    kdisp = true;
                }
            }
            else
            {
                if (s!=HID_KEY_ESCAPE)
                {
                    emu_silenceSound();
                }
                if (kdisp)
                {
                    displayHideKeyboard();
                    kdisp = false;
                }

                // Handle menus and reset
                switch (s)
                {
                    case HID_KEY_F1: // reset
                        return;
                    break;

                    case HID_KEY_F2:
                        if (loadMenu())
                        {
                            z8x_Start(emu_GetLoadName());
                            return;
                        }
                    break;

                    case HID_KEY_F3:
                        statusMenu();
                    break;

                    case HID_KEY_F4:
                        pauseMenu();
                    break;

                    case HID_KEY_F6:
                    {
                        bool reset;

                        if (modifyMenu(&reset))
                        {
                            if (reset)
                            {
                                return;
                            }
                            else
                            {
                                z8x_updateValues();
                            }
                        }
                    }
                    break;

                    case HID_KEY_F7:
                        rebootMenu();
                    break;
                }
            }
        }

        // Following can fail if user attempts load which requires different hardware
        if (!z8x_Step())
        {
            if (kdisp) displayHideKeyboard();
            return;
        }
        emu_WaitFor50HzTimer();
    }
}


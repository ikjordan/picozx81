/*******************************************************************
 Video
*******************************************************************/
#include "pico.h"
#include "emuapi.h"
#include "emusound.h"
#include "emuvideo.h"
#include "display.h"

// Sizes for 640 by 480
#define DISPLAY_START_PIXEL_640 46  // X Offset to first pixel with no centring
#define DISPLAY_START_Y_640     24  // Y Offset to first pixel with no centring
#define DISPLAY_ADJUST_X_640     4  // The number of pixels to adjust in X dimension to centre the display

// Sizes for 720 by 576
#define DISPLAY_START_PIXEL_720 24  // X Offset to first pixel with no centring
#define DISPLAY_START_Y_720      0  // Y Offset to first pixel with no centring
#define DISPLAY_ADJUST_X_720     2  // The number of pixels to adjust in X dimension to centre the display

Display_T disp;                     // Dimension information for the display

uint emu_VideoInit(void)
{
    bool match = false;
    bool five = (emu_576Requested() != OFF);

    if (five)
    {
        match = (emu_576Requested() != ON);
    }

    uint clock = displayInitialise(five, match, 1, &disp.width,
                                   &disp.height, &disp.stride_bit, emu_SoundSampleRate());

    if (disp.width >= 720)
    {
        disp.start_x = DISPLAY_START_PIXEL_720;
        disp.start_y = DISPLAY_START_Y_720;
        disp.adjust_x = DISPLAY_ADJUST_X_720;
    }
    else
    {
        disp.start_x = DISPLAY_START_PIXEL_640;
        disp.start_y = DISPLAY_START_Y_640;
        disp.adjust_x = DISPLAY_ADJUST_X_640;
    }

    // Initialise the rest of the structure
    disp.stride_byte = disp.stride_bit >> 3;
    disp.end_x = disp.start_x + disp.width;
    disp.end_y = disp.height + disp.start_y;
    disp.offset = -(disp.stride_bit * disp.start_y) - disp.start_x;

    return clock;
}

void emu_VideoSetInterlace(void)
{
    displaySetInterlace(emu_FrameSyncRequested() == SYNC_ON_INTERLACED);
}


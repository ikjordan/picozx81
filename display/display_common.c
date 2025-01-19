/*
 * Common display code for VGA, DVI and LCD
 */
#include <stdio.h>
#include <stdlib.h>
#include "pico/multicore.h"
#include "display.h"
#include "display_priv.h"
#include "zx80bmp.h"
#include "zx81bmp.h"
#include "../src/emusnap.h"

#ifdef SUPPORT_CHROMA
// Structure for chroma buffers
typedef struct
{
    uint8_t* buff;
    bool     used;
} chroma_t;
#endif

mutex_t next_frame_mutex;
semaphore_t display_initialised;

bool blank = true;
uint16_t blank_colour = BLACK;

uint8_t* curr_buff = 0;      // buffer being displayed
#ifdef SUPPORT_CHROMA
uint8_t* cbuffer = 0;        // Chroma buffer
#endif
const KEYBOARD_PIC* keyboard = &ZX81KYBD;
bool showKeyboard = false;

// Number of display buffers
#define MAX_FREE 4
#define MAX_PEND 2

// Note: Need 4 buffers, as the ZX81 produces rates at greater than 50 Hz
// so 2 frames can be created in one time slice, and 1 emulated time slice
// may complete in 14ms, therefore a backlog of 2 frames is valid, no_skip
// when nominally frame matched

static uint8_t* pend_buff[MAX_PEND] = {0, 0};           // Buffers queued for display
static uint8_t* free_buff[MAX_FREE] = {0, 0, 0, 0};     // Buffers available to be claimed
#ifdef SUPPORT_CHROMA
static chroma_t chroma[MAX_FREE] = { {0, false}, {0, false}, {0,false}, {0,false} };
#endif
static uint8_t* index_to_display[MAX_FREE] = {0, 0, 0, 0};
static uint8_t* last_buff = 0;      // previously displayed buffer (interlace mode only)

static uint8_t free_count = 0;
static uint8_t pend_count = 0;
static bool interlace = false;
static bool no_skip = false;

//
// Private interface
//
static inline void freeAllPending(void);
static inline void freeLast(void);
static inline void swapCurrAndLast(void);
#ifdef SUPPORT_CHROMA
static inline void displayGetChromaBufferUsed(uint8_t** chroma_buff, uint8_t* buff);
static inline void set_chroma_used(uint8_t* buff, bool used);
#endif
//
// Public functions
//

void displayAllocateBuffers(uint16_t minBuffByte, uint16_t stride, uint16_t height)
{
    // Allocate the buffers
    for (int i=0; i<MAX_FREE; ++i)
    {
        free_buff[i] = (uint8_t*)malloc(minBuffByte + stride * height)
                         + minBuffByte;

        // Store original index, so that can map a chroma buffer, if necessary
        index_to_display[i] = free_buff[i];
    }

#ifdef SUPPORT_CHROMA
    // Allocate chroma buffers
    for (int i=0; i<MAX_FREE; ++i)
    {
        chroma[i].buff = (uint8_t*)malloc(minBuffByte + stride * height)
                        + minBuffByte;

        if (!chroma[i].buff)
        {
            printf("Insufficient memory for chroma - aborting\n");
            exit(-1);
        }
    }
#endif
    free_count = MAX_FREE;
}

/* Set the interlace state */
void __not_in_flash_func(displaySetInterlace)(bool on)
{
    mutex_enter_blocking(&next_frame_mutex);
    interlace = on;

    if (!interlace)
    {
        freeLast();
    }
    mutex_exit(&next_frame_mutex);
}

/* Obtain a buffer from the free list */
void __not_in_flash_func(displayGetFreeBuffer)(uint8_t** buff)
{
    mutex_enter_blocking(&next_frame_mutex);
    if (free_count)
    {
        *buff = free_buff[0];
        --free_count;

        // Move items towards front of free list
        for (int i=0; i<free_count; ++i)
        {
            free_buff[i] = free_buff[i+1];
        }
    }
    else
    {
        // No buffer available - so must have at least 1 buffer pending
        // If there is a last buffer take that
        if (last_buff)
        {
            *buff = last_buff;
            last_buff = 0;
        }
        else if (curr_buff)
        {
            // Take the current
            *buff = curr_buff;
            curr_buff = pend_buff[0];

            if (--pend_count)
            {
                pend_buff[0] = pend_buff[1];
            }
        }
        else
        {
            // To get here is unexpected!
            printf("In displayGetFreeBuffer No buffers\n");
        }
    }
    mutex_exit(&next_frame_mutex);
}

/* Display the buffer, optionally delaying presentation until vsync
   Option to put the currently displayed buffer onto free list,
   note the option to not put the previous buffer onto the
   free list allows for it to be redisplayed later (e.g.
   after a menu is removed) */
void __not_in_flash_func(displayBuffer)(uint8_t* buff, bool sync, bool free, bool chroma)
{
    // Obtain lock
    mutex_enter_blocking(&next_frame_mutex);

#ifdef SUPPORT_CHROMA
    set_chroma_used(buff, chroma);
#endif
    if (sync && !blank)
    {
        // Store in next, unless next is full
        if (pend_count != MAX_PEND)
        {
            pend_buff[pend_count++] = buff;
        }
        else
        {
            // Already have two next buffers
            if (interlace)
            {
                // Release 2
                free_buff[free_count++] = pend_buff[0];
                free_buff[free_count++] = pend_buff[1];

                pend_buff[0] = buff;
                pend_count = 1;
            }
            else
            {
                // Release 1
                free_buff[free_count++] = pend_buff[0];

                pend_buff[0] = pend_buff[1];
                pend_buff[1] = buff;
            }
        }
    }
    else
    {
        // Display immediately
        freeAllPending();
        freeLast();

        // Read existing
        uint8_t* temp_buff = curr_buff;

        // write existing
        curr_buff = buff;
        if (temp_buff && free)
            free_buff[free_count++] = temp_buff;


        blank = false;
    }

    // free lock
    mutex_exit(&next_frame_mutex);
}

/* Get a pointer to the buffer currently being displayed.
   Typically this is done if the buffer should be redisplayed
   after a menu is displayed and subsequently removed*/
void __not_in_flash_func(displayGetCurrentBuffer)(uint8_t** buff)
{
    // Obtain lock
    mutex_enter_blocking(&next_frame_mutex);

    // Want the current buffer, or the queued buffer if one exists
    if (pend_count)
    {
        // Free the current buffer
        free_buff[free_count++] = curr_buff;

        curr_buff = pend_buff[--pend_count];
        freeAllPending();
    }
    *buff = curr_buff;

    // free lock
    mutex_exit(&next_frame_mutex);
}

#ifdef SUPPORT_CHROMA
/* Get the chroma buffer associated with a display buffer */
void __not_in_flash_func(displayGetChromaBuffer)(uint8_t** chroma_buff, uint8_t* buff)
{
    for (int i=0; i<MAX_FREE; i++)
    {
        if (index_to_display[i] == buff)
        {
            *chroma_buff = chroma[i].buff;
            return;
        }
    }
    *chroma_buff = 0;
}

/* Set all chroma buffers to unused */
void displayResetChroma(void)
{
    for (int i=0; i<MAX_FREE; i++)
    {
        chroma[i].used = false;
    }
}

/* Obtain a pointer to the chroma buffer associated with the pixel buffer
   return null pointer if chroma not enabled for this pixel buffer */
static inline void displayGetChromaBufferUsed(uint8_t** chroma_buff, uint8_t* buff)
{
    for (int i=0; i<MAX_FREE; i++)
    {
        if (index_to_display[i] == buff)
        {
            *chroma_buff = chroma[i].used ? chroma[i].buff : 0;
            return;
        }
    }
    *chroma_buff = 0;
}
#endif

/* Blank the display */
void __not_in_flash_func(displayBlank)(bool black)
{
    // Obtain lock
    mutex_enter_blocking(&next_frame_mutex);

    blank = true;
    blank_colour = black ? BLACK : WHITE;

    /* Free all buffers */
    freeAllPending();
    freeLast();

    if (curr_buff)
    {
        free_buff[free_count++] = curr_buff;
        curr_buff = 0;
    }

    // free lock
    mutex_exit(&next_frame_mutex);
}

bool displayIsBlank(bool* isBlack)
{
    *isBlack = (blank_colour == BLACK);
    return blank;
}

bool displayHideKeyboard(void)
{
    bool ret = showKeyboard;
    showKeyboard = false;
    return ret;
}

void displayStartCommon(void)
{
    // create a semaphore to be posted when video init is complete
    sem_init(&display_initialised, 0, 1);

    // launch all the video on core 1, so it isn't affected by USB handling on core 0
#ifdef PICOZX_LCD
    if (useLCD)
    {
        multicore_launch_core1(core1_main_lcd);
    }
    else
    {
        multicore_launch_core1(core1_main_vga);
    }
#else
    multicore_launch_core1(core1_main);
#endif
    sem_acquire_blocking(&display_initialised);
}

void __not_in_flash_func(newFrame)(void)
{
    mutex_enter_blocking(&next_frame_mutex);

    if (!blank)
    {
        if (pend_count)
        {
            if (interlace)
            {
                if (no_skip)
                {
                    // Free the last frame
                    if (last_buff)
                    {
                        free_buff[free_count++] = last_buff;
                    }

                    // Store a new last frame, unless we have another pending buffer
                    if (pend_count == MAX_PEND)
                    {
                        last_buff = 0;
                        free_buff[free_count++] = curr_buff;
                    }
                    else
                    {
                        last_buff = curr_buff;
                    }
                    curr_buff = pend_buff[0];
                    --pend_count;
                    if (pend_count)
                    {
                        pend_buff[0] = pend_buff[1];
                    }
                }
                else
                {
                    // We are displaying the last_buffer, because we
                    // missed earlier, so either swap, or jump forward if we have
                    // the buffers to do it
                    if (pend_count == MAX_PEND)
                    {
                        // Free the current and last buffers
                        freeLast();
                        free_buff[free_count++] = curr_buff;

                        curr_buff = pend_buff[1];
                        last_buff = pend_buff[0];
                        pend_count = 0;
                    }
                    else
                    {
                        swapCurrAndLast();
                    }
                    no_skip = !no_skip;
                }
            }
            else
            {
                // Just display next frame
                free_buff[free_count++] = curr_buff;
                curr_buff = pend_buff[0];
                --pend_count;
                if (pend_count)
                {
                    pend_buff[0] = pend_buff[1];
                }
            }
        }
        else
        {
            if (interlace)
            {
                // Have to swap back to previous, if there is one
                swapCurrAndLast();
                no_skip = !no_skip;
            }
        }
    }
#ifdef SUPPORT_CHROMA
    // Obtain the associated chroma buffer iff chroma enabled
    displayGetChromaBufferUsed(&cbuffer, curr_buff);
#endif
    mutex_exit(&next_frame_mutex);
}

bool display_save_snap(void)
{
    if (!emu_FileWriteBytes(&blank, sizeof(blank))) return false;
    if (!emu_FileWriteBytes(&blank_colour, sizeof(blank_colour))) return false;
    if (!emu_FileWriteBytes(&keyboard, sizeof(keyboard))) return false;
    if (!emu_FileWriteBytes(&showKeyboard, sizeof(showKeyboard))) return false;
    if (!emu_FileWriteBytes(&interlace, sizeof(interlace))) return false;
    if (!emu_FileWriteBytes(&no_skip, sizeof(no_skip))) return false;
    return true;
}

bool display_load_snap(void)
{
    if (!emu_FileReadBytes(&blank, sizeof(blank))) return false;
    if (!emu_FileReadBytes(&blank_colour, sizeof(blank_colour))) return false;
    if (!emu_FileReadBytes(&keyboard, sizeof(keyboard))) return false;
    if (!emu_FileReadBytes(&showKeyboard, sizeof(showKeyboard))) return false;
    if (!emu_FileReadBytes(&interlace, sizeof(interlace))) return false;
    if (!emu_FileReadBytes(&no_skip, sizeof(no_skip))) return false;
    return true;
}

//
// Private functions
//

#ifdef SUPPORT_CHROMA
/* Indicate whether chroma is enabled for the supplied pixel buffer */
static inline void set_chroma_used(uint8_t* buff, bool used)
{
    // Find the index of the buffer
    for (int i=0; i<MAX_FREE; i++)
    {
        if (index_to_display[i] == buff)
        {
            chroma[i].used = used;
            return;
        }
    }
}
#endif

static inline void __not_in_flash_func(freeAllPending)(void)
{
    for (int i=0; i<pend_count; ++i)
    {
        free_buff[free_count++] = pend_buff[i];
    }
    pend_count = 0;
}

static inline void __not_in_flash_func(freeLast)(void)
{
    if (last_buff)
    {
        free_buff[free_count++] = last_buff;
        last_buff = 0;
    }
}

static inline void __not_in_flash_func(swapCurrAndLast)(void)
{
    if (last_buff)
    {
        uint8_t* tmp_buff = curr_buff;
        curr_buff = last_buff;
        last_buff = tmp_buff;
    }
}

// Allow selection between VGA and LCD for ZXPICO board
#ifdef PICOZX_LCD
#undef WHITE
#undef BLUE
#undef YELLOW
#undef RED
#undef BLACK

#define WHITE  0xfff
#define BLUE   0x00f
#define YELLOW 0xff0
#define RED    0xf00
#define BLACK  0x000

#undef ZX80_KEYBOARD
#define ZX80_KEYBOARD ZX80KYBD_LCD
#undef ZX81_KEYBOARD
#define ZX81_KEYBOARD ZX81KYBD_LCD
#include "zx80bmp.h"
#include "zx81bmp.h"

extern uint displayInitialiseLCD(bool fiveSevenSix, bool match, uint16_t minBuffByte, uint16_t* pixelWidth,
                                 uint16_t* pixelHeight, uint16_t* strideBit, DisplayExtraInfo_T* info);
extern void displayStartLCD(void);
extern bool displayShowKeyboardLCD(bool ROM8K);

extern uint displayInitialiseVGA(bool fiveSevenSix, bool match, uint16_t minBuffByte, uint16_t* pixelWidth,
                                 uint16_t* pixelHeight, uint16_t* strideBit, DisplayExtraInfo_T* info);
extern void displayStartVGA(void);
extern bool displayShowKeyboardVGA(bool ROM8K);

uint displayInitialise(bool fiveSevenSix, bool match, uint16_t minBuffByte, uint16_t* pixelWidth,
                       uint16_t* pixelHeight, uint16_t* strideBit, DisplayExtraInfo_T* info)
{
    if (useLCD)
    {
        return displayInitialiseLCD(fiveSevenSix, match, minBuffByte, pixelWidth,
                                    pixelHeight, strideBit, info);
    }
    else
    {
        return displayInitialiseVGA(fiveSevenSix, match, minBuffByte, pixelWidth,
                                    pixelHeight, strideBit, info);
    }
}

void displayStart(void)
{
    if (useLCD)
    {
        return displayStartLCD();
    }
    else
    {
        return displayStartVGA();
    }
}

bool displayShowKeyboard(bool ROM8K)
{
    if (useLCD)
    {
        return displayShowKeyboardLCD(ROM8K);
    }
    else
    {
        return displayShowKeyboardVGA(ROM8K);
    }
}

#endif

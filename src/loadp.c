#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include "emuapi.h"
#include "ff.h"
#include "common.h"
#include "loadp.h"

#define LOADP_INITIAL_TCOUNT 0xeeeeeeee

#define LOADP_HIGH_LENGTH           488         // 150.1 us
#define LOADP_LOW_LENGTH            487         // 149.8 us
#define LOADP_SILENCE_LENGTH        4225        // 1300 us
#define LOADP_SILENCE_INTRO_LENGTH  6500000     // 2 s  (Note 5 s of silence saved)

#define LOADP_ZERO_COUNT 4
#define LOADP_ONE_COUNT 9

typedef enum
{
    LOW_PULSE = 0,
    HIGH_PULSE = 1,
    SILENCE_PULSE = 2,
} PulseState_t;

typedef enum
{
    ZERO_BIT = 0,
    ONE_BIT = 1,
} BitState_t;

static bool initialised = false;
static FIL fil;
static int bit_pos = 7;
static int name_addr = 0;
static bool name_sent = false;

static uint32_t ltstate = LOADP_INITIAL_TCOUNT;
static uint32_t tpulse = 0;

static BitState_t bit_state = ZERO_BIT;

static PulseState_t pulse_state = SILENCE_PULSE;
static uint32_t pulse_length_max = LOADP_SILENCE_INTRO_LENGTH;
static uint pulse_count = 0;
static char byte_to_send = 0;

static void moveToNextByte(void);
static bool getNextByte(char* byte);
static bool skipName(void);

bool loadPInitialise(char* fullpath, int filename, bool ROM4k)
{
#ifdef DEBUG_LOADP
    printf("loadPInitialise: P: %s, F: %04x, ROM4k %c\n", fullpath, filename, ROM4k ? 'T' : 'F');
#endif
    name_sent = ROM4k;       // 4K ROM files have no name
    name_addr = (filename < 0x8000) ? filename : 0;

EMU_LOCK_SDCARD
    if (initialised)
    {
        f_close(&fil);
    }

    initialised = (f_open(&fil, fullpath, FA_READ) == FR_OK);
EMU_UNLOCK_SDCARD

    if (initialised)
    {
        if ((!ROM4k) && emu_EndsWith(fullpath, ".p81"))
        {
            // Skip the name in the file, as it may differ from
            // the name supplied, and we always want to match.
            // Strictly a p81 file should not be loaded on a 4K ROM, but this
            // allows for the "hack" where the same p81 file can be loaded to both
            // ROM sizes by using a 9 character name to offset the start of
            // memory load address
            if (!skipName())
            {
                loadPUninitialise();
            }
        }
        ltstate = LOADP_INITIAL_TCOUNT;
    }
    return initialised;
}

void loadPUninitialise(void)
{
#ifdef DEBUG_LOADP
    printf("loadPUninitialise\n");
#endif
    if (initialised)
    {
        // Close the file
EMU_LOCK_SDCARD
        f_close(&fil);
EMU_UNLOCK_SDCARD
    }
    initialised = false;
}

int loadPGetBit(void)
{
    int pulse = 0;      // By default return a low pulse

    if (initialised)
    {
        // Determine time delta
        if (ltstate == LOADP_INITIAL_TCOUNT)
        {
            // Start with 5 seconds of silence
            tpulse = 0;
            pulse_state = SILENCE_PULSE;
            pulse_count = 0;
            pulse_length_max = LOADP_SILENCE_INTRO_LENGTH;
            bit_state = 0;  // Not used
            bit_pos = -1;   // So new byte will be triggered after silence
        }
        else
        {
            tpulse += ((tstates + tsmax - ltstate) % tsmax);
        }
        ltstate = tstates;

        // Has the current pulse completed?
        if (tpulse > pulse_length_max)
        {
            tpulse -= pulse_length_max;

            switch (pulse_state)
            {
                case LOW_PULSE:
                {
                    // Followed by 1 or silence
                    pulse_count++;

                    // Determine what comes after the 0 pulse
                    int max_count = (bit_state == ZERO_BIT) ? LOADP_ZERO_COUNT : LOADP_ONE_COUNT;

                    if (max_count  == pulse_count)
                    {
                        pulse_state = SILENCE_PULSE;
                        pulse_length_max = LOADP_SILENCE_LENGTH;
                    }
                    else
                    {
                        pulse_state = HIGH_PULSE;
                        pulse_length_max = LOADP_HIGH_LENGTH;
                    }
                }
                break;

                case HIGH_PULSE:
                    // Always followed by zero
                    pulse_state = LOW_PULSE;
                    pulse_length_max = LOADP_LOW_LENGTH;
                break;

                case SILENCE_PULSE:
                    // Move to next bit
                    if (bit_pos == -1) moveToNextByte();

                    bit_state = ((byte_to_send >> bit_pos) & 0x1) ? ONE_BIT : ZERO_BIT;
                    bit_pos--;
                    pulse_state = HIGH_PULSE;
                    pulse_length_max = LOADP_HIGH_LENGTH;
                    pulse_count = 0;
                break;
            }
        }
        pulse = (pulse_state == HIGH_PULSE) ? 1: 0;
    }
    return pulse;
}

static void moveToNextByte(void)
{
    if (initialised)
    {
        if (!name_sent)
        {
            if (name_addr)
            {
                byte_to_send = mem[name_addr++];
            }
            else
            {
                // No name needed - so send 1 inverse character
                byte_to_send = 0xa6;    // Inverse A
            }

            // Inverse character (bit 7 set) indicates end of name
            if (byte_to_send & 0x80)
            {
                name_sent = true;
#ifdef DEBUG_LOADP
                printf("moveToNextByte: Name sent\n");
#endif
            }
        }
        else
        {
            // Read a byte from the file, if any remain
EMU_LOCK_SDCARD
            int eof = f_eof(&fil);
EMU_UNLOCK_SDCARD
            if (!eof)
            {
                getNextByte(&byte_to_send);
            }
            else
            {
                loadPUninitialise();
                byte_to_send = 0;
#ifdef DEBUG_LOADP
                printf("moveToNextByte: EOF\n");
#endif
            }
        }
    }
    else
    {
        byte_to_send = 0;
    }

    bit_pos = 7;
#ifdef DEBUG_LOADP
    printf("moveToNextByte: %02x\n", byte_to_send);
#endif
}

static bool getNextByte(char* byte)
{
    UINT bytes_read = 0;
    FRESULT res= f_read(&fil, byte, 1, &bytes_read);
    return ((res == FR_OK) && (bytes_read == 1));
}

static bool skipName(void)
{
    char read_buffer;
    bool ret;

EMU_LOCK_SDCARD
    do
    {
        ret = getNextByte(&read_buffer);
    } while (ret && ((read_buffer & 0x80) == 0));
    
EMU_UNLOCK_SDCARD
    return ret;
}

#include "pico.h"
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"
#include "pico/sync.h"
#include "audio_ring.h"
#include "display.h"
#include "emuapi.h"
#include "emupriv.h"
#include "emusound_common.h"

#define FIFTYHZMS       20                  // ms between 50 HZ ticks
#define TICKMS          2                   // ms ticks to service hdmi audio ring buffer
#define TICKCOUNT       (FIFTYHZMS/TICKMS)  // number of ring buffer ticks per 50Hz tick
#define TICK_SAMPLES    (NUMSAMPLES / TICKCOUNT)

static struct repeating_timer audio_timer;
static audio_ring_t* ring;
static audio_sample_t* hdmi_buffer;
static int hdmi_buffer_size;

static bool __not_in_flash_func(audio_timer_callback)(struct repeating_timer *t)
{
    (void)(t);
    static uint32_t call_count = 0;
    static int cnt = 0;

    // write in chunks
    int size = get_write_size(ring, true);
    if (size >= TICK_SAMPLES)
    {
        int audio_offset = get_write_offset(ring);
        if ((size >= ((3*hdmi_buffer_size)>>2)) &&
            (cnt <= ((NUMSAMPLES << 2)-(TICK_SAMPLES << 1))))
        {
            // Allow to refill buffer
            size = (TICK_SAMPLES<<1);
        }
        else
        {
            size = TICK_SAMPLES;
        }

        for (int c = 0; c < size; c++)
        {
            hdmi_buffer[audio_offset].channels[0] = soundBuffer16[cnt++];
            hdmi_buffer[audio_offset].channels[1] = soundBuffer16[cnt++];
            audio_offset = (audio_offset + 1) & (hdmi_buffer_size-1);
        }
        set_write_offset(ring, audio_offset);

        if (cnt >= (NUMSAMPLES << 2))
        {
            cnt -= (NUMSAMPLES << 2);
        }
    }

    if (++call_count == TICKCOUNT)
    {
        call_count = 0;

        // Swap the buffers
        sound_first = !sound_first;

        // resync the play pointer
        if (sound_first)
        {
            cnt=0;
        }
        // Signal the 50Hz semaphore
        sem_release(&timer_sem);
    }
    return true;
}

void initAudio_hdmi(void)
{
    // Create the timer callback
    getAudioRing(&ring);
    hdmi_buffer = ring->buffer;
    hdmi_buffer_size = ring->size;
}

void startAudio_hdmi(void)
{
    add_repeating_timer_ms(-TICKMS, audio_timer_callback, NULL, &audio_timer);
}

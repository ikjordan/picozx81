/*******************************************************************
 Sound
*******************************************************************/
#include "pico.h"
#include <stdlib.h>
#include "pico/stdlib.h"
#include <stdio.h>
//#include "hardware/irq.h"
//#include "hardware/clocks.h"
#include "pico/sync.h"
#include "sound.h"
#include "emuapi.h"
#include "emupriv.h"
#include "emusound.h"
#include "emusound_common.h"

semaphore_t timer_sem;

extern const uint16_t NUMSAMPLES = (SAMPLE_FREQ / 50); // samples in 50th of second

uint16_t soundBuffer16[NUMSAMPLES << 2]; // Effectively two stereo buffers
uint16_t* soundBuffer2 = &soundBuffer16[NUMSAMPLES << 1];

volatile bool first = true;      // True if the first buffer is playing
bool genSound = false;

int queued_sound_type;           // new sound type requested
int change_count;                // count down to frame to change sound type
bool queued_play;

static void beginAudio(void);

#ifdef TIME_SPARE
int32_t sound_count = 0;
int64_t int_count = 0;
#endif

uint16_t emu_sndGetSampleRate(void)
{
  return SAMPLE_FREQ;
}

void emu_sndInit(bool playSound, bool reset)
{
  static bool soundCreated = false;

  genSound = playSound;
  change_count = 0;   // in case a changed was queued

  // This can be called multiple times
  if (!soundCreated)
  {
    sound_create();
    sound_init(emu_ACBRequested(), reset);
    emu_sndSilence();

    // audio drives the 50Hz timer
    beginAudio();
    soundCreated = true;
  }
  else
  {
    // Call each time, as sound type may have changed
    sound_init(emu_ACBRequested(), reset);
    emu_sndSilence();
  }
}

int emu_sndImmediateChange(int current_sound_type, int new_sound_type)
{
  int old_sound;

  // Is there a queued change?
  if (change_count != 0)
  {
    // There is, so the queued change becomes the cache
    old_sound = queued_sound_type;
    sound_change_type(new_sound_type);
    change_count = 0;
  }
  else
  {
    // No queued change
    old_sound = current_sound_type;
    if (current_sound_type != new_sound_type)
    {
      sound_change_type(new_sound_type);
      emu_sndInit(new_sound_type != SOUND_TYPE_NONE, true);
    }
  }
  return old_sound;
}

void emu_sndQueueChange(int new_sound_type)
{
  queued_sound_type = new_sound_type;
  queued_play = (new_sound_type != SOUND_TYPE_NONE);
  change_count = 3; // Allow final save bytes to propagate
}

// Calls to this function are synchronised to 50Hz through main timer interrupt
void emu_sndGenerateSamples(void)
{
  if (genSound)
  {
    sound_frame(first ? soundBuffer2 : soundBuffer16);
#ifdef TIME_SPARE
    sound_count++;
#endif

    // process any queued sound change
    if (change_count)
    {
      if (--change_count == 0)
      {
        sound_change_type(queued_sound_type);
        emu_sndInit(queued_play, false);
      }
    }
  }
}

static void beginAudio(void)
{
  sem_init(&timer_sem, 0, 1);
#ifndef SOUND_HDMI
#ifdef SOUND_I2S
  beginAudio_i2s();
#else // SOUND_I2S
  beginAudio_pwm();
#endif // SOUND_I2S
#else // SOUND_HDMI
  beginAudio_hdmi();
#endif // SOUND_HDMI

  printf("sound initialized\n");
}

void emu_sndSilence(void)
{
  // Set buffers to silence
  for (int i = 0; i < (NUMSAMPLES<<2); ++i)
  {
    soundBuffer16[i] = ZEROSOUND;
  }
}

bool emu_sndSaveSnap(void)
{
  if (!sound_save_snap()) return false;
  if (!emu_FileWriteBytes(&queued_sound_type, sizeof(queued_sound_type))) return false;
  if (!emu_FileWriteBytes(&queued_play, sizeof(queued_play))) return false;

  return true;
}

bool emu_sndLoadSnap(uint32_t version)
{
  if (!sound_load_snap(version)) return false;
  if (!emu_FileReadBytes(&queued_sound_type, sizeof(queued_sound_type))) return false;
  if (!emu_FileReadBytes(&queued_play, sizeof(queued_play))) return false;

  return true;
}

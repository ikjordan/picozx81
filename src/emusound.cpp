/*******************************************************************
 Sound
*******************************************************************/
#include "pico.h"
#include <stdlib.h>
#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/sync.h"
#include "sound.h"
#include "emuapi.h"
#include "emupriv.h"
#include "emusound.h"
#include "emusound_common.h"
#ifdef INPUT_EAR
#include "emulinein.h"
#endif

semaphore_t timer_sem;

extern const uint16_t NUMSAMPLES = (SAMPLE_FREQ / 50); // samples in 50th of second

uint16_t soundBuffer16[NUMSAMPLES << 2];  // Effectively two stereo buffers
uint16_t* soundBuffer2 = &soundBuffer16[NUMSAMPLES << 1];
volatile bool sound_first = true;         // True if the first buffer is playing

#ifdef MIC_SOUND
uint16_t micBuffer16[NUMSAMPLES << 2];  // Effectively two stereo buffers
uint16_t* micBuffer2 = &micBuffer16[NUMSAMPLES << 1];
volatile bool mic_first = true;         // True if the first buffer is playing
#endif

int queued_sound_type;           // new sound type requested
int change_count;                // count down to frame to change sound type

static void beginAudio(void);

#ifdef TIME_SPARE
int32_t sound_count = 0;
int64_t int_count = 0;
#endif

uint16_t emu_sndGetSampleRate(void)
{
  return SAMPLE_FREQ;
}

void emu_sndInit(bool force_reset)
{
  static bool soundCreated = false;

  change_count = 0;   // in case a changed was queued

  // This can be called multiple times
  if (!soundCreated)
  {
    sound_create();
    sound_init(emu_ACBRequested(), force_reset);
    emu_sndSilence();

    // audio drives the 50Hz timer
    beginAudio();
    soundCreated = true;
  }
  else
  {
    // Call each time, as sound type may have changed
    sound_init(emu_ACBRequested(), force_reset);
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
      emu_sndInit(false);
    }
  }
  return old_sound;
}

void emu_sndQueueChange(int new_sound_type)
{
  queued_sound_type = new_sound_type;
  change_count = 3; // Allow final save bytes to propagate
}

// Calls to this function are synchronised to 50Hz through main timer interrupt
void emu_sndGenerateSamples(void)
{
  sound_frame(sound_first ? soundBuffer2 : soundBuffer16);
#ifdef MIC_SOUND
  mic_frame(mic_first ? micBuffer2 : micBuffer16);
#endif
#ifdef TIME_SPARE
    sound_count++;
#endif

  // process any queued sound change
  if (change_count)
  {
    if (--change_count == 0)
    {
      sound_change_type(queued_sound_type);
      emu_sndInit(false);
    }
  }
}

static void beginAudio(void)
{
  sem_init(&timer_sem, 0, 1);

  // Configure the transfers
#ifdef SOUND_I2S
  initAudio_i2s();
#endif

#if defined(SOUND_DMA) || defined(SOUND_PWM)
  initAudio_pwm();
#endif

#ifdef SOUND_HDMI
  initAudio_hdmi();
#endif

// Start the transfers
#ifdef INPUT_EAR
  emu_linein_start();
#endif

#ifdef SOUND_I2S
  startAudio_i2s();
#endif

#if defined(SOUND_DMA) || defined(SOUND_PWM)
  startAudio_pwm();
#endif

#ifdef SOUND_HDMI
  startAudio_hdmi();
#endif

  printf("sound initialized\n");
}

void emu_sndSilence(void)
{
  // Set buffers to silence
  for (int i = 0; i < (NUMSAMPLES<<2); ++i)
  {
    soundBuffer16[i] = ZEROSOUND;
#ifdef MIC_SOUND
    micBuffer16[i] = ZEROMIC;
#endif
  }
}

bool emu_sndSaveSnap(void)
{
  if (!sound_save_snap()) return false;
  if (!emu_FileWriteBytes(&queued_sound_type, sizeof(queued_sound_type))) return false;

  return true;
}

bool emu_sndLoadSnap(uint32_t version)
{
  if (!sound_load_snap(version)) return false;
  if (!emu_FileReadBytes(&queued_sound_type, sizeof(queued_sound_type))) return false;
  if (version == SUPPORTED_VERSION_1)
  {
    bool dummy;
    if (!emu_FileReadBytes(&dummy, sizeof(dummy))) return false;
  }
  return true;
}

#ifndef EMUSOUNDCOMMON_H
#define EMUSOUNDCOMMON_H
#include "pico.h"
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/sync.h"
#include "sound.h"

extern semaphore_t timer_sem;

extern const uint16_t NUMSAMPLES;

extern uint16_t soundBuffer16[];
extern uint16_t* soundBuffer2;

extern volatile bool first;
extern bool genSound;

extern int queued_sound_type;           // new sound type requested
extern int change_count;                // count down to frame to change sound type
extern bool queued_play;

#ifdef SOUND_I2S
void beginAudio_i2s(void);
#endif

#ifdef SOUND_HDMI
void beginAudio_hdmi(void);
#endif

#ifdef SOUND_DMA
void initAudio_dma(int audio_pin_slice_r, int audio_pin_slice_l);
void startAudio_dma(void);
#endif

#if defined(SOUND_DMA) || defined (SOUND_PWM)
void beginAudio_pwm(void);
#endif

#endif


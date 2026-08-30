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
extern volatile bool sound_first;

#ifdef MIC_SOUND
extern uint16_t micBuffer16[];
extern uint16_t* micBuffer2;
extern volatile bool mic_first;

#define SBUFFER16 micBuffer16
#define SBUFFER2  micBuffer2
#define SFIRST    mic_first
#define SEM_REL
#else
#define SBUFFER16 soundBuffer16
#define SBUFFER2  soundBuffer2
#define SFIRST    sound_first
#define SEM_REL   sem_release(&timer_sem)
#endif

extern int queued_sound_type;           // new sound type requested
extern int change_count;                // count down to frame to change sound type

#ifdef SOUND_I2S
void initAudio_i2s(void);
void startAudio_i2s(void);
#endif

#ifdef SOUND_HDMI
void initAudio_hdmi(void);
void startAudio_hdmi(void);
#endif

#if defined(SOUND_DMA) || defined (SOUND_PWM)
void initAudio_pwm(void);
void startAudio_pwm(void);
#endif

#endif


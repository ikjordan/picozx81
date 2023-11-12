#ifndef EMUSOUND_H
#define EMUSOUND_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
extern void emu_sndInit(bool playSound, bool reset);
extern void emu_generateSoundSamples(void);
extern void emu_silenceSound(void);
extern uint16_t emu_SoundSampleRate(void);

#ifdef __cplusplus
}
#endif

#endif

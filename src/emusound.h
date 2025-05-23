#ifndef EMUSOUND_H
#define EMUSOUND_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
extern void emu_sndInit(bool playSound, bool reset);
extern void emu_sndGenerateSamples(void);
extern void emu_sndSilence(void);
extern uint16_t emu_sndGetSampleRate(void);
extern void emu_sndQueueChange(bool playSound, int queued_sound_type);

extern bool emu_sndSaveSnap(void);
extern bool emu_sndLoadSnap(uint32_t version);

#ifdef __cplusplus
}
#endif

#endif

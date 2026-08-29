#ifndef EMUSOUND_H
#define EMUSOUND_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif
extern void emu_sndInit(int new_sound_type, bool force_reset);
extern void emu_sndGenerateSamples(void);
extern void emu_sndSilence(void);
extern uint16_t emu_sndGetSampleRate(void);
extern void emu_sndQueueChange(int queued_sound_type);
extern int emu_sndImmediateChange(int current_sound_type, int new_sound_type);

extern bool emu_sndSaveSnap(void);
extern bool emu_sndLoadSnap(uint32_t version);

#ifdef __cplusplus
}
#endif

#endif

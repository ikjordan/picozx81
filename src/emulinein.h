#ifndef EMULINEIN_H
#define EMULINEIN_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void emu_linein_initialise(LoadVolume_T vol);
extern void emu_linein_start(void);

extern void emu_linein_set_frame_tstate(uint32_t tstates);
extern void emu_linein_apply_filter(void);
extern bool emu_linein_signal_high(uint32_t tstates);
extern void emu_linein_set_volume(LoadVolume_T vol);

// Helper function for debug
extern void emu_linein_get_buffer(uint32_t** buffer, uint32_t* buffer_size);

#ifdef __cplusplus
}
#endif

#endif

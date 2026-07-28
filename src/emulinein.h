#ifndef EMULINEIN_H
#define EMULINEIN_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LINEIN_RATE_HZ  (32000)

extern void emu_linein_initialise(void);
extern void emu_linein_start(void);

extern void emu_linein_set_frame_tstate(uint32_t tstates);
extern bool emu_linein_is_high(uint32_t tstates);

// Helper function for debug
extern void emu_linein_get_buffer(uint32_t** buffer, uint32_t* buffer_size);

#ifdef __cplusplus
}
#endif

#endif
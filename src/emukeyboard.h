/* Device keyboard */
#ifndef emukeyboard_h
#define emukeyboard_h

#ifdef __cplusplus
extern "C" {
#endif

extern void emu_KeyboardInitialise(uint8_t* keyboard);
extern bool emu_KeyboardUpdate(uint8_t* special);
extern void emu_KeyboardScan(void* data);

#ifdef PICO_PICOZX_BOARD
extern bool emu_KeyboardFire(void);
#endif

#ifdef __cplusplus
}
#endif
#endif

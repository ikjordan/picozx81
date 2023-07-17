#ifndef EMUAPI_H
#define EMUAPI_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_FILENAME_LEN 64
#define MAX_DIRECTORY_LEN 128
#define MAX_FULLPATH_LEN (MAX_FILENAME_LEN + MAX_DIRECTORY_LEN)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ZX80 = 0,
    ZX81,
    ZX81X2 // Big bang ROM
} ComputerType;

extern void emu_init(void);

extern bool emu_UpdateKeyboard(uint8_t* special);

extern bool emu_FileOpen(const char * filepath, const char * mode);
extern int emu_FileRead(void * buf, int size, int offset);
extern void emu_FileClose(void);

extern unsigned int emu_FileSize(const char * filepath);
extern bool emu_SaveFile(const char * filepath, void * buf, int size);
extern bool emu_endsWith(const char * s, const char * suffix);

extern const char* emu_GetLoadName(void);
extern void emu_SetLoadName(const char* name);
extern const char* emu_GetDirectory(void);
extern void emu_SetDirectory(const char* dir);
extern void emu_ReadDefaultValues(void);
extern void emu_ReadSpecificValues(const char* filename);
extern void emu_setZX80(bool zx80);
extern int emu_MemoryRequested(void);
extern bool emu_ZX80Requested(void);
extern ComputerType emu_ComputerRequested(void);
extern bool emu_M1NOTRequested(void);
extern bool emu_LowRAMRequested(void);
extern bool emu_QSUDGRequested(void);
extern bool emu_WRXRequested(void);
extern bool emu_NTSCRequested(void);
extern int emu_soundRequested(void);
extern bool emu_DoubleShiftRequested(void);
extern bool emu_ExtendFileRequested(void);
extern bool emu_AllFilesRequested(void);
extern uint16_t emu_VTol(void);
extern bool emu_ACBRequested(void);
extern int emu_CentreX(void);
extern int emu_CentreY(void);
extern bool emu_resetNeeded(void);

extern void emu_DisplayFrame(unsigned char * buf);
extern void emu_BlankScreen(void);

extern void emu_WaitFor50HzTimer(void);

#ifdef __cplusplus
}
#endif

#endif

#ifndef EMUAPI_H
#define EMUAPI_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_FILENAME_LEN 64
#define MAX_DIRECTORY_LEN 128
#define MAX_FULLPATH_LEN (MAX_FILENAME_LEN + MAX_DIRECTORY_LEN)
#define VTOL 25       // Default TV Vertical tolerance in scanlines

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ZX80_4K = 0,
    ZX80_8K,
    ZX81,
    ZX81X2 // Big bang ROM
} ComputerType_T;

typedef enum {
    SYNC_OFF = 0,
    SYNC_ON,
    SYNC_ON_INTERLACED
} FrameSync_T;

typedef enum {
    OFF = 0,
    ON,
    MATCH
} FiveSevenSix_T;

extern void emu_init(void);
extern bool emu_fsInitialised(void);

extern bool emu_FileOpen(const char * filepath, const char * mode);
extern int emu_FileRead(void * buf, int size, int offset);
extern void emu_FileClose(void);

extern unsigned int emu_FileSize(const char * filepath);
extern bool emu_SaveFile(const char * filepath, void * buf, int size);
extern bool emu_EndsWith(const char * s, const char * suffix);

extern const char* emu_GetLoadName(void);
extern void emu_SetLoadName(const char* name);
extern const char* emu_GetDirectory(void);
extern void emu_SetDirectory(const char* dir);
extern void emu_ReadDefaultValues(void);
extern void emu_ReadSpecificValues(const char* filename);
extern int emu_MemoryRequested(void);
extern bool emu_ZX80Requested(void);
extern bool emu_ROM4KRequested(void);
extern ComputerType_T emu_ComputerRequested(void);
extern bool emu_M1NOTRequested(void);
extern bool emu_LowRAMRequested(void);
extern bool emu_QSUDGRequested(void);
extern bool emu_WRXRequested(void);
extern bool emu_CHR128Requested(void);
extern bool emu_NTSCRequested(void);
extern int emu_SoundRequested(void);
extern bool emu_DoubleShiftRequested(void);
extern bool emu_ExtendFileRequested(void);
extern bool emu_AllFilesRequested(void);
extern bool emu_NinePinJoystickRequested(void);
extern bool emu_loadUsingROMRequested(void);
extern bool emu_saveUsingROMRequested(void);
extern int emu_MenuBorderRequested(void);
extern uint16_t emu_VTol(void);
extern bool emu_ACBRequested(void);
extern bool emu_ACBPossible(void);
extern bool emu_Centre(void);
extern int emu_CentreX(void);
extern int emu_CentreY(void);
extern bool emu_ResetNeeded(void);

extern bool emu_lcdInvertColourRequested(void);
extern bool emu_lcdSkipFrameRequested(void);
extern bool emu_lcdRotateRequested(void);
extern bool emu_lcdReflectRequested(void);
extern bool emu_lcdBGRRequested(void);

extern bool emu_vgaRequested(void);

extern FrameSync_T emu_FrameSyncRequested(void);
extern FiveSevenSix_T emu_576Requested(void);

extern bool emu_chromaSupported(void);

extern void emu_SetRom4K(bool Rom4k);
extern void emu_SetFrameSync(FrameSync_T fsync);
extern void emu_SetNTSC(bool ntsc);
extern void emu_SetVTol(uint16_t vTol);
extern void emu_SetWRX(bool wrx);
extern void emu_SetCentre(bool centre);
extern void emu_SetSound(int soundType);
extern void emu_SetACB(bool stereo);
extern void emu_SetComputer(ComputerType_T computer);
extern void emu_SetMemory(int memory);
extern void emu_SetLowRAM(bool lowRAM);
extern void emu_SetM1NOT(bool m1NOT);
extern void emu_SetLoadROM(bool loadROM);
extern void emu_SetSaveROM(bool saveROM);
extern void emu_SetQSUDG(bool qsudg);
extern void emu_SetCHR128(bool chr128);

extern void emu_SetRebootMode(FiveSevenSix_T mode);

extern void emu_WaitFor50HzTimer(void);

extern void emu_JoystickInitialiseNinePin(void);
extern void emu_JoystickParse(void);
extern void emu_JoystickDeviceParse(bool up, bool down, bool left, bool right, bool button);

#ifdef PICO_SPI_LCD_SD_SHARE
extern void emu_lockSDCard(void);
extern void emu_unlockSDCard(void);

#define EMU_LOCK_SDCARD emu_lockSDCard();
#define EMU_UNLOCK_SDCARD emu_unlockSDCard();
#else
#define EMU_LOCK_SDCARD
#define EMU_UNLOCK_SDCARD
#endif

#ifdef __cplusplus
}
#endif

#endif

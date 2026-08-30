#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdbool.h>
#include <stdint.h>

typedef unsigned char  byte;

typedef enum
{
    ROM_OFF = 0,
    ROM_SD_CARD,
    ROM_EAR_MIC
} SLRomType_T;

/* ROM load and save detection */
// Detection of start of save and load
#define LOAD_START_4K       0x207       // POP DE           D1
#define SAVE_START_4K       0x1b7       // POP DE           D1
#define LOAD_START_8K       0x347       // RRC D            CB 10
#define SAVE_START_8K       0x2ff       // LD DE,$12CB      11

// Restart addresses after loading without using the ROM
#define LOAD_SAVE_RET_4K    0x203
#define LOAD_SAVE_RET_8K    0x207

// Detection of success and failure when using the ROM
#define LOAD_SAVE_SUCCESS_4K  0x204     // JP $0283
#define LOAD_SAVE_FAILURE_4K  0x24e     // DEC D
#define LOAD_SAVE_SUCCESS_8K  0x20A     // LD HL,$403B
#define LOAD_SAVE_FAILURE_8K  0x207     // POP HL - NOT USED


typedef struct
{
    uint16_t        start;
    SLRomType_T     use_rom;
} RomPatch_T;

typedef struct
{
    RomPatch_T load;
    RomPatch_T save;
    uint16_t   retAddr;
    uint16_t   successAddr;
    uint16_t   failureAddr;
} RomPatches_T;

extern RomPatches_T rom_patches;

typedef enum
{
    LOAD_SAVE_COMPLETED = 0,
    LOAD_SAVE_ROM,
    LOAD_SAVE_FAILED,
    LOAD_SAVE_REBOOT_NEEDED
} LoadSaveResult_t;

/* SOUND board types */
#define SOUND_TYPE_NONE         0
#define SOUND_TYPE_QUICKSILVA   1
#define SOUND_TYPE_ZONX         2
#define SOUND_TYPE_CHROMA       3
#define SOUND_TYPE_VSYNC        4
#define SOUND_TYPE_CASSETTE     5

#define MEMORYRAM_SIZE 0x10000

extern unsigned char mem[MEMORYRAM_SIZE];
extern unsigned char *memptr[64];
extern int memattr[64];
extern int sound_type;
extern unsigned long tstates;
extern unsigned long tstates_frame;
extern const unsigned long tsmax;
extern int ramsize;
extern int autoload;
extern int zx80;
extern int rom4k;
extern bool m1not;
extern bool useWRX;
extern bool useQSUDG;
extern bool UDGEnabled;
extern bool LowRAM;
extern bool chr128;
extern bool useNTSC;
extern bool frameSync;
extern bool running_rom;

/* Chroma variables */
extern int chromamode;
#ifdef SUPPORT_CHROMA
extern unsigned char chroma_set;
extern unsigned char bordercolour;
extern unsigned char bordercolournew;
extern unsigned char fullcolour;
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int in(int h, int l);
extern void out(int h, int l, int a);
extern LoadSaveResult_t save_p(int name_addr, bool defer_rom);
extern LoadSaveResult_t load_p(int name_addr, bool defer_rom);

#ifdef __cplusplus
}
#endif
#endif


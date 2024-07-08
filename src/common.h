#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdbool.h>
#include <stdint.h>

typedef unsigned char  byte;

#ifdef LOAD_AND_SAVE

/* ROM Patching */
#define LOAD_START_4K       0x207       // POP DE           D1
#define SAVE_START_4K       0x1b7       // POP DE           D1
#define LOAD_SAVE_RET_4K    0x203       // POP HL           E1
#define LOAD_SAVE_RSTRT_4K  0x283

#define LOAD_START_8K       0x347       // RRC D            CB 10
#define SAVE_START_8K       0x2ff       // LD DE,$12CB      11
#define LOAD_SAVE_RET_8K    0x20A       // LD HL,$403B      21
#define LOAD_SAVE_RSTRT_8K  0x207

typedef struct
{
    uint16_t start;
    bool use_rom;
} RomPatch_T;

typedef struct
{
    RomPatch_T load;
    RomPatch_T save;
    uint16_t   retAddr;
    uint16_t   rstrtAddr;
} RomPatches_T;

extern RomPatches_T rom_patches;
#endif

/* SOUND board types */
#define SOUND_TYPE_NONE         0
#define SOUND_TYPE_QUICKSILVA   1
#define SOUND_TYPE_ZONX         2
#define SOUND_TYPE_CHROMA       3
#define SOUND_TYPE_VSYNC        4

#define MEMORYRAM_SIZE 0x10000

extern unsigned char mem[MEMORYRAM_SIZE];
extern unsigned char *memptr[64];
extern int memattr[64];
extern int sound_type;
extern unsigned long tstates,tsmax;
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
extern bool save_p(int name_addr, bool defer_rom);
extern bool load_p(int name_addr, bool defer_rom);

#ifdef __cplusplus
}
#endif
#endif

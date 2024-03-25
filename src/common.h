#ifndef _COMMON_H_
#define _COMMON_H_
#include <stdbool.h>
#include <stdint.h>

typedef unsigned char  byte;

/* AY board types */
#define AY_TYPE_NONE		0
#define AY_TYPE_QUICKSILVA	1
#define AY_TYPE_ZONX		2
extern int sound_ay;

/* Last instruction type */
#define LASTINSTNONE  0
#define LASTINSTINFE  1
#define LASTINSTOUTFE 2
#define LASTINSTOUTFD 3
#define LASTINSTOUTFF 4
extern int LastInstruction;

#define MEMORYRAM_SIZE 0x10000

extern unsigned char mem[MEMORYRAM_SIZE];
extern unsigned char *memptr[64];
extern int memattr[64];
extern unsigned long tstates,tsmax;
extern int vsync_visuals;
extern int ramsize;
extern int autoload;
extern int zx80;
extern bool m1not;
extern bool useWRX;
extern bool useQSUDG;
extern bool UDGEnabled;
extern bool LowRAM;
extern bool chr128;
extern bool useNTSC;
extern bool frameSync;
extern int adjustStartX;
extern int adjustStartY;
/* Chroma variables */
extern int chromamode;
extern unsigned char bordercolour;
extern unsigned char bordercolournew;

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int in(int h,int l);
extern unsigned int out(int h,int l,int a);
extern void save_p(int a);
extern void load_p(int a);

extern void setEmulatedTV(bool fiftyHz, uint16_t vtol);

#ifdef __cplusplus
}
#endif
#endif

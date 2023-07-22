#include <stdbool.h>
#include <stdint.h>

typedef unsigned char  byte;

// Screen size
#define DISPLAY_WIDTH       320
#define DISPLAY_HEIGHT      240
#define DISPLAY_BLANK       32   // Need this much padding to maintain 32 bit line alignment for DVI
#define DISPLAY_BLANK_BYTE  (DISPLAY_BLANK >> 3)
#define DISPLAY_STRIDE_BIT  (DISPLAY_WIDTH + DISPLAY_BLANK)
#define DISPLAY_STRIDE_BYTE (DISPLAY_STRIDE_BIT >> 3)

#define DISPLAY_START_X     40
#define DISPLAY_START_Y     24
#define DISPLAY_END_X       (DISPLAY_WIDTH + DISPLAY_START_X)
#define DISPLAY_END_Y       (DISPLAY_HEIGHT + DISPLAY_START_Y)
#define DISPLAY_PIXEL_OFF   6

#define DISPLAY_START_PIXEL (DISPLAY_START_X + DISPLAY_PIXEL_OFF)
#define DISPLAY_END_PIXEL   (DISPLAY_START_X + DISPLAY_PIXEL_OFF + DISPLAY_WIDTH)
#define DISPLAY_OFFSET      (-(DISPLAY_STRIDE_BIT * DISPLAY_START_Y) - (DISPLAY_START_X + DISPLAY_PIXEL_OFF) + DISPLAY_BLANK)

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

/* Default TV Vertical tolerance */
#define VTOL 100        // In scanlines

#define MEMORYRAM_SIZE 0x10000

extern unsigned char mem[MEMORYRAM_SIZE];
extern unsigned char *memptr[64];
extern unsigned char font[1024];
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
extern bool useNTSC;
extern int adjustStartX;
extern int adjustStartY;

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int in(int h,int l);
extern unsigned int out(int l,int a);
extern void save_p(int a);
extern void load_p(int a);
extern void bitbufBlit(unsigned char * buf);
extern void blankScreen();

extern unsigned char* getMenuBuffer(void);
extern void setTVRange(bool fiftyHz, uint16_t vtol);

#ifdef __cplusplus
}
#endif

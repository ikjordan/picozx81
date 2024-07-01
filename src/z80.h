/* z81/xz81, Linux console and X ZX81/ZX80 emulators.
 * Copyright (C) 1994 Ian Collier. z81 changes (C) 1995-2001 Russell Marks.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "common.h"
#include "sound.h"

/* Last instruction type */
#define LASTINSTNONE  0
#define LASTINSTINFE  1
#define LASTINSTOUTFE 2
#define LASTINSTOUTFD 3
#define LASTINSTOUTFF 4

extern unsigned long tstates,tsmax,frames;
extern int ay_reg;
extern int LastInstruction;
extern bool frameNotSync;

#ifdef __cplusplus
extern "C" {
#endif
extern void resetZ80(void);
extern void execZX81(void);
extern void execZX80(void);

extern void setDisplayBoundaries(void);
extern void setEmulatedTV(bool fiftyHz, uint16_t vtol);
#ifdef SUPPORT_CHROMA
void adjustChroma(bool start);
#endif

#ifdef __cplusplus
}
#endif

#define fetch(x) (memptr[(unsigned short)(x)>>10][(x)&0x3FF])
#define fetch2(x) ((fetch((x)+1)<<8)|fetch(x))
#define fetchm(x) (pc<0xC000 ? fetch(pc) : fetch(pc&0x7fff))

#define AY_STORE_CHECK(x,y) \
         if(sound_type==SOUND_TYPE_QUICKSILVA) {\
            switch(x){\
               case 0x7fff:\
                  ay_reg=((y)&0x0F); break;\
               case 0x7ffe:\
                  sound_ay_write(ay_reg,(y));\
            }\
         }\

#define QSUDG_STORE_CHECK(x,y) \
         if(useQSUDG) {\
            if (x>=0x8400 && x<0x8800){\
               mem[x] = y;\
               UDGEnabled = true;\
            }\
         }\

#define store(x,y) do {\
          unsigned short off=(x)&0x3FF;\
          unsigned char page=(unsigned short)(x)>>10;\
          int attr=memattr[page];\
          AY_STORE_CHECK(x,y) \
          QSUDG_STORE_CHECK(x,y) \
          if(attr){\
             memptr[page][off]=(y);\
             }\
           } while(0)

#define store2b(x,hi,lo) do {\
          unsigned short off=(x)&0x3FF;\
          unsigned char page=(unsigned short)(x)>>10;\
          int attr=memattr[page];\
          if(attr) { \
             memptr[page][off]=(lo);\
             memptr[page][off+1]=(hi);\
             }\
          } while(0)

#define store2(x,y) store2b(x,(y)>>8,(y)&0xFF)

#ifdef __GNUC__
inline static void storefunc(unsigned short ad,unsigned char b){
   store(ad,b);
}
#undef store
#define store(x,y) storefunc(x,y)

inline static void store2func(unsigned short ad,unsigned char b1,unsigned char b2){
   store2b(ad,b1,b2);
}
#undef store2b
#define store2b(x,hi,lo) store2func(x,hi,lo)
#endif

#define bc ((b<<8)|c)
#define de ((d<<8)|e)
#define hl ((h<<8)|l)


/* Emulation of the Z80 CPU with hooks into the other parts of z81.
 * Copyright (C) 1994 Ian Collier.
 * z81 changes (C) 1995-2001 Russell Marks.
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


#include <string.h>   /* for memset/memcpy */
#include "pico.h"     /* For not in flash */
#include "Z80.h"
#include <stdio.h>
#include "emuvideo.h"
#include "display.h"

#define parity(a) (partable[a])

unsigned char partable[256]={
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      0, 4, 4, 0, 4, 0, 0, 4, 4, 0, 0, 4, 0, 4, 4, 0,
      4, 0, 0, 4, 0, 4, 4, 0, 0, 4, 4, 0, 4, 0, 0, 4
   };

unsigned long tstates=0,tsmax=65000,frames=0;

static unsigned char* scrnbmp_new = 0;
static unsigned char* scrnbmpc_new = 0;

static int vsx=0;
static int vsy=0;
static int nrmvideo=1;
int ay_reg=0;
int LastInstruction;

// Horizontal line timings
#define HLENGTH       207 // TStates in horizontal scanline

// TV Emulation
#define SCAN50  310       // Number of scanline per frame at 50Hz
#define SCAN60  262       // Number of scanline per frame at 60Hz
#define HSCAN   (2*HLENGTH)
#define HTOL    30        // Tolerance in detection of horizontal sync

#define VMIN    170
#define HMIN    8
#define HMAX    32

static const int HSYNC_TOLERANCEMIN = HSCAN - HTOL;
static const int HSYNC_TOLERANCEMAX = HSCAN + HTOL;

static const int HSYNC_MINLEN = HMIN;
static const int HSYNC_MAXLEN = HMAX;
static const int VSYNC_MINLEN = VMIN;

static int VSYNC_TOLERANCEMIN = SCAN50 - 100;
static int VSYNC_TOLERANCEMAX = SCAN50 + 100;

static const int HSYNC_START = 16;
static const int HSYNC_END = 32;
static const int HLEN = HLENGTH;
static const int MAX_JMP = 8;

static int RasterX = 0;
static int RasterY = 0;
static int dest;

static int int_pending, nmi_pending, hsync_pending;
static int NMI_generator;
static int VSYNC_state, HSYNC_state, SYNC_signal;
static int psync, sync_len;
static int rowcounter=0;
static int hsync_counter=0;
static bool rowcounter_hold = false;

static void displayAndNewScreen(bool sync);
static inline void checkhsync(int tolchk);
static inline void checkvsync(int tolchk);
static inline void checksync(int inc);
static void anyout(void);
static void vsync_raise(void);
static void vsync_lower(void);
static inline int z80_interrupt(void);
static inline int nmi_interrupt(void);

int sound_ay = AY_TYPE_NONE;
bool m1not = false;
bool useWRX = false;
bool UDGEnabled = false;
bool useQSUDG = false;
bool LowRAM = false;
bool chr128 = false;
bool useNTSC = false;
bool frameSync = false;
int  adjustStartX=0;
int  adjustStartY=0;

void setEmulatedTV(bool fiftyHz, uint16_t vtol)
{
  // This can look confusing as we have an emulated display, and a real
  // display, both can be at either 50 or 60 Hz
  if (fiftyHz)
  {
    VSYNC_TOLERANCEMIN = SCAN50 - vtol;
    VSYNC_TOLERANCEMAX = SCAN50 + vtol;
  }
  else
  {
    VSYNC_TOLERANCEMIN = SCAN60 - vtol;
    VSYNC_TOLERANCEMAX = SCAN60 + vtol;
  }
}

static void __not_in_flash_func(displayAndNewScreen)(bool sync)
{
  // Display the current screen
  displayBuffer(scrnbmp_new, sync, true, (chromamode != 0));
  displayGetFreeBuffer(&scrnbmp_new);

  /* Need a chroma buffer ready in case it is switched on mid frame */
  displayGetChromaBuffer(&scrnbmpc_new, scrnbmp_new);

  // Clear the new screen - we have 3 buffers, so will not
  // have race with switching the screens to be displayed
  memset(scrnbmp_new, 0x00, disp.length);
  if (chromamode && (bordercolournew != bordercolour)) bordercolour = bordercolournew;
  if (chromamode) memset(scrnbmpc_new, (bordercolour << 4) + bordercolour, disp.length);
}

static void __not_in_flash_func(vsync_raise)(void)
{
  /* save current pos - in screen coords*/
  vsx=RasterX - (disp.start_x - adjustStartX);
  vsy=RasterY - (disp.start_y - adjustStartY);
}

/* for vsync on -> off */
static void __not_in_flash_func(vsync_lower)(void)
{
  int ny = RasterY - (disp.start_y - adjustStartY);
  int nx = RasterX - (disp.start_x - adjustStartX);

  // Can ignore if nx: ny pair larger than vsx: vsy pair and both all off screen
  if (((ny > vsy) || ((ny == vsy) && (nx >= vsx))) &&
      (((ny < 0) && (vsy < 0)) || ((ny >= disp.height) && (vsy >= disp.height))))
    return;

  // Something to display, so fit in display size
  if (vsy < 0)
  {
    vsy = 0;
    vsx = 0;
  }
  else if (vsy >= disp.height)
  {
    vsy = disp.height - 1;
    vsx = disp.width - 1;
  }

  if (ny < 0)
  {
    ny = 0;
    nx = 0;
  }
  else if (ny >= disp.height)
  {
    ny = disp.height - 1;
    nx = disp.width - 1;
  }

  if (vsx < 0)
    vsx = 0;
  else if (vsx >= disp.width)
    vsx = disp.width - 1;

  if (nx < 0)
    nx = 0;
  else if (nx >= disp.width)
    nx = disp.width - 1;

  if((ny < vsy) || ((ny == vsy) && (nx < vsx)))
  {
    /* must be wrapping around a frame edge; do bottom half */
    uint8_t* start = scrnbmp_new+vsy*disp.stride_byte+(vsx>>3);
    *start++ = (0xff >> (vsx & 0x7));
    memset(start, 0xff, disp.stride_byte*(disp.height-vsy)-(vsx>>3) -1);
    vsy=0;
    vsx=0;
  }

  uint8_t* start = scrnbmp_new+vsy*disp.stride_byte+(vsx>>3);
  uint8_t* end = scrnbmp_new+ny*disp.stride_byte+(nx>>3);

  if (end > start)
  {
    *start++ = (0xff >> (vsx & 0x7));
    // end bits?
    if (nx & 0x7)
    {
      *end = (0xff << (nx & 0x7));
    }

    if (end > start)
      memset(start, 0xff, end-start);
  }
  else
  {
    *start = (0xff >> (vsx & 0x7)) & (0xff << (nx & 0x7));
  }
}

unsigned char a, f, b, c, d, e, h, l;
unsigned char r, a1, f1, b1, c1, d1, e1, h1, l1, i, iff1, iff2, im;
unsigned short pc;
unsigned short ix, iy, sp;
unsigned char radjust;
unsigned char ixoriy, new_ixoriy;
unsigned char intsample=0;
unsigned char op;

void __not_in_flash_func(ExecZ80)(void)
{
  bool videodata;
  unsigned long ts;
  unsigned long tstore;
  unsigned char v=0;

  bool retval = false;
  unsigned char colour;

  do
  {
    v=0;
    ts = 0;
    LastInstruction = LASTINSTNONE;
    colour = (bordercolour << 4) + bordercolour;

    if(intsample && !((radjust-1)&64) && iff1)
      int_pending=1;

    if(nmi_pending)
    {
      ts = nmi_interrupt();
    }
    else if (int_pending)
    {
      ts = z80_interrupt();
      hsync_counter = -2;             /* INT ACK after two tstates */
      hsync_pending = 1;              /* a HSYNC may be started */
    }
    else
    {
      // Get the next op, calculate the next byte to display and execute the op
      op = fetchm(pc);

      if (m1not && pc<0xC000)
      {
        videodata = false;
      }
      else
      {
        videodata = (pc&0x8000) ? true: false;
      }

      if(videodata && !(op&64))
      {
        v=0xff;

        if ((i<0x20) || (i<0x40 && LowRAM && (!useWRX)))
        {
          int addr;
          if (chr128 && i>0x20 && i&1)
            addr = ((i&0xfe)<<8)|((((op&128)>>1)|(op&63))<<3)|rowcounter;
          else
            addr = ((i&0xfe)<<8)|((op&63)<<3)|rowcounter;

          if (UDGEnabled && addr>=0x1E00 && addr<0x2000)
          {
            v = font[addr-((op&128)?0x1C00:0x1E00)];
          }
          else
          {
            v = mem[addr];
          }
        }
        else
        {
          int addr = (i<<8)|(r&0x80)|(radjust&0x7f);
          if (useWRX)
          {
            v = mem[addr];
          }
        }
        v = (op&128)?~v:v;

        if (chromamode)
        {
            if (chromamode & 0x10)
                colour = fetch(pc);
            else
                colour = fetch(0xc000 | ((((op & 0x80) >> 1) | (op & 0x3f)) << 3) | rowcounter);
        }
        op=0; /* the CPU sees a nop */
      }
      tstore = tstates;

      do
      {
        pc++;
        radjust++;
        intsample=1;

        switch(op)
        {
#include "z80ops.h"
        }
        ixoriy=0;

        // Complete ix and iy instructions
        if (new_ixoriy)
        {
          ixoriy=new_ixoriy;
          new_ixoriy=0;
          op = fetchm(pc);
        }
      } while (ixoriy);

      ts = tstates - tstore;
      tstates = tstore;
    }

    nmi_pending = int_pending = 0;
    tstates += ts;

    switch(LastInstruction)
    {
      case LASTINSTOUTFD:
        NMI_generator = nmi_pending = 0;
        anyout();
      break;
      case LASTINSTOUTFE:
        if (!zx80)
        {
          NMI_generator=1;
        }
        anyout();
      break;
      case LASTINSTINFE:
        if (!NMI_generator)
        {
          if (VSYNC_state==0)
          {
            VSYNC_state = 1;
            vsync_raise();
          }
        }
      break;
      case LASTINSTOUTFF:
        anyout();
        if (zx80) hsync_pending=1;
      break;
    }

#if 0
    // Plot data in shift register
    // Note subtract 6 as this leaves the smallest positive number
    // of bits to carry to next byte (2)
    if (v &&
        (RasterX >= (disp.start_x - adjustStartX - 6)) &&
        (RasterX < (disp.end_x - adjustStartX)) &&
        (RasterY >= (disp.start_y - adjustStartY)) &&
        (RasterY < (disp.end_y - adjustStartY)))
    {
      int k = dest + RasterX;
      {
        int kh = k >> 3;
        int kl = k & 7;

        if (kl)
        {
          scrnbmp_new[kh++]|=(v>>kl);
          scrnbmp_new[kh]=(v<<(8-kl));
        }
        else
        {
          scrnbmp_new[kh]=v;
        }
      }
    }
#endif

    // Plot data in shift register
    // Note subtract 6 as this leaves the smallest positive number
    // of bits to carry to next byte (2)
    if ((v || chromamode) &&
        (RasterX >= (disp.start_x - adjustStartX - 6)) &&
        (RasterX < (disp.end_x - adjustStartX)) &&
        (RasterY >= (disp.start_y - adjustStartY)) &&
        (RasterY < (disp.end_y - adjustStartY)))
    {
      if (chromamode)
      {
        int k = (dest + RasterX) >> 3;
        scrnbmpc_new[k] = colour;
        scrnbmp_new[k] = v;
      }
      else
      {
        if (v)
        {
          int k = dest + RasterX;
          int kh = k >> 3;
          int kl = k & 7;

          if (kl)
          {
            scrnbmp_new[kh++] |= (v >> kl);
            scrnbmp_new[kh] = (v << (8 - kl));
          }
          else
          {
            scrnbmp_new[kh] = v;
          }
        }
      }
    }

    int tstate_inc;
    int states_remaining = ts;
    int since_hstart = 0;
    int tswait = 0;

    do
    {
      tstate_inc = states_remaining > MAX_JMP ? MAX_JMP: states_remaining;
      states_remaining -= tstate_inc;

      hsync_counter+=tstate_inc;
      RasterX += (tstate_inc<<1);

      if (hsync_counter >= HLEN)
      {
        hsync_counter -= HLEN;
        if (!zx80)
          hsync_pending = 1;
      }

      // Start of HSYNC, and NMI if enabled
      if (hsync_pending==1 && hsync_counter>=HSYNC_START)
      {
        if (NMI_generator)
        {
          nmi_pending = 1;
          if (ts==4)
          {
            tswait = 14 + (3-states_remaining - (hsync_counter - HSYNC_START));
          }
          else
          {
            tswait = 14;
          }
          states_remaining += tswait;
          ts += tswait;
          tstates += tswait;
        }

        HSYNC_state = 1;
        since_hstart = hsync_counter - HSYNC_START + 1;

        if (VSYNC_state || rowcounter_hold)
        {
          rowcounter = 0;
          rowcounter_hold = false;
        }
        else
        {
          rowcounter++;
          rowcounter &= 7;
        }
        hsync_pending = 2;
      }

      // end of HSYNC
      if (hsync_pending==2 && hsync_counter>=HSYNC_END)
      {
        if (VSYNC_state==2)
        {
          VSYNC_state = 0;
          vsync_lower();
        }
        HSYNC_state = 0;
        hsync_pending = 0;
      }

      // NOR the vertical and horizontal SYNC states to create the SYNC signal
      SYNC_signal = (VSYNC_state || HSYNC_state) ? 0 : 1;
      checksync(since_hstart ? since_hstart : MAX_JMP);
      since_hstart = 0;
    }
    while (states_remaining);

    if(tstates>=tsmax)
    {
      tstates-=tsmax;

      frames++;
      retval=true;
    }

  } while (!retval);
}

void ResetZ80(void)
{
  a=f=b=c=d=e=h=l=a1=f1=b1=c1=d1=e1=h1=l1=i=iff1=iff2=im=r=0;
  ixoriy=new_ixoriy=0;
  ix=iy=sp=pc=0;
  tstates=radjust=0;
  intsample=0;
  tstates=0;
  frames=0;
  vsy=0;
  nrmvideo=0;
  RasterX = 0;
  RasterY = 0;
  dest = disp.offset + (adjustStartY * disp.stride_bit) + adjustStartX;
  psync = 1;
  sync_len = 0;

  /* ULA */
  NMI_generator=0;
  int_pending=0;
  hsync_pending=0;
  VSYNC_state=HSYNC_state=0;

  emu_VideoSetInterlace();

  /* Ensure chroma is turned off */
  bordercolour=0x0f;
  bordercolournew=0x0f;
  chromamode = 0;
  displayResetChroma();

  if (!scrnbmp_new)
  {
    displayGetFreeBuffer(&scrnbmp_new);
  }

  if (scrnbmp_new)
  {
    memset(scrnbmp_new, 0x00, disp.length);

    /* Need a chroma buffer ready in case it is switched on mid frame */
    displayGetChromaBuffer(&scrnbmpc_new, scrnbmp_new);
  }

  if(autoload)
  {
    // Have already read the new specific parameters
    if (zx80) {
      /* Registers (common values) */
      a = 0x00; f = 0x44; b = 0x00; c = 0x00;
      d = 0x07; e = 0xae; h = 0x40; l = 0x2a;
      pc = 0x0283;
      ix = 0x0000; iy = 0x4000; i = 0x0e; r = 0xdd;
      a1 = 0x00; f1 = 0x00; b1 = 0x00; c1 = 0x21;
      d1 = 0xd8; e1 = 0xf0; h1 = 0xd8; l1 = 0xf0;
      iff1 = 0x00; iff2 = 0x00; im = 0x02;
      radjust = 0x6a;
      /* Machine Stack (common values) */
      if (ramsize >= 16) {
        sp = 0x8000 - 4;
      } else {
        sp = 0x4000 - 4 + ramsize * 1024;
      }
      mem[sp + 0] = 0x47;
      mem[sp + 1] = 0x04;
      mem[sp + 2] = 0xba;
      mem[sp + 3] = 0x3f;
      /* Now override if RAM configuration changes things
       * (there's a possibility these changes are unimportant) */
      if (ramsize == 16) {
        mem[sp + 2] = 0x22;
      }
    } else {
      /* Registers (common values) */
      a = 0x0b; f = 0x00; b = 0x00; c = 0x02;
      d = 0x40; e = 0x9b; h = 0x40; l = 0x99;
      pc = 0x0207;
      ix = 0x0281; iy = 0x4000; i = 0x1e; r = 0xdd;
      a1 = 0xf8; f1 = 0xa9; b1 = 0x00; c1 = 0x00;
      d1 = 0x00; e1 = 0x2b; h1 = 0x00; l1 = 0x00;
      iff1 = 0; iff2 = 0; im = 2;
      radjust = 0xa4;
      /* GOSUB Stack (common values) */
      if (ramsize >= 16) {
        sp = 0x8000 - 4;
      } else {
        sp = 0x4000 - 4 + ramsize * 1024;
      }
      mem[sp + 0] = 0x76;
      mem[sp + 1] = 0x06;
      mem[sp + 2] = 0x00;
      mem[sp + 3] = 0x3e;
      /* Now override if RAM configuration changes things,
         without these changes Multi-scroll sometimes fail to start */
      if (ramsize >= 4) {
        d = 0x43; h = 0x43;
        a1 = 0xec; b1 = 0x81; c1 = 0x02;
        radjust = 0xa9;
      }
      /* System variables */
      mem[0x4000] = 0xff;				/* ERR_NR */
      mem[0x4001] = 0x80;				/* FLAGS */
      mem[0x4002] = sp & 0xff;		/* ERR_SP lo */
      mem[0x4003] = sp >> 8;			/* ERR_SP hi */
      mem[0x4004] = (sp + 4) & 0xff;	/* RAMTOP lo */
      mem[0x4005] = (sp + 4) >> 8;	/* RAMTOP hi */
      mem[0x4006] = 0x00;				/* MODE */
      mem[0x4007] = 0xfe;				/* PPC lo */
      mem[0x4008] = 0xff;				/* PPC hi */
    }

    /* finally, load. It'll reset (via reset81) if it fails. */
    load_p(32768);
  }
}

static inline int __not_in_flash_func(z80_interrupt)(void)
{
  int tinc=0;

  if(iff1)
  {
    if(fetchm(pc)==0x76)
    {
      pc++;
    }
    iff1=iff2=0;
    push2(pc);
    radjust++;

    switch(im)
    {
      case 0: /* IM 0 */
      case 2: /* IM 1 */
        pc=0x38;
        tinc=13;
      break;
      case 3: /* IM 2 */
      {
        int addr=fetch2((i<<8)|0xff);
        pc=addr;
        tinc=19;
      }
      break;

      default:
        tinc=12;
      break;
    }
  }
  return tinc;
}

static inline int __not_in_flash_func(nmi_interrupt)(void)
{
  iff1=0;
  if(fetchm(pc)==0x76)
  {
    pc++;
  }
  push2(pc);
  radjust++;
  pc=0x66;

  return 11;
}

/* Normally, these sync checks are done by the TV :-) */
static inline void __not_in_flash_func(checkhsync)(int tolchk)
{
  if ( ( !tolchk && sync_len >= HSYNC_MINLEN && sync_len < (HSYNC_MAXLEN + MAX_JMP) && RasterX>=HSYNC_TOLERANCEMIN )  ||
       (  tolchk &&                                                                    RasterX>=HSYNC_TOLERANCEMAX ) )
  {
    if (zx80)
      RasterX = (hsync_counter - HSYNC_END) << 1;
    else
      RasterX = ((hsync_counter - HSYNC_END) < MAX_JMP) ? ((hsync_counter - HSYNC_END) << 1) : 0;
    RasterY++;
    dest += disp.stride_bit;
  }
}

static inline void __not_in_flash_func(checkvsync)(int tolchk)
{
  if ( ( !tolchk && sync_len >= VSYNC_MINLEN && RasterY>=VSYNC_TOLERANCEMIN ) ||
       (  tolchk &&                             RasterY>=VSYNC_TOLERANCEMAX ) )
  {
    if (sync_len>(int)tsmax)
    {
      // If there has been no sync for an entire frame then blank the screen
      displayBlank(true);
      sync_len = 0;
    }
    else
    {
      displayAndNewScreen(frameSync);
    }
    RasterY = 0;
    dest = disp.offset + (disp.stride_bit * adjustStartY) + adjustStartX;
  }
}

static inline void __not_in_flash_func(checksync)(int inc)
{
  if (!SYNC_signal)
  {
    if (psync==1)
      sync_len = 0;
    sync_len += inc;
    checkhsync(1);
    checkvsync(1);
  }
  else
  {
    if (!psync)
    {
      checkhsync(0);
      checkvsync(0);
    }
  }
  psync = SYNC_signal;
}

/* The rowcounter is a 7493; as long as both reset inputs are high, the counter is at zero
   and cannot count. Any out sets it free. */

static void anyout(void)
{
  if (VSYNC_state)
  {
    if (zx80)
      VSYNC_state = 2; // will be reset by HSYNC circuitry
    else
    {
      VSYNC_state = 0;
      // A fairly arbitrary value selected after comparing with a "real" ZX81
      if ((hsync_counter < HSYNC_START) || (hsync_counter >= 178) )
      {
        rowcounter_hold = true;
      }
      vsync_lower();
    }
  }
}

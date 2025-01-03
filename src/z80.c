/* Emulation of the Z80 CPU with hooks into the other parts of z81.
 * Copyright (C) 1994 Ian Collier.
 * z81 changes (C) 1995-2001 Russell Marks.
 * picozx development (C) 2023-2024 Ian Jordan
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


#include <string.h>   /* for memset */
#include "pico.h"     /* For not in flash */
#include "z80.h"
#include <stdio.h>
#include "emuvideo.h"
#include "emusound.h"
#include "display.h"

#define parity(a) (partable[a])

unsigned char partable[256] = {
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

unsigned long tstates = 0;
const unsigned long tsmax = 65000;
static unsigned long ts = 0;

static unsigned char* scrnbmp_new = 0;
#ifdef SUPPORT_CHROMA
static unsigned char* scrnbmpc_new = 0;
#endif

static int vsx = 0;
static int vsy = 0;
static int nrmvideo = 1;
int ay_reg = 0;
int LastInstruction;
bool frameNotSync = true;

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
static int FRAME_SCAN = SCAN50;

static const int HSYNC_MINLEN = HMIN;
static const int HSYNC_MAXLEN = HMAX;
static const int VSYNC_MINLEN = VMIN;

static int VSYNC_TOLERANCEMIN = SCAN50 - VTOL;
static int VSYNC_TOLERANCEMAX = SCAN50 + VTOL;

static const int HSYNC_START = 16;
static const int HSYNC_END = 32;
static const int HLEN = HLENGTH;
static const int MAX_JMP = 8;

static int RasterX = 0;
static int RasterY = 0;
static int dest;

static int adjustStartX = 0;
static int adjustStartY = 0;
static int startX = 0;
static int startY = 0;
static int syncX = 0;
static int endX = 0;
static int endY = 0;

static int nmi_pending, hsync_pending;
static int NMI_generator;
static int VSYNC_state, HSYNC_state, SYNC_signal;
static int psync, sync_len;
static int rowcounter = 0;
static int hsync_counter = 0;
static bool rowcounter_hold = false;

static void setRemainingDisplayBoundaries(void);
static void displayAndNewScreen(bool sync);
static inline void checkhsync(int tolchk);
static inline void checkvsync(int tolchk);
static inline void checksync(int inc);
static void anyout(void);
static void vsync_raise(void);
static void vsync_lower(void);
static inline int z80_interrupt(void);
static inline int nmi_interrupt(void);
static unsigned long z80_op(void);

#ifdef LOAD_AND_SAVE
static void loadAndSaveROM(void);
#endif

int sound_type = SOUND_TYPE_NONE;
bool m1not = false;
bool useWRX = false;
bool UDGEnabled = false;
bool useQSUDG = false;
bool LowRAM = false;
bool chr128 = false;
bool useNTSC = false;
bool frameSync = false;
bool running_rom = false;

unsigned char a, f, b, c, d, e, h, l;
unsigned char r, a1, f1, b1, c1, d1, e1, h1, l1, i, iff1, iff2, im;
unsigned short pc;
unsigned short ix, iy, sp;
unsigned char radjust;
unsigned char ixoriy, new_ixoriy;
unsigned char intsample = 0;
unsigned char op;
unsigned short m1cycles;

/* ZX80 specific */
#define SYNCNONE        0
#define SYNCTYPEH       1
#define SYNCTYPEV       2

static int S_RasterX = 0;
static int S_RasterY = 0;

static int scanlineCounter = 0;

static int videoFlipFlop1Q = 1;
static int videoFlipFlop2Q = 0;
static int videoFlipFlop3Q = 0;
static int videoFlipFlop3Clear = 0;
static int prevVideoFlipFlop3Q = 0;

static int lineClockCarryCounter = 0;

static int scanline_len = 0;
static int sync_type = SYNCNONE;
static int nosync_lines = 0;
static bool vsyncFound = false;

static const int scanlinePixelLength = (HLENGTH << 1);
static const int ZX80HSyncDuration = 20;

static const int ZX80HSyncAcceptanceDuration = (3 * ZX80HSyncDuration) / 2;
static const int ZX80HSyncAcceptanceDurationPixels = ZX80HSyncAcceptanceDuration * 2;
static const int ZX80MaximumSupportedScanlineOverhang = ZX80HSyncDuration * 2;
static const int ZX80MaximumSupportedScanlineOverhangPixels = ZX80MaximumSupportedScanlineOverhang * 2;

static const int PortActiveDuration = 3;
static const int ZX80HSyncAcceptancePixelPosition = scanlinePixelLength - ZX80HSyncAcceptanceDurationPixels;
static const int scanlineThresholdPixelLength = scanlinePixelLength + ZX80MaximumSupportedScanlineOverhangPixels;

void setEmulatedTV(bool fiftyHz, uint16_t vtol)
{
  // This can look confusing as we have an emulated display, and a real
  // display, both can be at either 50 or 60 Hz
  if (fiftyHz)
  {
    VSYNC_TOLERANCEMIN = SCAN50 - vtol;
    VSYNC_TOLERANCEMAX = SCAN50 + vtol;
    FRAME_SCAN = SCAN50;
  }
  else
  {
    VSYNC_TOLERANCEMIN = SCAN60 - vtol;
    VSYNC_TOLERANCEMAX = SCAN60 + vtol;
    FRAME_SCAN = SCAN60;
  }
}

void setDisplayBoundaries(void)
{
  adjustStartX = emu_CentreX();
  setRemainingDisplayBoundaries();
}

static void setRemainingDisplayBoundaries(void)
{
  adjustStartY = emu_CentreY();
  startX = disp.start_x - adjustStartX - 6;
  startY = disp.start_y - adjustStartY;
  syncX = disp.start_x - adjustStartX;
  endX = disp.end_x - adjustStartX;
  endY = disp.end_y - adjustStartY;
}

#ifdef SUPPORT_CHROMA
/* Ensure that chroma and pixels are byte aligned */
void adjustChroma(bool start)
{
    adjustStartX = start ? disp.adjust_x + (zx80 ? 8 : 0) : emu_CentreX();
    setRemainingDisplayBoundaries();
}
#endif

void resetZ80(void)
{
  a = f = b = c = d = e = h = l = 0;
  a1 = f1 = b1 = c1 = d1 = e1 = h1 = l1 = i = iff1 = iff2 = im = r = 0;
  ixoriy = new_ixoriy = 0;
  ix= iy = sp = pc = 0;
  radjust = 0;
  intsample = 0;
  m1cycles = 0;
  tstates = 0;
  ts = 0;
  vsx = vsy = 0;
  nrmvideo = 0;
  RasterX = 0;
  RasterY = 0;
  psync = 1;
  sync_len = 0;
  running_rom = false;
  frameNotSync = true;
  LastInstruction = LASTINSTNONE;

  /* ULA */
  NMI_generator = 0;
  hsync_pending = 0;
  VSYNC_state = HSYNC_state = 0;

  setDisplayBoundaries();
  dest = disp.offset + (adjustStartY * disp.stride_bit) + adjustStartX;

  S_RasterX = 0;
  S_RasterY = 0;

  /* ZX80 Hardware */
  videoFlipFlop1Q = 1;
  videoFlipFlop2Q = 0;
  videoFlipFlop3Q = 0;
  videoFlipFlop3Clear = 0;
  prevVideoFlipFlop3Q = 0;

  vsyncFound = false;
  lineClockCarryCounter = 0;

  scanline_len = 0;
  sync_type = SYNCNONE;
  sync_len = 0;
  nosync_lines = 0;

  emu_VideoSetInterlace();

  /* Ensure chroma is turned off */
  chromamode = 0;
#ifdef SUPPORT_CHROMA
  bordercolour = 0x0f;
  bordercolournew = 0x0f;
  displayResetChroma();
#endif

  if (!scrnbmp_new)
  {
    displayGetFreeBuffer(&scrnbmp_new);
  }

  if (scrnbmp_new)
  {
    memset(scrnbmp_new, 0x00, disp.length);

#ifdef SUPPORT_CHROMA
    /* Need a chroma buffer ready in case it is switched on mid frame */
    displayGetChromaBuffer(&scrnbmpc_new, scrnbmp_new);
#endif
  }

  if(autoload)
  {
    // Have already read the new specific parameters
    if (rom4k)
    {
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
      sp =  (ramsize >= 16) ? (0x8000 - 4) : (0x4000 - 4 + ramsize * 1024);

      mem[sp + 0] = 0x47;
      mem[sp + 1] = 0x04;
      mem[sp + 2] = (ramsize >= 16) ? 0x22 : 0xba;
      mem[sp + 3] = 0x3f;
    }
    else
    {
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
      sp = (ramsize >= 16) ? (0x8000 - 4) : (0x4000 - 4 + ramsize * 1024);

      mem[sp + 0] = 0x76;
      mem[sp + 1] = 0x06;
      mem[sp + 2] = 0x00;
      mem[sp + 3] = 0x3e;

      /* Update values for larger RAM sizes,
         without these changes Multi-scroll sometimes fails to start */
      if (ramsize >= 4)
      {
        d = 0x43; h = 0x43;
        a1 = 0xec; b1 = 0x81; c1 = 0x02;
        radjust = 0xa9;
      }

      /* System variables */
      mem[0x4000] = 0xff;               /* ERR_NR */
      mem[0x4001] = 0x80;;              /* FLAGS */
      mem[0x4002] = sp & 0xff;;         /* ERR_SP lo */
      mem[0x4003] = sp >> 8;            /* ERR_SP hi */
      mem[0x4004] = (sp + 4) & 0xff;    /* RAMTOP lo */
      mem[0x4005] = (sp + 4) >> 8;      /* RAMTOP hi */
      mem[0x4006] = 0x00;               /* MODE */
      mem[0x4007] = 0xfe;               /* PPC lo */
      mem[0x4008] = 0xff;               /* PPC hi */
    }

    /* finally, load. Reset (via resetZ80) occurs if load fails. */
    load_p(0x8000, false);
  }
}

#ifdef LOAD_AND_SAVE
static void __not_in_flash_func(loadAndSaveROM)(void)
{
  static int sound_cache;

  if (!running_rom)
  {
    if (pc == rom_patches.load.start) // load
    {
      if (!load_p(rom4k ? hl : de, rom_patches.load.use_rom))
      {
        pc = rom_patches.rstrtAddr;
      }
      else
      {
        running_rom = true;
      }
    }
    else if (pc == rom_patches.save.start) // save
    {
      if (!save_p(hl, rom_patches.save.use_rom))
      {
        pc = rom_patches.rstrtAddr;
      }
      else
      {
        running_rom = true;
      }
    }

    if (running_rom)
    {
      // Run the ROM, generating VSYNC sound
      sound_cache = sound_type;
      if (sound_type != SOUND_TYPE_VSYNC)
      {
        sound_type = SOUND_TYPE_VSYNC;
        emu_sndInit(true, false);
      }
    }
  }
  else
  {
    if (pc == rom_patches.retAddr)
    {
      // Restore the sound mode
      if (sound_cache != sound_type)
      {
        emu_sndQueueChange(sound_cache != SOUND_TYPE_NONE, sound_cache);
      }
      running_rom = false;
    }
  }
}
#endif

static void __not_in_flash_func(displayAndNewScreen)(bool sync)
{
  // Display the current screen
  displayBuffer(scrnbmp_new, sync, true, (chromamode != 0));
  displayGetFreeBuffer(&scrnbmp_new);

#ifdef SUPPORT_CHROMA
  /* Need a chroma buffer ready in case it is switched on mid frame */
  displayGetChromaBuffer(&scrnbmpc_new, scrnbmp_new);
#endif
  // Clear the new screen - we have 3 buffers, so will not
  // have race with switching the screens to be displayed
  memset(scrnbmp_new, 0x00, disp.length);
#ifdef SUPPORT_CHROMA
  if (chromamode && (bordercolournew != bordercolour))
  {
    bordercolour = bordercolournew;
    fullcolour = (bordercolour << 4) + bordercolour;
  }
  if (chromamode) memset(scrnbmpc_new, fullcolour, disp.length);
#endif
}

static void __not_in_flash_func(vsync_raise)(void)
{
  /* save current pos - in screen coords*/
  vsx = RasterX - syncX + (ts << 1);
  vsy = RasterY - startY;

  // move to next valid pixel
  if (vsx >= disp.width)
  {
    vsx = 0;
    vsy++;
  }
  else if (vsx < 0)
  {
    vsx = 0;
  }

  if ((vsy < 0) || (vsy >= disp.height))
  {
    vsx = 0;
    vsy = 0;
  }
}

/* for vsync on -> off */
static void __not_in_flash_func(vsync_lower)(void)
{
  int nx = RasterX - syncX + (ts << 1);
  int ny = RasterY - startY;

  // Move to the next valid pixel
  if (nx >= disp.width)
  {
    nx = 0;
    ny++;
  }
  else if (nx < 0)
  {
    nx = 0;
  }

  if ((ny < 0) || (ny >= disp.height))
  {
    nx = 0;
    ny = 0;
  }

  // leave if start and end are same pixel
  if ((nx == vsx) && (ny == vsy)) return;

  // Determine if there is a frame wrap
  if((ny < vsy) || ((ny == vsy) && (nx < vsx)))
  {
    // wrapping around frame, so display bottom
    uint8_t* start = scrnbmp_new + vsy * disp.stride_byte + (vsx >> 3) - 1;
    *start++ = (0xff >> (vsx & 0x7));
    memset(start, 0xff, disp.stride_byte * (disp.height - vsy) - (vsx >> 3) - 1);

    // check for case where wrap ends at bottom
    if ((nx == 0) && (ny == 0)) return;

    // Fall through to display top half
    vsx = 0;
    vsy = 0;
  }

  uint8_t* start = scrnbmp_new + vsy * disp.stride_byte + (vsx >> 3) - 1;
  uint8_t* end = scrnbmp_new + ny * disp.stride_byte + (nx >> 3);
  *start++ = (0xff >> (vsx & 0x7));

  // end bits?
  if (nx & 0x7)
  {
    *end = (0xff << (nx & 0x7));
  }

  // Note: End equalling start is not unusual after adjusting positions to be on screen
  // especially when displaying the loading screen
  if (end > start)
  {
    memset(start, 0xff, end - start);
  }
}

void __not_in_flash_func(execZX81)(void)
{
  do
  {
    if(nmi_pending)
    {
      ts = nmi_interrupt();
      tstates += ts;
      nmi_pending = 0;
    }
    else if (iff1 && intsample && !((radjust - 1) & 0x40))
    {
      ts = z80_interrupt();
      hsync_counter = -2;             /* INT ACK after two tstates */
      hsync_pending = 1;              /* a HSYNC may be started */
      tstates += ts;
    }
    else
    {
      // Get the next op, calculate the next byte to display and execute the op
      op = fetchm(pc);

      // After this instruction can have interrupt
      intsample = 1;

      if (((pc & 0x8000) && (!m1not || (pc & 0x4000)) && !(op & 0x40)))
      {
        if ((RasterX >= startX) &&
            (RasterX < endX) &&
            (RasterY >= startY) &&
            (RasterY < endY))
        {
          unsigned char v;
          int addr;

          if ((i < 0x20) || ((i < 0x40) && LowRAM && (!useWRX)))
          {
            if (!(chr128 && (i > 0x20) && (i & 1)))
              addr = ((i & 0xfe) << 8) | ((op & 0x3f) << 3) | rowcounter;
            else
              addr = ((i & 0xfe) << 8) | ((((op & 0x80) >> 1) | (op & 0x3f)) << 3) | rowcounter;

            if (UDGEnabled && (addr >= 0x1E00) && (addr < 0x2000))
            {
              v = mem[addr + ((op & 0x80) ? 0x6800 : 0x6600)];
            }
            else
            {
              v = mem[addr];
            }
          }
          else if (useWRX)
          {
            v = mem[(i << 8) | (r & 0x80) | (radjust & 0x7f)];
          }
          else
          {
            v = 0xff;
          }
          v = (op & 0x80) ? ~v : v;

#ifdef SUPPORT_CHROMA
          if (chromamode)
          {
            int k = (dest + RasterX) >> 3;
            scrnbmpc_new[k] = (chromamode & 0x10) ? fetch(pc) : fetch(0xc000 | ((((op & 0x80) >> 1) | (op & 0x3f)) << 3) | rowcounter);
            scrnbmp_new[k] = v;
          }
          else
#endif
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
        /* The CPU sees a nop - so skip the Z80 emulation loop */
        pc++;
        radjust++;

        ts = 4;
        tstates += ts;
      }
      else
      {
        ts = z80_op();

        switch(LastInstruction)
        {
          case LASTINSTOUTFD:
            NMI_generator = 0;
            anyout();
          break;

          case LASTINSTOUTFE:
            NMI_generator = 1;
            anyout();
          break;

          case LASTINSTINFE:
            if (!NMI_generator)
            {
              if (VSYNC_state == 0)
              {
                VSYNC_state = 1;
                vsync_raise();
              }
            }
            LastInstruction = LASTINSTNONE;
          break;

          case LASTINSTOUTFF:
            anyout();
          break;
        }
      }
    }

    // Determine changes to sync state
    int states_remaining = ts;
    int since_hstart = 0;
    int tswait = 0;
    int tstate_inc;

    do
    {
      tstate_inc = states_remaining > MAX_JMP ? MAX_JMP: states_remaining;
      states_remaining -= tstate_inc;

      hsync_counter += tstate_inc;
      RasterX += (tstate_inc << 1);

      if (hsync_counter >= HLEN)
      {
          hsync_counter -= HLEN;
          hsync_pending = 1;
      }

      // Start of HSYNC, and NMI if enabled
      if ((hsync_pending == 1) && (hsync_counter >= HSYNC_START))
      {
        if (NMI_generator)
        {
          nmi_pending = 1;
          if (ts == 4)
          {
            tswait = 14 + (3 - states_remaining - (hsync_counter - HSYNC_START));
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
      if ((hsync_pending == 2) && (hsync_counter >= HSYNC_END))
      {
        HSYNC_state = 0;
        hsync_pending = 0;
      }

      // NOR the vertical and horizontal SYNC states to create the SYNC signal
      SYNC_signal = (VSYNC_state || HSYNC_state) ? 0 : 1;
      checksync(since_hstart ? since_hstart : MAX_JMP);
      since_hstart = 0;
    }
    while (states_remaining);
  }
  while (tstates < tsmax);

  tstates -= tsmax;
}

void __not_in_flash_func(execZX80)(void)
{
  unsigned long ts;     // deliberately hides static ts, so vsync at correct raster

  do
  {
    // Get the next op, calculate the next byte to display and execute the op
    op = fetchm(pc);

    intsample = 1;
    m1cycles = 1;

    if (((pc & 0x8000) && (!m1not || (pc & 0x4000)) && !(op & 0x40)))
    {
      if ((RasterX >= startX) &&
          (RasterX < endX) &&
          (RasterY >= startY) &&
          (RasterY < endY))
      {
        unsigned char v;
        int addr;

        if ((i < 0x20) || ((i < 0x40) && LowRAM && (!useWRX)))
        {
          if (!(chr128 && (i > 0x20) && (i & 1)))
            addr = ((i & 0xfe) << 8) | ((op & 0x3f) << 3) | rowcounter;
          else
            addr = ((i & 0xfe) << 8) | ((((op & 0x80) >> 1) | (op & 0x3f)) << 3) | rowcounter;

          if (UDGEnabled && (addr >= 0x1E00) && (addr < 0x2000))
          {
            v = mem[addr + ((op & 0x80) ? 0x6800 : 0x6600)];
          }
          else
          {
            v = mem[addr];
          }
        }
        else if (useWRX)
        {
          v = mem[(i << 8) | (r & 0x80) | (radjust & 0x7f)];
        }
        else
        {
          v = 0xff;
        }
        v = (op & 0x80) ? ~v : v;

#ifdef SUPPORT_CHROMA
        if (chromamode)
        {
          int k = (dest + RasterX) >> 3;
          scrnbmpc_new[k] = (chromamode & 0x10) ? fetch(pc) : fetch(0xc000 | ((((op & 0x80) >> 1) | (op & 0x3f)) << 3) | rowcounter);
          scrnbmp_new[k] = v;
        }
        else
#endif
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

      /* The CPU sees a nop - so skip the Z80 emulation loop */
      pc++;
      radjust++;

      ts = 4;
      tstates += ts;

      // Update the flip flop
      prevVideoFlipFlop3Q = videoFlipFlop3Q;

      if (videoFlipFlop3Clear)
      {
        videoFlipFlop3Q = videoFlipFlop2Q;
      }
      videoFlipFlop2Q = !videoFlipFlop1Q;
    }
    else
    {
      ts = z80_op();

      // Update the flip flop
      prevVideoFlipFlop3Q = videoFlipFlop3Q;

      for (int i = 0; i < m1cycles; ++i)
      {
        if (videoFlipFlop3Clear)
        {
          videoFlipFlop3Q = videoFlipFlop2Q;
        }

        videoFlipFlop2Q = !videoFlipFlop1Q;
      }
    }

    if (!videoFlipFlop3Q)
    {
      videoFlipFlop1Q = 0;

      if (prevVideoFlipFlop3Q)
      {
        rowcounter = (rowcounter + 1) & 7;
      }
    }

    // execute an interrupt
    if (iff1 && intsample && !((radjust - 1) & 0x40))
    {
      unsigned long tstore = z80_interrupt();
      tstates += tstore;
      ts += tstore;

      // single m1Cycle
      if (videoFlipFlop3Clear)
      {
        videoFlipFlop3Q = videoFlipFlop2Q;
      }

      videoFlipFlop2Q = !videoFlipFlop1Q;
      videoFlipFlop1Q = 1;
    }

    RasterX += (ts << 1);
    scanline_len += (ts << 1);
    if (RasterX >= scanlinePixelLength)
    {
      RasterX -= scanlinePixelLength;
      RasterY++;
    }

    switch (LastInstruction)
    {
      case LASTINSTOUTFD:
      case LASTINSTOUTFE:
      case LASTINSTOUTFF:     // VSync end
        videoFlipFlop1Q = 0;
        videoFlipFlop2Q = 1;
        videoFlipFlop3Clear = 1;
        if (!videoFlipFlop3Q)
        {
          sync_len += ts;

          if (sync_len > ZX80HSyncAcceptanceDuration)
          {
            videoFlipFlop3Q = 1;
          }
        }
        LastInstruction = LASTINSTNONE;
      break;

      case LASTINSTINFE:      // VSync start
        if (videoFlipFlop3Q)
        {
          sync_len = PortActiveDuration;
        }
        else
        {
          sync_len += ts;
        }

        videoFlipFlop1Q = 1;
        videoFlipFlop3Clear = 0;
        videoFlipFlop3Q = 0;
        rowcounter = 0;
        LastInstruction = LASTINSTNONE;
      break;

      default:
        if (!videoFlipFlop3Q)
        {
          sync_len += ts;
        }
      break;
    }

    if (prevVideoFlipFlop3Q != videoFlipFlop3Q)
    {
      videoFlipFlop3Q ? vsync_lower() : vsync_raise();
      // ZX80 HSYNC sound - excluded if Chroma
      if (sound_type == SOUND_TYPE_VSYNC) sound_beeper(videoFlipFlop3Q);
    }

    if (videoFlipFlop3Q && (sync_len > 0))
    {
      // The line is now complete
      if (sync_len <= ZX80HSyncAcceptanceDuration)
      {
        sync_type = SYNCTYPEH;
        if (scanline_len >= ZX80HSyncAcceptancePixelPosition)
        {
          lineClockCarryCounter = ts;
          scanline_len = scanlinePixelLength;
        }
      }
      else
      {
        int overhangPixels = scanline_len - scanlinePixelLength;
        sync_type = SYNCTYPEV;

        if (overhangPixels < 0)
        {
          if (scanline_len >= ZX80HSyncAcceptancePixelPosition)
          {
            lineClockCarryCounter = 0;
          }
          else
          {
            lineClockCarryCounter = scanline_len > 1;
          }
          scanline_len = scanlinePixelLength;
        }
        else if (overhangPixels > 0)
        {
          lineClockCarryCounter = overhangPixels > 1;
          scanline_len = scanlinePixelLength;
        }
      }
    }

    // If we are at the end of a line then process it
    if (!((scanline_len < scanlineThresholdPixelLength) && (sync_type == SYNCNONE)))
    {
      if (sync_type == SYNCTYPEV)
      {
        // Frames synchonised after second vsync in range
        if (vsyncFound)
        {
          frameNotSync = !((RasterY >= VSYNC_TOLERANCEMIN) && (RasterY < VSYNC_TOLERANCEMAX) &&
                          (scanlineCounter >= VSYNC_TOLERANCEMIN) && (scanlineCounter < VSYNC_TOLERANCEMAX));
          vsyncFound = !frameNotSync;
        }
        else
        {
          vsyncFound = (scanlineCounter >= VSYNC_TOLERANCEMIN) && (scanlineCounter < VSYNC_TOLERANCEMAX);
        }
        scanlineCounter = 0;

        if (!vsyncFound)
        {
          sync_type = SYNCNONE;
          sync_len = 0;
        }
        nosync_lines = 0;
      }
      else
      {
        if (sync_type == SYNCTYPEH)
        {
          scanlineCounter++;
        }

        if (((sync_type == SYNCNONE) && videoFlipFlop3Q) || (scanlineCounter >= VSYNC_TOLERANCEMAX))
        {
          frameNotSync = true;
          vsyncFound = false;
        }

        if (sync_type == SYNCNONE)
        {
          int overhangPixels = scanline_len - scanlinePixelLength;
          if (overhangPixels > 0)
          {
            lineClockCarryCounter = (overhangPixels >> 1);
            scanline_len = scanlinePixelLength;
          }
          nosync_lines++;
        }
        else
        {
          nosync_lines = 0;
        }
      }

      // Synchronise the TV position
      S_RasterX += scanline_len;
      if (S_RasterX >= scanlinePixelLength)
      {
        S_RasterX -= scanlinePixelLength;
        S_RasterY++;

        if (S_RasterY >= VSYNC_TOLERANCEMAX)
        {
          S_RasterX = 0;
          sync_type = SYNCTYPEV;
          if (sync_len < HSYNC_MINLEN)
          {
            sync_len = HSYNC_MINLEN;
          }
        }
      }

      if (sync_len < HSYNC_MINLEN) sync_type = 0;

      if (sync_type)
      {
        if (S_RasterX > HSYNC_TOLERANCEMAX)
        {
          S_RasterX = 0;
          S_RasterY++;
        }

        if (((sync_len > VSYNC_MINLEN) && (S_RasterY > VSYNC_TOLERANCEMIN)) ||
             (S_RasterY >= VSYNC_TOLERANCEMAX))
        {
          if (nosync_lines >= FRAME_SCAN)
          {
            // Whole frame with no sync, so blank the display
            displayBlank(true);
            nosync_lines -= FRAME_SCAN;
          }
          else
          {
            displayAndNewScreen(frameSync);
          }
          S_RasterX = 0;
          S_RasterY = 0;
        }
      }

      if (sync_type != SYNCNONE)
      {
        sync_type = SYNCNONE;
        sync_len = 0;
      }

      // Handle line carry over here
      if (lineClockCarryCounter > 0)
      {
        scanline_len = lineClockCarryCounter << 1;
        lineClockCarryCounter = 0;
      }
      else
      {
        scanline_len = 0;
      }

      // Update data for new ZX80 scanline
      RasterX = S_RasterX + scanline_len;
      RasterY = S_RasterY;
      dest = disp.offset + (disp.stride_bit * (adjustStartY + RasterY)) + adjustStartX;
    }
  }
  while (tstates < tsmax);

  tstates -= tsmax;
}

static unsigned long z80_op(void)
{
  unsigned long tstore = tstates;

  do
  {
    pc++;
    radjust++;

    switch (op)
    {
#include "z80ops.h"
    }
    ixoriy = 0;

    // Complete ix and iy instructions
    if (new_ixoriy)
    {
      ixoriy = new_ixoriy;
      new_ixoriy = 0;
      op = fetchm(pc);
    }
  } while (ixoriy);

  return tstates - tstore;
}

static inline int __not_in_flash_func(z80_interrupt)(void)
{
  // NOTE: For optimisation, need to ensure iff1 set before calling this
  if(fetchm(pc)  == 0x76)
  {
    pc++;
  }
  iff1 = iff2 = 0;
  push2(pc);
  radjust++;

  switch(im)
  {
    case 0: /* IM 0 */
    case 2: /* IM 1 */
      pc = 0x38;
      return 13;
    break;

    case 3: /* IM 2 */
      pc = fetch2((i << 8) | 0xff);
      return 19;
    break;

    default:
      return 12;
    break;
  }
}

static inline int __not_in_flash_func(nmi_interrupt)(void)
{
  iff1 = 0;
  if(fetchm(pc) == 0x76)
  {
    pc++;
  }
  push2(pc);
  radjust++;
  //m1cycles++;       Not needed for ZX81
  pc = 0x66;

  return 11;
}

/* Normally, these sync checks are done by the TV :-) */
static inline void __not_in_flash_func(checkhsync)(int tolchk)
{
  if ( ( !tolchk && sync_len >= HSYNC_MINLEN && sync_len <= (HSYNC_MAXLEN + MAX_JMP) && RasterX >= HSYNC_TOLERANCEMIN )  ||
       (  tolchk &&                                                                     RasterX >= HSYNC_TOLERANCEMAX ) )
  {
    RasterX = ((hsync_counter - HSYNC_END) < MAX_JMP) ? ((hsync_counter - HSYNC_END) << 1) : 0;
    RasterY++;
    dest += disp.stride_bit;
  }
}

static inline void __not_in_flash_func(checkvsync)(int tolchk)
{
  if ( ( !tolchk && sync_len >= VSYNC_MINLEN && RasterY >= VSYNC_TOLERANCEMIN ) ||
       (  tolchk &&                             RasterY >= VSYNC_TOLERANCEMAX ) )
  {
    if (sync_len>(int)tsmax)
    {
      // If there has been no sync for an entire frame then blank the screen
      displayBlank(true);
      sync_len = 0;
      frameNotSync = true;
      vsyncFound = false;
    }
    else
    {
      displayAndNewScreen(frameSync);
      if (vsyncFound)
      {
        frameNotSync = (RasterY >= VSYNC_TOLERANCEMAX);
      }
      else
      {
        frameNotSync = true;
        vsyncFound = (RasterY < VSYNC_TOLERANCEMAX);
      }
    }
    RasterY = 0;
    dest = disp.offset + (disp.stride_bit * adjustStartY) + adjustStartX;
  }
}

static inline void __not_in_flash_func(checksync)(int inc)
{
  if (!SYNC_signal)
  {
    if (psync)
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

static void __not_in_flash_func(anyout)(void)
{
  LastInstruction = LASTINSTNONE;

  if (VSYNC_state)
  {
    VSYNC_state = 0;
    // A fairly arbitrary value selected after comparing with a "real" ZX81
    if ((hsync_counter < HSYNC_START) || (hsync_counter >= 178) )
    {
      rowcounter_hold = true;
    }
    vsync_lower();
    if ((sync_len > HSYNC_MAXLEN) && (RasterY < VSYNC_TOLERANCEMIN))
    {
        vsyncFound = false;
    }
  }
}

bool save_snap_z80(void)
{
  if (!emu_FileWriteBytes(&a, sizeof(a))) return false;
  if (!emu_FileWriteBytes(&f, sizeof(f))) return false;
  if (!emu_FileWriteBytes(&b, sizeof(b))) return false;
  if (!emu_FileWriteBytes(&c, sizeof(c))) return false;
  if (!emu_FileWriteBytes(&d, sizeof(d))) return false;
  if (!emu_FileWriteBytes(&e, sizeof(e))) return false;
  if (!emu_FileWriteBytes(&h, sizeof(h))) return false;
  if (!emu_FileWriteBytes(&l, sizeof(l))) return false;

  if (!emu_FileWriteBytes(&a1, sizeof(a1))) return false;
  if (!emu_FileWriteBytes(&f1, sizeof(f1))) return false;
  if (!emu_FileWriteBytes(&b1, sizeof(b1))) return false;
  if (!emu_FileWriteBytes(&c1, sizeof(c1))) return false;
  if (!emu_FileWriteBytes(&d1, sizeof(d1))) return false;
  if (!emu_FileWriteBytes(&e1, sizeof(e1))) return false;
  if (!emu_FileWriteBytes(&h1, sizeof(h1))) return false;
  if (!emu_FileWriteBytes(&l1, sizeof(l1))) return false;
  if (!emu_FileWriteBytes(&i, sizeof(i))) return false;
  if (!emu_FileWriteBytes(&iff1, sizeof(iff1))) return false;
  if (!emu_FileWriteBytes(&iff2, sizeof(iff2))) return false;
  if (!emu_FileWriteBytes(&im, sizeof(im))) return false;
  if (!emu_FileWriteBytes(&r, sizeof(r))) return false;

  if (!emu_FileWriteBytes(&ixoriy, sizeof(ixoriy))) return false;
  if (!emu_FileWriteBytes(&new_ixoriy, sizeof(new_ixoriy))) return false;

  if (!emu_FileWriteBytes(&ix, sizeof(ix))) return false;
  if (!emu_FileWriteBytes(&iy, sizeof(iy))) return false;
  if (!emu_FileWriteBytes(&sp, sizeof(sp))) return false;
  if (!emu_FileWriteBytes(&pc, sizeof(pc))) return false;

  if (!emu_FileWriteBytes(&radjust, sizeof(radjust))) return false;
  if (!emu_FileWriteBytes(&intsample, sizeof(intsample))) return false;
  if (!emu_FileWriteBytes(&m1cycles, sizeof(m1cycles))) return false;

  if (!emu_FileWriteBytes(&tstates, sizeof(tstates))) return false;
  if (!emu_FileWriteBytes(&ts, sizeof(ts))) return false;
  if (!emu_FileWriteBytes(&vsx, sizeof(vsx))) return false;
  if (!emu_FileWriteBytes(&vsy, sizeof(vsy))) return false;
  if (!emu_FileWriteBytes(&nrmvideo, sizeof(nrmvideo))) return false;
  if (!emu_FileWriteBytes(&RasterX, sizeof(RasterX))) return false;
  if (!emu_FileWriteBytes(&RasterY, sizeof(RasterY))) return false;
  if (!emu_FileWriteBytes(&psync, sizeof(psync))) return false;
  if (!emu_FileWriteBytes(&sync_len, sizeof(sync_len))) return false;

  if (!emu_FileWriteBytes(&running_rom, sizeof(running_rom))) return false;
  if (!emu_FileWriteBytes(&frameNotSync, sizeof(frameNotSync))) return false;
  if (!emu_FileWriteBytes(&LastInstruction, sizeof(LastInstruction))) return false;

  if (!emu_FileWriteBytes(&NMI_generator, sizeof(NMI_generator))) return false;
  if (!emu_FileWriteBytes(&hsync_pending, sizeof(hsync_pending))) return false;
  if (!emu_FileWriteBytes(&VSYNC_state, sizeof(VSYNC_state))) return false;
  if (!emu_FileWriteBytes(&HSYNC_state, sizeof(HSYNC_state))) return false;

  if (!emu_FileWriteBytes(&S_RasterX, sizeof(S_RasterX))) return false;
  if (!emu_FileWriteBytes(&S_RasterY, sizeof(S_RasterY))) return false;

  if (!emu_FileWriteBytes(&videoFlipFlop1Q, sizeof(videoFlipFlop1Q))) return false;
  if (!emu_FileWriteBytes(&videoFlipFlop2Q, sizeof(videoFlipFlop2Q))) return false;
  if (!emu_FileWriteBytes(&videoFlipFlop3Q, sizeof(videoFlipFlop3Q))) return false;
  if (!emu_FileWriteBytes(&videoFlipFlop3Clear, sizeof(videoFlipFlop3Clear))) return false;
  if (!emu_FileWriteBytes(&prevVideoFlipFlop3Q, sizeof(prevVideoFlipFlop3Q))) return false;

  if (!emu_FileWriteBytes(&vsyncFound, sizeof(vsyncFound))) return false;
  if (!emu_FileWriteBytes(&lineClockCarryCounter, sizeof(lineClockCarryCounter))) return false;

  if (!emu_FileWriteBytes(&scanline_len, sizeof(scanline_len))) return false;
  if (!emu_FileWriteBytes(&sync_type, sizeof(sync_type))) return false;
  if (!emu_FileWriteBytes(&sync_len, sizeof(sync_len))) return false;
  if (!emu_FileWriteBytes(&nosync_lines, sizeof(nosync_lines))) return false;

  // Process interlace emu_VideoSetInterlace();

  if (!emu_FileWriteBytes(&chromamode, sizeof(chromamode))) return false;
#ifdef SUPPORT_CHROMA
  if (!emu_FileWriteBytes(&bordercolour, sizeof(bordercolour))) return false;
  if (!emu_FileWriteBytes(&bordercolournew, sizeof(bordercolournew))) return false;
#endif

  return true;
}

bool load_snap_z80(void)
{
  if (!emu_FileReadBytes(&a, sizeof(a))) return false;
  if (!emu_FileReadBytes(&f, sizeof(f))) return false;
  if (!emu_FileReadBytes(&b, sizeof(b))) return false;
  if (!emu_FileReadBytes(&c, sizeof(c))) return false;
  if (!emu_FileReadBytes(&d, sizeof(d))) return false;
  if (!emu_FileReadBytes(&e, sizeof(e))) return false;
  if (!emu_FileReadBytes(&h, sizeof(h))) return false;
  if (!emu_FileReadBytes(&l, sizeof(l))) return false;

  if (!emu_FileReadBytes(&a1, sizeof(a1))) return false;
  if (!emu_FileReadBytes(&f1, sizeof(f1))) return false;
  if (!emu_FileReadBytes(&b1, sizeof(b1))) return false;
  if (!emu_FileReadBytes(&c1, sizeof(c1))) return false;
  if (!emu_FileReadBytes(&d1, sizeof(d1))) return false;
  if (!emu_FileReadBytes(&e1, sizeof(e1))) return false;
  if (!emu_FileReadBytes(&h1, sizeof(h1))) return false;
  if (!emu_FileReadBytes(&l1, sizeof(l1))) return false;
  if (!emu_FileReadBytes(&i, sizeof(i))) return false;
  if (!emu_FileReadBytes(&iff1, sizeof(iff1))) return false;
  if (!emu_FileReadBytes(&iff2, sizeof(iff2))) return false;
  if (!emu_FileReadBytes(&im, sizeof(im))) return false;
  if (!emu_FileReadBytes(&r, sizeof(r))) return false;

  if (!emu_FileReadBytes(&ixoriy, sizeof(ixoriy))) return false;
  if (!emu_FileReadBytes(&new_ixoriy, sizeof(new_ixoriy))) return false;

  if (!emu_FileReadBytes(&ix, sizeof(ix))) return false;
  if (!emu_FileReadBytes(&iy, sizeof(iy))) return false;
  if (!emu_FileReadBytes(&sp, sizeof(sp))) return false;
  if (!emu_FileReadBytes(&pc, sizeof(pc))) return false;

  if (!emu_FileReadBytes(&radjust, sizeof(radjust))) return false;
  if (!emu_FileReadBytes(&intsample, sizeof(intsample))) return false;
  if (!emu_FileReadBytes(&m1cycles, sizeof(m1cycles))) return false;

  if (!emu_FileReadBytes(&tstates, sizeof(tstates))) return false;
  if (!emu_FileReadBytes(&ts, sizeof(ts))) return false;
  if (!emu_FileReadBytes(&vsx, sizeof(vsx))) return false;
  if (!emu_FileReadBytes(&vsy, sizeof(vsy))) return false;
  if (!emu_FileReadBytes(&nrmvideo, sizeof(nrmvideo))) return false;
  if (!emu_FileReadBytes(&RasterX, sizeof(RasterX))) return false;
  if (!emu_FileReadBytes(&RasterY, sizeof(RasterY))) return false;
  if (!emu_FileReadBytes(&psync, sizeof(psync))) return false;
  if (!emu_FileReadBytes(&sync_len, sizeof(sync_len))) return false;

  if (!emu_FileReadBytes(&running_rom, sizeof(running_rom))) return false;
  if (!emu_FileReadBytes(&frameNotSync, sizeof(frameNotSync))) return false;
  if (!emu_FileReadBytes(&LastInstruction, sizeof(LastInstruction))) return false;

  if (!emu_FileReadBytes(&NMI_generator, sizeof(NMI_generator))) return false;
  if (!emu_FileReadBytes(&hsync_pending, sizeof(hsync_pending))) return false;
  if (!emu_FileReadBytes(&VSYNC_state, sizeof(VSYNC_state))) return false;
  if (!emu_FileReadBytes(&HSYNC_state, sizeof(HSYNC_state))) return false;

  if (!emu_FileReadBytes(&S_RasterX, sizeof(S_RasterX))) return false;
  if (!emu_FileReadBytes(&S_RasterY, sizeof(S_RasterY))) return false;

  if (!emu_FileReadBytes(&videoFlipFlop1Q, sizeof(videoFlipFlop1Q))) return false;
  if (!emu_FileReadBytes(&videoFlipFlop2Q, sizeof(videoFlipFlop2Q))) return false;
  if (!emu_FileReadBytes(&videoFlipFlop3Q, sizeof(videoFlipFlop3Q))) return false;
  if (!emu_FileReadBytes(&videoFlipFlop3Clear, sizeof(videoFlipFlop3Clear))) return false;
  if (!emu_FileReadBytes(&prevVideoFlipFlop3Q, sizeof(prevVideoFlipFlop3Q))) return false;

  if (!emu_FileReadBytes(&vsyncFound, sizeof(vsyncFound))) return false;
  if (!emu_FileReadBytes(&lineClockCarryCounter, sizeof(lineClockCarryCounter))) return false;

  if (!emu_FileReadBytes(&scanline_len, sizeof(scanline_len))) return false;
  if (!emu_FileReadBytes(&sync_type, sizeof(sync_type))) return false;
  if (!emu_FileReadBytes(&sync_len, sizeof(sync_len))) return false;
  if (!emu_FileReadBytes(&nosync_lines, sizeof(nosync_lines))) return false;

  // Process interlace emu_VideoSetInterlace();

  if (!emu_FileReadBytes(&chromamode, sizeof(chromamode))) return false;
#ifdef SUPPORT_CHROMA
  if (!emu_FileReadBytes(&bordercolour, sizeof(bordercolour))) return false;
  if (!emu_FileReadBytes(&bordercolournew, sizeof(bordercolournew))) return false;
#endif
  return true;
}

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
 *
 *
 * sound.c - sound support, based on the beeper/AY code I
 *           wrote for Fuse.
 */

/* some AY details (volume levels, white noise RNG algorithm) based on
 * info from MAME's ay8910.c - MAME's licence explicitly permits free
 * use of info (even encourages it).
 */

/* NB: I know some of this stuff looks fairly CPU-hogging.
 * For example, the AY code tracks changes with sub-frame timing
 * in a rather hairy way, and there's subsampling here and there.
 * But if you measure the CPU use, it doesn't actually seem
 * very high at all. And I speak as a Cyrix owner. :-)
 *
 * (I based that on testing in Fuse, but I doubt it's that much
 * worse in z81. Though in both, the AY code does cause cache oddities
 * on my machine, so I get the bizarre situation of z81 jumping
 * between <=2% CPU use and *30*% CPU use pretty much at random...)
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico.h"     /* For not in flash */

#include "common.h"
#include "sound.h"
#include "z80.h"
#include "iopins.h"
#include "emusnap.h"

/* configuration */
/* Always generate two channels, even if mono */

int sound_enabled=0;
int sound_stereo_acb=0;     /* 1 for ACB stereo, else 0 */

/* sound_type is in z80.c */

#define AY_CLOCK_QUICKSILVA (3250000>>2)

/* Bi-Pak Zon X-81, clock rate straight from the manual */
#define AY_CLOCK_ZONX       (3250000>>1)

/*
 * For I2S max value is 32768 mid point is 0 max for 1 channel is < 32768 / 4 = 8192
 * For 12S further divide by 4 to avoid full volume
 */
#define AMPL_AY_TONE        2048

#if ((!defined (SOUND_I2S)) && (!defined (SOUND_HDMI)))
#define VSYNC_ON            (RANGE - 1)
#define VSYNC_OFF           0

#ifndef PICO_PICOZXREAL_BOARD
/* For PWM max value is 999, mid point 499.5 mid for each channel is 499.5 / 4 = 124
   AMPL_AY_TONE (2048) divided by 16 (2<<4) gives 128
*/
#define PWM_SOUND_SHIFT_REDUCE  4
#else
// Allow risk of over saturation for ZXReal board, as PWM output is low volume
#define PWM_SOUND_SHIFT_REDUCE  3
#endif
#else
#if (defined (SOUND_HDMI))
// Not realistic to load from HDMI, so can make quieter
#define VSYNC_ON            0x500
#else
#define VSYNC_ON            0x4000
#endif
#define VSYNC_OFF           (-VSYNC_ON)
#endif

#ifdef MIC_SOUND
#define MIC_OFF             ZEROMIC
#ifdef SOUND_I2S
#define MIC_ON              (-ZEROMIC)
#else
#define MIC_ON              (RANGE -1)
#endif
#endif

/* max. number of sub-frame AY port writes allowed;
 * given the number of port writes theoretically possible in a
 * 50th I think this should be plenty.
 */
#define AY_CHANGE_MAX         160
#define VSYNC_CHANGE_MAX      (AY_CHANGE_MAX * 4)
#define FRAME_SIZE            (SAMPLE_FREQ / 50) // Number of samples in 20 ms

static uint16_t ay_tone_levels[16];

/* tick/incr/periods are all fixed-point with low 16 bits as
 * fractional part, except ay_env_{tick,period} which count as the chip does.
 */
static unsigned int ay_tone_tick[3],ay_noise_tick;
static unsigned int ay_env_tick,ay_env_subcycles;
static unsigned int ay_tick_incr;
static unsigned int ay_tone_period[3],ay_noise_period,ay_env_period;

static int env_held=0,env_alternating=0;

/* AY registers */
/* we have 16 so we can fake an 8910 if needed */
static unsigned char sound_ay_registers[16];

typedef struct
{
  unsigned long tstates;
  unsigned short ofs;
  unsigned char reg,val;
}  ay_change_tag;

typedef struct
{
  int16_t change_count;       // Number of entries in change_tag
  int16_t volume_on;          // Volume for pulse 1
  int16_t volume_off;         // Volume for pulse 0
  bool initial_state;         // state at start of frame
  bool current_state;         // Current state
} vsync_status_tag;

typedef union
{
  unsigned short  vsync_offset[VSYNC_CHANGE_MAX];
  ay_change_tag   ay[AY_CHANGE_MAX];
} change_tag;

static change_tag change;
static vsync_status_tag vsync;
static int ay_change_count;

#ifdef MIC_SOUND
static change_tag mic_change;
static vsync_status_tag mic;
#endif

/* Private function declarations */
static void sound_vsync_reset(void);
static void sound_ay_reset(void);
static void sound_ay_setvol(void);
static void sound_ay_overlay(int16_t* buff);
static void sound_populate_frame(uint16_t* buff, vsync_status_tag* status, const change_tag* c);
static void sound_capture_mic(int on, vsync_status_tag* status, change_tag* c);

/* Macros */

/* ay */
#define AY_GET_SUBVAL(tick,period)                                    \
  (level*2*(tick-period)/ay_tick_incr)

#define AY_OVERLAY_TONE(ptr,chan,level)                               \
  was_high=0;                                                         \
  if(level)                                                           \
  {                                                                   \
    if(ay_tone_tick[chan]>=ay_tone_period[chan])                      \
      (*(ptr))+=(level),was_high=1;                                   \
    else                                                              \
      (*(ptr))-=(level);                                              \
  }                                                                   \
                                                                      \
  ay_tone_tick[chan]+=ay_tick_incr;                                   \
  if(level && !was_high && ay_tone_tick[chan]>=ay_tone_period[chan])  \
    (*(ptr))+=AY_GET_SUBVAL(ay_tone_tick[chan],ay_tone_period[chan]); \
                                                                      \
  if(ay_tone_tick[chan]>=ay_tone_period[chan]*2)                      \
  {                                                                   \
    ay_tone_tick[chan]-=ay_tone_period[chan]*2;                       \
    /* sanity check needed to avoid making samples sound terrible */  \
    if(level && ay_tone_tick[chan]<ay_tone_period[chan])              \
      (*(ptr))-=AY_GET_SUBVAL(ay_tone_tick[chan],0);                  \
  }


/* The behaviour of the MIC output works differently on a ZX81
 * compared to Spectrum. There is no decay.
 * For saving and loading square waves are modelled.
 * The band pass filter in the ZX81 and ZX80 Mic curcuit is not
 * modelled, as it has little impact to the relative amplitude of
 * the frequencies generated. The 15kHz hsync signal is not emulated.
 */

/*
 * Public interface
 */
void sound_create(void)
{
  sound_ay_setvol();
  sound_ay_reset();
  sound_vsync_reset();
}

void sound_init(bool acb, bool force_reset)
{
  static int last_sound_type = SOUND_TYPE_NONE;

  sound_stereo_acb = (AUDIO_PIN_L != AUDIO_PIN_R) ? acb : 0;

  if (force_reset || (sound_type != last_sound_type))
  {
    last_sound_type = sound_type;
    sound_enabled=1;

    if ((sound_type == SOUND_TYPE_VSYNC) || (sound_type == SOUND_TYPE_CHROMA) || (sound_type == SOUND_TYPE_CASSETTE))
    {
      sound_vsync_reset();
    }
    else if ((sound_type == SOUND_TYPE_QUICKSILVA) || (sound_type == SOUND_TYPE_ZONX))
    {
      sound_ay_reset();
    }
  }
}

void sound_end(void)
{
  sound_enabled=0;
}

void sound_change_type(int new_sound_type)
{
    sound_type = new_sound_type;
}

void __not_in_flash_func(sound_frame)(uint16_t* buff)
{
  if (sound_type == SOUND_TYPE_NONE)
  {
    return;
  }

  if((sound_type == SOUND_TYPE_QUICKSILVA) || (sound_type == SOUND_TYPE_ZONX))
  {
    sound_ay_overlay((int16_t*)buff);
    ay_change_count = 0;
  }
  else
  {
    sound_populate_frame(buff, &vsync, &change);
  }
}

#ifdef MIC_SOUND
void mic_frame(uint16_t* buff)
{
  sound_populate_frame(buff, &mic, &mic_change);
}
#endif

/* Don't make the change immediately; record it for later,
 * to be made by sound_frame() (via sound_ay_overlay()).
 */
void __not_in_flash_func(sound_ay_write)(int reg,int val)
{
  // Rely on this only being called when sound_type is QS or ZonX
  if(!sound_enabled) return;

  /* accept r15, in case of the two-I/O-port 8910 */
  if(reg>=16) return;

  if(ay_change_count<AY_CHANGE_MAX)
  {
    change.ay[ay_change_count].tstates=tstates;
    change.ay[ay_change_count].reg=reg;
    change.ay[ay_change_count].val=val;
    ay_change_count++;
  }
  else
  {
    printf("ay_change_count exceeded AY_CHANGE_MAX\n");
  }
}

void __not_in_flash_func(sound_vsync)(int on)
{
  if ((sound_type == SOUND_TYPE_CASSETTE) || (sound_type == SOUND_TYPE_VSYNC) || (sound_type == SOUND_TYPE_CHROMA))
  {
    sound_capture_mic(on, &vsync, &change);
  }
}

#ifdef MIC_SOUND
void __not_in_flash_func(sound_mic)(int on)
{
  sound_capture_mic(on, &mic, &mic_change);
}
#endif

/*
 * Private interface
 */
static void __not_in_flash_func(sound_populate_frame)(uint16_t* buff, vsync_status_tag* status, const change_tag* c)
{
  int frame_index = 0;
  int16_t* restrict ibuff = (int16_t*)buff;
#ifdef DEBUG_SOUND
  static int change_count_max = 0;

  if (change_count_max < status->change_count)
  {
    change_count_max = status->change_count;
    printf("change_count_max: %i\n", change_count_max);
  }
#endif

  for (int vs = 0; vs < status->change_count; ++vs)
  {
    int16_t val = (status->initial_state ? status->volume_on : status->volume_off);

    for (int fill = frame_index; fill < c->vsync_offset[vs]; ++fill)
    {
      *ibuff++ = val;
      *ibuff++ = val;
    }
    status->initial_state = !status->initial_state;
    frame_index = c->vsync_offset[vs];
  }

  // Fill in end of frame
  int16_t val = (status->initial_state ? status->volume_on : status->volume_off);

  for (int fill = frame_index; fill < FRAME_SIZE; ++fill)
  {
    *ibuff++ = val;
    *ibuff++ = val;
  }
  status->change_count = 0;

  if (status->initial_state != status->current_state)
  {
#ifdef DEBUG_SOUND
    printf("current_state incorrect");
#endif
  }
}

static void __not_in_flash_func(sound_capture_mic)(int on, vsync_status_tag* status, change_tag* c)
{
  // Ignore if state has not changed
  if (status->current_state == (on != 0))
  {
    return;
  }

  if (status->change_count < VSYNC_CHANGE_MAX)
  {
    status->current_state = !status->current_state;
    int pos = (tstates*FRAME_SIZE)/tsmax;

    if (status->change_count && (c->vsync_offset[status->change_count - 1] == pos))
    {
      // Remove the previous zero length blip
      status->change_count--;
    }
    else
    {
      c->vsync_offset[status->change_count] = pos;
      status->change_count++;
    }
  }
  else
  {
      printf("change_count exceeded\n");
  }
}

static void sound_ay_setvol(void)
{
  int f;
  double v;

  /* logarithmic volume levels, 3dB per step */
  v=AMPL_AY_TONE;
  for(f=15;f>0;f--)
  {
    ay_tone_levels[f]=(uint16_t)(v+0.5);
    // printf("Tone %i\n", ay_tone_levels[f]);
    v/=1.4125375446;
  }
  ay_tone_levels[0]=0;
}

static void sound_ay_reset(void)
{
  ay_noise_tick=ay_noise_period=0;
  ay_env_tick=ay_env_period=0;
  for(int f=0; f<3; f++)
    ay_tone_tick[f]=ay_tone_period[f]=0;

  ay_change_count=0;
  env_held=0;
  env_alternating=0;

  for (int i=0; i<16;++i)
  {
    sound_ay_registers[i] = 0;
  }

  // Set the ay clock rate
  int clock = (sound_type == SOUND_TYPE_QUICKSILVA) ? AY_CLOCK_QUICKSILVA : AY_CLOCK_ZONX;
  ay_tick_incr=(int)(65536.*clock/SAMPLE_FREQ);
}

static void sound_vsync_reset(void)
{
  // vsync reset
  vsync.initial_state = false;
  vsync.current_state = false;
  vsync.change_count = 0;
  vsync.volume_on = VSYNC_ON;
  vsync.volume_off = VSYNC_OFF;
#ifdef MIC_SOUND
  mic.initial_state = false;
  mic.current_state = false;
  mic.change_count = 0;
  mic.volume_on = MIC_ON;
  mic.volume_off = MIC_OFF;
#endif
}

static int rng=1;
static int noise_toggle=1;
static int env_level=0;

static void __not_in_flash_func(sound_ay_overlay)(int16_t* buff)
{
  int tone_level[3];
  int mixer,envshape;
  int f,g,level;
  int v=0;
  int16_t* ptr;
  ay_change_tag* change_ptr=change.ay;
  int changes_left=ay_change_count;
  int reg,r;
  int was_high;
  int channels=2;

  /* convert change times to sample offsets */
  for(f=0;f<ay_change_count;f++)
    change.ay[f].ofs=(change.ay[f].tstates*SAMPLE_FREQ)/3250000;

  for(f=0,ptr=buff;f<FRAME_SIZE;f++,ptr+=channels)
  {
    /* update ay registers. All this sub-frame change stuff
    * is pretty hairy, but how else would you handle the
    * samples in Robocop? :-) It also clears up some other
    * glitches.
    *
    * Ok, maybe that's no big deal on the ZX81, but even so. :-)
    * (Though, due to tstate-changing games in z80.c, we can
    * rarely `lose' one this way - hence "f==.." bit below
    * to catch any that slip through.)
    */
    while(changes_left && (f>=change_ptr->ofs || f==(FRAME_SIZE-1)))
    {
      sound_ay_registers[reg=change_ptr->reg]=change_ptr->val;
      change_ptr++; changes_left--;

      /* fix things as needed for some register changes */
      switch(reg)
      {
        case 0: case 1: case 2: case 3: case 4: case 5:
          r=reg>>1;
          ay_tone_period[r]=(8*(sound_ay_registers[reg&~1]|
                                (sound_ay_registers[reg|1]&15)<<8))<<16;

          /* important to get this right, otherwise e.g. Ghouls 'n' Ghosts
          * has really scratchy, horrible-sounding vibrato.
          */
          if(ay_tone_period[r] && ay_tone_tick[r]>=ay_tone_period[r]*2)
            ay_tone_tick[r]%=ay_tone_period[r]*2;
          break;
        case 6:
          ay_noise_tick=0;
          ay_noise_period=(16*(sound_ay_registers[reg]&31))<<16;
          break;
        case 11: case 12:
          /* this one *isn't* fixed-point */
          ay_env_period=sound_ay_registers[11]|(sound_ay_registers[12]<<8);
          break;
        case 13:
          ay_env_tick=ay_env_subcycles=0;
          env_held=env_alternating=0;
          env_level=0;
          break;
      }
    }

    /* the tone level if no enveloping is being used */
    for(g=0;g<3;g++)
      tone_level[g]=ay_tone_levels[sound_ay_registers[8+g]&15];

    /* envelope */
    envshape=sound_ay_registers[13];
    if(ay_env_period)
    {
      if(!env_held)
      {
        v=((int)ay_env_tick*15)/ay_env_period;
        if(v<0) v=0;
        if(v>15) v=15;
        if((envshape&4)==0) v=15-v;
        if(env_alternating) v=15-v;
        env_level=ay_tone_levels[v];
      }
    }

    for(g=0;g<3;g++)
      if(sound_ay_registers[8+g]&16)
        tone_level[g]=env_level;

    if(ay_env_period)
    {
      /* envelope gets incr'd every 256 AY cycles */
      ay_env_subcycles+=ay_tick_incr;
      if(ay_env_subcycles>=(256<<16))
      {
        ay_env_subcycles-=(256<<16);

        ay_env_tick++;
        if(ay_env_tick>=ay_env_period)
        {
          ay_env_tick-=ay_env_period;
          if(!env_held && ((envshape&1) || (envshape&8)==0))
          {
            env_held=1;
            if((envshape&2) || (envshape&0xc)==4)
              env_level=ay_tone_levels[15-v];
          }
          if(!env_held && (envshape&2))
            env_alternating=!env_alternating;
        }
      }
    }

    /* generate tone+noise */
    /* channel C first to make ACB easier */
    mixer=sound_ay_registers[7];
    *ptr = 0; // Mid point of range, will correct for PWM case later
    if((mixer&4)==0 || (mixer&0x20)==0)
    {
      level=(noise_toggle || (mixer&0x20))?tone_level[2]:0;
      AY_OVERLAY_TONE(ptr,2,level);
    }

    if(sound_stereo_acb)
      ptr[1]=*ptr;

    if((mixer&1)==0 || (mixer&0x08)==0)
    {
      level=(noise_toggle || (mixer&0x08))?tone_level[0]:0;
      AY_OVERLAY_TONE(ptr,0,level);
    }
    if((mixer&2)==0 || (mixer&0x10)==0)
    {
      level=(noise_toggle || (mixer&0x10))?tone_level[1]:0;
      AY_OVERLAY_TONE(&ptr[sound_stereo_acb],1,level);
    }

    if(!sound_stereo_acb)
      ptr[1]=*ptr;

  #if ((!defined (SOUND_I2S)) && (!defined (SOUND_HDMI)))
    // Correct to PWM
    *ptr = (*ptr>>PWM_SOUND_SHIFT_REDUCE) + ZEROSOUND;
    ptr[1] = (ptr[1]>>PWM_SOUND_SHIFT_REDUCE) + ZEROSOUND;
  #endif
    /* update noise RNG/filter */
    ay_noise_tick+=ay_tick_incr;
    if(ay_noise_tick>=ay_noise_period)
    {
      if((rng&1)^((rng&2)?1:0))
        noise_toggle=!noise_toggle;

      /* rng is 17-bit shift reg, bit 0 is output.
      * input is bit 0 xor bit 2.
      */
      rng|=((rng&1)^((rng&4)?1:0))?0x20000:0;
      rng>>=1;

      ay_noise_tick-=ay_noise_period;
    }
  }
}

bool sound_save_snap(void)
{
  if (!emu_FileWriteBytes(&sound_enabled, sizeof(sound_enabled))) return false;
  if (!emu_FileWriteBytes(&sound_stereo_acb, sizeof(sound_stereo_acb))) return false;


#ifdef MIC_SOUND
  if (!emu_FileReadBytes(&mic_change, sizeof(mic_change))) return false;
  if (!emu_FileReadBytes(&mic, sizeof(mic))) return false;
#else
  // These reads will be overwritten later
  if (!emu_FileWriteBytes(&change, sizeof(change))) return false;
  if (!emu_FileWriteBytes(&vsync, sizeof(vsync))) return false;
#endif
  if (!emu_FileWriteBytes(&vsync, sizeof(vsync))) return false;

  if (!emu_FileWriteBytes(&ay_noise_tick, sizeof(ay_noise_tick))) return false;
  if (!emu_FileWriteBytes(&ay_env_tick, sizeof(ay_env_tick))) return false;
  if (!emu_FileWriteBytes(&ay_env_subcycles, sizeof(ay_env_subcycles))) return false;
  if (!emu_FileWriteBytes(&ay_tick_incr, sizeof(ay_tick_incr))) return false;
  if (!emu_FileWriteBytes(&ay_noise_period, sizeof(ay_noise_period))) return false;
  if (!emu_FileWriteBytes(&ay_env_period, sizeof(ay_env_period))) return false;
  if (!emu_FileWriteBytes(&env_held, sizeof(env_held))) return false;
  if (!emu_FileWriteBytes(&env_alternating, sizeof(env_alternating))) return false;
  if (!emu_FileWriteBytes(&change, sizeof(change))) return false;
  if (!emu_FileWriteBytes(&ay_change_count, sizeof(ay_change_count))) return false;

  if (!emu_FileWriteBytes(&rng, sizeof(rng))) return false;
  if (!emu_FileWriteBytes(&noise_toggle, sizeof(noise_toggle))) return false;
  if (!emu_FileWriteBytes(&env_level, sizeof(env_level))) return false;

  if (!emu_FileWriteBytes(ay_tone_levels, sizeof(uint16_t) * 16)) return false;
  if (!emu_FileWriteBytes(ay_tone_tick, sizeof(unsigned int) * 3)) return false;
  if (!emu_FileWriteBytes(ay_tone_period, sizeof(unsigned int) * 3)) return false;
  if (!emu_FileWriteBytes(sound_ay_registers, sizeof(unsigned char) * 16)) return false;

  return true;
}

bool sound_load_snap(uint32_t version)
{
  if (!emu_FileReadBytes(&sound_enabled, sizeof(sound_enabled))) return false;
  if (!emu_FileReadBytes(&sound_stereo_acb, sizeof(sound_stereo_acb))) return false;
  if (version == SUPPORTED_VERSION_1)
  {
    unsigned int dummy;
    if (!emu_FileReadBytes(&dummy, sizeof(dummy))) return false;
    if (!emu_FileReadBytes(&dummy, sizeof(dummy))) return false;
    // mic_xxx and vsync_xxx variables will be initialised as sound reset called before smapshot load
  }
  else // SUPPORTED_VERSION_2
  {
#ifdef MIC_SOUND
    if (!emu_FileReadBytes(&mic_change, sizeof(mic_change))) return false;
    if (!emu_FileReadBytes(&mic, sizeof(mic))) return false;
#else
    // These reads will be overwritten later
    if (!emu_FileReadBytes(&change, sizeof(change))) return false;
    if (!emu_FileReadBytes(&vsync, sizeof(vsync))) return false;
#endif
  }

  if (!emu_FileReadBytes(&vsync, sizeof(vsync))) return false;

  if (version == SUPPORTED_VERSION_1)
  {
    int dummy;
    if (!emu_FileReadBytes(&dummy, sizeof(dummy))) return false;
    if (!emu_FileReadBytes(&dummy, sizeof(dummy))) return false;
    if (!emu_FileReadBytes(&dummy, sizeof(dummy))) return false;
    if (!emu_FileReadBytes(&dummy, sizeof(dummy))) return false;
    if (!emu_FileReadBytes(&dummy, sizeof(dummy))) return false;
  }
  if (!emu_FileReadBytes(&ay_noise_tick, sizeof(ay_noise_tick))) return false;
  if (!emu_FileReadBytes(&ay_env_tick, sizeof(ay_env_tick))) return false;
  if (!emu_FileReadBytes(&ay_env_subcycles, sizeof(ay_env_subcycles))) return false;
  if (!emu_FileReadBytes(&ay_tick_incr, sizeof(ay_tick_incr))) return false;
  if (!emu_FileReadBytes(&ay_noise_period, sizeof(ay_noise_period))) return false;
  if (!emu_FileReadBytes(&ay_env_period, sizeof(ay_env_period))) return false;
  if (!emu_FileReadBytes(&env_held, sizeof(env_held))) return false;
  if (!emu_FileReadBytes(&env_alternating, sizeof(env_alternating))) return false;
  if (!emu_FileReadBytes(&change, sizeof(change))) return false;
  if (!emu_FileReadBytes(&ay_change_count, sizeof(ay_change_count))) return false;

  if (!emu_FileReadBytes(&rng, sizeof(rng))) return false;
  if (!emu_FileReadBytes(&noise_toggle, sizeof(noise_toggle))) return false;
  if (!emu_FileReadBytes(&env_level, sizeof(env_level))) return false;

  if (!emu_FileReadBytes(ay_tone_levels, sizeof(uint16_t) * 16)) return false;
  if (!emu_FileReadBytes(ay_tone_tick, sizeof(unsigned int) * 3)) return false;
  if (!emu_FileReadBytes(ay_tone_period, sizeof(unsigned int) * 3)) return false;
  if (!emu_FileReadBytes(sound_ay_registers, sizeof(unsigned char) * 16)) return false;
  return true;
}

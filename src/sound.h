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
 * sound.h
 */
#ifndef _SOUND_H
#define _SOUND_H_
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined (I2S) || defined (SOUND_HDMI)
#define ZEROSOUND 0         // Zero point for I2S sound
#else
#define ZEROSOUND 500       // Zero point for PWM sound
#define RANGE     1000
#endif

#define SAMPLE_FREQ   32000

extern bool sound_create(int framesize);
extern void sound_init(bool acb, bool reset);
extern void sound_ay_write(int reg,int val);
extern void sound_frame(uint16_t* buff);
extern void sound_beeper(int on);
extern void sound_change_type(int new_sound_type);

extern bool sound_save_snap(void);
extern bool sound_load_snap(void);

#ifdef __cplusplus
}
#endif
#endif

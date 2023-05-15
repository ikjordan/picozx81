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
#ifdef I2S
#define ZEROSOUND 0         // Zero point for I2S sound
#else
#define ZEROSOUND 500       // Zero point for PWM sound
#endif

extern bool sound_create(int freq, int framesize);
extern void sound_init(bool acb);
extern void sound_ay_write(int reg,int val);
extern void sound_frame(uint16_t* buff);

#ifdef __cplusplus
}
#endif
#endif
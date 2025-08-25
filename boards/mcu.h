#ifndef _MCU_H_
#define _MCU_H_

#include <pico.h>   // Needed for CMake macro definitions

#ifdef PICO_2350
#include <boards/pico2.h>
#else
#include <boards/pico.h>
#endif

#endif // _MCU_H_

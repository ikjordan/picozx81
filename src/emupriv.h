#ifndef EMUPRIV_H
#define EMUPRIV_H

#ifdef TIME_SPARE
typedef struct
{
  uint64_t total_time;
  uint32_t underrun;
  int32_t  sound_count;
  int32_t  int_count;
} perform;

extern uint32_t sound_count;
extern uint32_t int_count;

#endif
#endif

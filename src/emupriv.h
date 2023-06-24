#ifndef EMUPRIV_H
#define EMUPRIV_H

#ifdef TIME_SPARE
typedef struct
{
  uint64_t total_time;
  uint32_t underrun;
  int32_t  sound_count;
  int64_t  int_count;
} perform;

extern int32_t sound_count;
extern int64_t int_count;

#endif
#endif

#ifndef EMUPRIV_H
#define EMUPRIV_H

#ifdef TIME_SPARE
extern int32_t sound_count;
extern int64_t int_count;
extern int32_t linein_count;
extern int max_vol_val;
extern int min_vol_val;
#endif

// Channels 0 to 5 reserved for PicoDVI and scanvideo
#define DMA_CHANNEL_SOUND   6
#define DMA_CHANNEL_LINEIN  7
#endif

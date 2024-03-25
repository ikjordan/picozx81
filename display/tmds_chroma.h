#ifndef _TMDS_CHROMA_H
#define _TMDS_CHROMA_H

#include "pico/types.h"

void __not_in_flash_func(tmds_encode_screen)(
  const uint8_t *screenRowPtr,
  const uint8_t	*attributeRowPtr,
  uint32_t *tmdsbuf,
  uint32_t nchars,
  uint32_t plane
);


void __not_in_flash_func(tmds_encode_border)(
  uint32_t nchars,   // r0 is width in characters
  uint32_t plane,    // r1 is colour channel
  uint32_t *tmdsbuf, // r2 is output TMDS buffer
  uint32_t attr      // r3 is the colour attribute
);

#endif

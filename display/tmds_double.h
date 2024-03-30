#ifndef _TMDS_DOUBLE_H_
#define _TMDS_DOUBLE_H_

#include "dvi_config_defs.h"

// Functions from tmds_double.S
void tmds_double_1bpp(const uint8_t *pixbuf, uint32_t *symbuf, size_t n_pix);
void tmds_double_2bpp(const uint8_t *pixbuf, uint32_t *symbuf, size_t n_pix);

void tmds_clone(const uint32_t *symbuf, size_t n_pix);

#endif

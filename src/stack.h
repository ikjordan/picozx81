#ifndef _STACK_H_
#define _STACK_H_

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

extern void stack_init(void);
extern uint32_t stack_used(void);

#ifdef __cplusplus
}
#endif
#endif

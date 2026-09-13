#include "stdint.h"

extern uint8_t __StackBottom;
extern uint8_t __StackTop;

#define STACK_PATTERN 0xDEADBEEF

static uint32_t get_sp(void)
{
    uint32_t sp;
    __asm volatile ("mov %0, sp" : "=r"(sp));
    return sp;
}

void stack_init(void)
{
    uint32_t sp = get_sp();

    uint32_t *p = (uint32_t *)&__StackBottom;
    uint32_t *end = (uint32_t *)sp;

    while (p < end)
        *p++ = STACK_PATTERN;
}

uint32_t stack_used(void)
{
    uint32_t *p = (uint32_t *)&__StackBottom;
    uint32_t *end = (uint32_t *)&__StackTop;

    while ((p < end) && (*p == STACK_PATTERN))
        p++;

    return ((uint32_t)(end - p)) * sizeof(uint32_t);
}


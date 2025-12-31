#ifndef _PIT_H
#define _PIT_H

#include "../lib.h"
#include "i8259.h"

#define PIT_IRQ     0
#define PIT_PORT    0x40

/* asm function for context switching in the scheduler */
extern void swtch_ctx(uint32_t);

/* PIT initialization */
void init_PIT(void);

/* PIT interrupt handler, performs round robin scheduling */
void PIT_Handler(void);


#endif /* _PIT_H */

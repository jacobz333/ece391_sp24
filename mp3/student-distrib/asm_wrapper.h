/* asm_wrapper.h
 * defines convenient x86 wrappers
 * 
 * vim:ts=4 noexpandtab
 */
#ifndef _ASM_WRAPPER_H
#define _ASM_WRAPPER_H

#ifndef ASM

/* SAVE_REG
 * saves all general purpose registers by pushing them to the stack 
 */
#define SAVE_REG    \
        cld;        \
        pushl %gs;  \
        pushl %fs;  \
        pushl %es;  \
        pushl %ds;  \
        pushl %eax; \
        pushl %ebp; \
        pushl %edi; \
        pushl %esi; \
        pushl %edx; \
        pushl %ecx; \
        pushl %ebx; \

/* RESTORE_REG
 * restores all general purpose registers by popping them from the stack 
 */
#define RESTORE_REG \
        popl %ebx;  \
        popl %ecx;  \
        popl %edx;  \
        popl %esi;  \
        popl %edi;  \
        popl %ebp;  \
        popl %eax;  \
        popl %ds;   \
        popl %es;   \
        popl %fs;  \
        popl %gs;  \

#endif /* ASM */

#endif /* _ASM_WRAPPER_H */

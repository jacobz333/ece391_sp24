#ifndef _SYSCALL_H
#define _SYSCALL_H

#include "lib.h"

/* x86 functions from syscall_asm.S */
/* Assembly for the execute, iret context switching occurs, allocation of pcb and pid*/
extern void execute_asm(uint32_t ss, uint32_t esp, uint32_t eflags, uint32_t cs, uint32_t eip);

/* assembly for the halt_asm. restores context returning pcb pid and 128mb address to current states*/
extern void halt_asm(uint32_t parent_ebp, uint32_t ret_val);


/* System calls starting from 1 to 10 */
int32_t halt(uint8_t status);
int32_t execute(const uint8_t* command);
int32_t read(int32_t fd, void* buf, int32_t nbytes);
int32_t write(int32_t fd, const void* buf, int32_t nbytes);
int32_t open(const uint8_t* filename);
int32_t close(int32_t fd);
int32_t getargs(uint8_t* buf, int32_t nbytes);
int32_t vidmap(uint8_t** screen_start);
int32_t set_handler(int32_t signum, void* handler_address);
int32_t sigreturn(void);

#endif /* _SYSCALL_H */

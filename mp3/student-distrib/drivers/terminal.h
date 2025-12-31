#ifndef _TERMINAL_H
#define _TERMINAL_H

#include "../lib.h"
#include "keyboard.h"
#include "fsys.h"
#include "../process.h"

#define MAX_TERMINALS 3                 // 3 max terminals, will reserve PIDs less than this value for base shells

/* terminal struct to store per-terminal context information */
typedef struct terminal_t {
    uint32_t saved_ebp;                 /* stack frame for context switch */
    uint32_t saved_esp0;                /* saved tss.esp0 right before context switch */
    uint32_t currentPID;                /* current pid of terminal */
    pcb_t* currentPCB;                  /* current pcb of terminal */

    uint32_t cursor_x;                  /* saved location of the cursor */
    uint32_t cursor_y;
    char keyboard_buffer[128];          /* keyboard buffer */
    uint32_t keyboard_idx;              /* idx of keyboard buffer */
    uint32_t terminal_newline;          /* new line flag for terminal */
    
    uint32_t rtc_interrupt_occurred;    /* flag to detect for if rtc interrupt occurred (1) or not (0) */
    uint32_t rtc_freq_bit;              /* bit of the rtc counter to check for rtc frequency */
} terminal_t;

/* terminal initialization function */
void init_terminals(void);

/* sets shown terminal */
void set_shown_terminal(uint32_t next_TS_idx);

/* sets active terminal */
void set_active_terminal(uint32_t next_TA_idx);


/* backspace handler for both visual terminal and buffer*/
void backspace(uint8_t* buf, uint32_t* bufIdx);

/* Writes from terminal to buffer */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);

/* Writes from buffer to terminal */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);

/* Terminal open (unused) */
int32_t terminal_open(const uint8_t* filename);

/* Terminal close (unused) */
int32_t terminal_close(int32_t fd);

#endif /* _TERMINAL_H */

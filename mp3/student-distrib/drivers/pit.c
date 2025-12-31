#include "pit.h"
#include "../x86_desc.h"
#include "../page.h"
#include "../syscall.h"
#include "terminal.h"

/* counter to check number of PIT interrupts, used to execute MAX_TERMINALS number of base shells */
static uint32_t pit_counter;

/* PIT_init() - PIT device initialization
 * 
 * Inputs: NONE
 * Outputs: NONE
 * Side Effects: initializes the PIT to fire interrupts at 100Hz
 */
void init_PIT() {
    /* initting at 100 hz */
    cli();

    uint16_t rate_const = 1193180 / 100;    /* Calculate our divisor */
    outb(0x36, PIT_PORT+3);                 /* Set our command byte 0x36 */
    outb(rate_const & 0xFF, PIT_PORT);      /* Set low byte of divisor */
    outb(rate_const >> 8, PIT_PORT);        /* Set high byte of divisor */

    pit_counter = 0;

    enable_irq(PIT_IRQ); // an interrupt will fire immediately after unmasking interrupts
    sti();
}

/* active terminal from terminal.c */
extern uint32_t TA_idx;
extern terminal_t* terminal_active;

/* PIT_Handler() - PIT interrupt handler
 * 
 * Inputs: NONE
 * Outputs: NONE
 * Side Effects: On the first 3 PIT interrupts from init, it executes 3 base shells. Every PIT interrupt changes the actively executing terminal
 * in a round robin fashion and changes the user video page. 
 */
void PIT_Handler() {
    // acknowledge the interrupt to allow further PIT interrupts
    send_eoi(PIT_IRQ);
    
    // save current terminal's PIT_Handler EBP, needed to exit out of later during swtch_ctx
    register uint32_t saved_ebp asm("ebp");
    terminal_active->saved_ebp = saved_ebp;

    // execute first 3 base shells
    if (pit_counter < 3) {
        // set active terminal
        set_active_terminal(pit_counter);
        // increment pit counter
        pit_counter++;

        // execute base shell
        execute((uint8_t*)"shell");
    }

    // round robin scheduling
    set_active_terminal((TA_idx + 1) % MAX_TERMINALS);

    // update virtual user page for video memory
    set_video_mem_page(current_PCB->vidmap_inuse);

    // context switch to other kernel stack
    swtch_ctx(terminal_active->saved_ebp);

    // we should never enter here, swtch_ctx should exit PIT_Handler for us
    printf("get out");
}

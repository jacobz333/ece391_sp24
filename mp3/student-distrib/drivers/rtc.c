#include "../lib.h"
#include "rtc.h"
#include "fsys.h"
#include "terminal.h"

/* file operations jumptable for rtc */
fops_jumptable_t rtc_jmptable = {
    rtc_read,
    rtc_write,
    rtc_open,
    rtc_close
};

/*
 * refer to https://courses.engr.illinois.edu/ece391/sp2024/secure/references/mc146818.pdf for specific register bit information
 * RTC programming rules
 *    - all interrupts and NMIs must be disabled, or else we may leave RTC device in an undefined/unrecoverable state
 *    - whenever writing onto port 0x70, CMOS RTC expects a read or write from port 0x71 or else the device may go into an undefined state
 *    - reading or writing from port 0x71 will reset the selected register, meaning we must reselect the register by writing to port 0x70
 */

/* NMI_enable - enable NMIs
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: enables NMIs
 */
static void NMI_enable() {
    outb(0x70, inb(0x70) & 0x7F);       // unset bit [7] when writing port 0x70 to enable NMI
    inb(0x71);                          // CMOS RTC expects a read from port 0x71 or else it may go into undefined state
}

/* NMI_disable - disable NMIs
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: disables NMIs
 */
static void NMI_disable() {
    outb(0x70, inb(0x70) | 0x80);       // set bit [7] when writing port 0x70 to disable NMI
    inb(0x71);                          // CMOS RTC expects a read from port 0x71 or else it may go into undefined state
}

/* init_RTC - RTC initialization
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: alters the rtc chip by writing and initalizing values to it
 *               starts the rtc frequency at 512 Hz for rtc_counter
 */
void init_RTC(void) { //call this only on initialization during the non CLI
    uint8_t prev_regA;  /* variable to store previous value of RTC register A */
    uint8_t prev_regB;  /* variable to store previous value of RTC register B */
    uint32_t save;      /* variable to store flags */

    // an interrupt has not occurred yet!
    rtc_counter = 0;

    // save flags and disable interrupts and NMIs for RTC programming
    cli_and_save(save);
    NMI_disable();

    // turn on rtc interrupts; setting PIE Periodic Interrupt Enable bit [6] of RTC register B
    outb(0x8B, RTC_PORT);		                // select register B, keep NMI disabled
    prev_regB = (uint8_t) inb(RTC_PORT + 1);	// read and save the current value of register B
    outb(0x8B, RTC_PORT);		                // reselect register B, keep NMI disabled
    outb(prev_regB | 0x40, RTC_PORT + 1);	    // write the previous register B value ORed with 0x40. This turns on bit [6] of register B

    // set RTC interrupt rate to 1024 Hz; write 0x6=0b0110 RS bits [3:0] of RTC register A, which corresponds to 1024 Hz
    outb(0x8A, RTC_PORT);		                    // select register A, keep NMI disabled
    prev_regA = (uint8_t) inb(RTC_PORT + 1);	    // read and save the current value of register A
    outb(0x8A, RTC_PORT);		                    // reselect register A, keep NMI disabled
    outb((prev_regA & 0xF0) | 0x07, RTC_PORT + 1);	// set LSB 4 of register A

    // restore flags and reenable interrupts and NMIs
    NMI_enable();
    restore_flags(save);
    sti();

    // unmask RTC interrupts
    enable_irq(RTC_IRQ);
}

/* active terminal from terminal.c */
extern terminal_t* terminal_active;
extern terminal_t terminal_arr[MAX_TERMINALS];


/* RTC_Handler - RTC interrupt handler handles different aspects related to timing with the RTC chip
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: fires periodically at 512 hz according to the rtc handler
 */
void RTC_Handler(void) {
    uint32_t i;     /* loop index */

    // disable interrupts
    cli();

    outb(0x0C, RTC_PORT);   // required to allow another rtc interrupt to fire
    inb(RTC_PORT + 1);      // expected read from port 0x71

    // update RTC interrupt flag for all terminals at once
    for (i = 0; i < MAX_TERMINALS; i++) {
        // set the RTC interrupt flag when specified bit on rtc_counter changes
        if (CHECK_FLAG(rtc_counter, terminal_arr[i].rtc_freq_bit) != CHECK_FLAG(rtc_counter + 1, terminal_arr[i].rtc_freq_bit)) {
            terminal_arr[i].rtc_interrupt_occurred = 1;
        }
    }
    
    // increment rtc_counter
    rtc_counter++;
    
    // end interrupt
    send_eoi(RTC_IRQ);

    // enable interrupts
    sti();
}

/* rtc_read - RTC read syscall
 * 
 * Inputs: fd - unused
 *         buf - unused
 *         nbytes - unused
 * Outputs: 0 (see discussion slides)
 * Side Effects: returns only when the next interrupt fires
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes) {
    // unset interrupt occurred flag
    terminal_active->rtc_interrupt_occurred = 0;

    // wait for interrupt to fire and set interrupt occurred flag before continuing
    while (!terminal_active->rtc_interrupt_occurred) {
    }

    return 0;
}

/* rtc_write - RTC write syscall
 *      sets the RTC frequency to the given 32-bit integer in Hz
 *      only RTC frequencies that are powers of 2 between 2 and 1024 inclusive are allowed
 * 
 * Inputs: fd - unused
 *         buf - pointer to stores 32-bit integer corresponding to frequency to set to
 *         nbytes - unused
 * Outputs: 0 for success, -1 if given invalid frequency
 * Side Effects: 
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes) {
    uint32_t freq = *((uint32_t*) buf); /* get desired frequency in Hz from given buffer */
    uint32_t freq_bit;                  /* bit of counter to check */

    // check if rate is power of 2, between 2 and 512 inclusive
    if ( !( (freq & (freq - 1)) == 0 && 2 <= freq && freq <= 512) ) {
        return -1; // failure
    }

    // calculate freq_bit: freq_bit=0 <-> freq=512, freq_bit=8 <-> freq=2
    freq_bit = 8;
    while (freq != 0x02) {
        freq = freq >> 1;
        freq_bit--;
    }

    // set the frequency bit of active terminal
    terminal_active->rtc_freq_bit = freq_bit;

    return 0;
}

/* rtc_open - RTC open syscall
 * 
 * Inputs: filename - unused
 * Outputs: 0 (see discussion slides)
 * Side Effects: resets the RTC frequency to 2 Hz
 */
int32_t rtc_open(const uint8_t* filename) {
    // sets active terminals frequency bit for 2 Hz
    terminal_active->rtc_freq_bit = 8;

    return 0;
}

/* rtc_close - RTC close syscall
 * 
 * Inputs: fd - unused
 * Outputs: 0
 * Side Effects: does nothing
 */
int32_t rtc_close(int32_t fd) {
    return 0;
}

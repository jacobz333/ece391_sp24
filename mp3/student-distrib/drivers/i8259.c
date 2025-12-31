/* i8259.c - Functions to interact with the 8259 interrupt controller
 * vim:ts=4 noexpandtab
 */

#include "../lib.h"
#include "i8259.h"

/* Interrupt masks to determine which interrupts are enabled and disabled */
uint8_t master_mask; /* IRQs 0-7  */
uint8_t slave_mask;  /* IRQs 8-15 */

/* i8259_init - initializes PIC
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: inits PIC by writing to ports
 */
void i8259_init(void) {
    uint32_t save; /* variable to store flags */

    // save flags and mask interrupts (cannot interrupt PIC initialization!)
    cli_and_save(save);
    outb(0xFF, MASTER_8259_PORT + 1);
    outb(0xFF, SLAVE_8259_PORT + 1);
    
    // initialize master and slave PICs with ICWs
    outb(ICW1, MASTER_8259_PORT);
    outb(ICW1, SLAVE_8259_PORT);

    outb(ICW2_MASTER, MASTER_8259_PORT + 1);
    outb(ICW2_SLAVE, SLAVE_8259_PORT + 1);

    outb(ICW3_MASTER, MASTER_8259_PORT + 1);
    outb(ICW3_SLAVE, SLAVE_8259_PORT + 1);

    outb(ICW4, MASTER_8259_PORT + 1);
    outb(ICW4, SLAVE_8259_PORT + 1);

    // restore flags and unmask interrupts (OCW1)
    outb(0xFF, MASTER_8259_PORT + 1); // write all 1s to enable all IRQs
    outb(0xFF, SLAVE_8259_PORT + 1);
    restore_flags(save);
    sti();
}

/* enable_irq - unmasks one IRQ on a PIC, unsets a bit on said PIC's IMR
 * 
 * Inputs: uint32_t irq_num - IRQ number (0-15) based on the IA32/lecture
 * Outputs: None
 * Side Effects: enables IRQ by zeroing the correct IRQ line 
 */
void enable_irq(uint32_t irq_num) {
    uint32_t save; /* variable to store flags */
    // save flags and mask interrupts
    cli_and_save(save);

    uint16_t mask = ~(1 << irq_num); // to be ANDed to unset corresponding bit for the PIC's IMR

    if (irq_num >= 8) { // slave PIC
        slave_mask = inb(SLAVE_8259_PORT + 1) & (uint8_t)(mask >> 8); // shift mask down from [15:8] -> [7:0]
        outb(slave_mask, SLAVE_8259_PORT + 1);
        outb((~(1 << SLAVE_ID)) & inb(MASTER_8259_PORT + 1), MASTER_8259_PORT + 1); // unmask the corresponding IRQ on master PIC when the slave PIC is written to
    } else { // master PIC
        master_mask = inb(MASTER_8259_PORT + 1) & (uint8_t)(mask);
        outb(master_mask, MASTER_8259_PORT + 1);
    }

    // restore flags and unmask interrupts (OCW1)
    restore_flags(save);
    sti();
}

/* disable_irq - masks one IRQ on a PIC, sets a bit on said PIC's IMR
 * 
 * Inputs: uint32_t irq_num - IRQ number (0-15) based on the IA32/lecture
 * Outputs: None
 * Side Effects: disables IRQ line by sending new masks to the PIC
 */
void disable_irq(uint32_t irq_num) {
    uint32_t save; /* variable to store flags */
    // save flags and mask interrupts
    cli_and_save(save);

    
    uint16_t mask = 1 << irq_num; // to be ORed to set corresponding bit for the PIC's IMR
    
    if (irq_num >= 8) { // slave PIC
        slave_mask = inb(SLAVE_8259_PORT + 1) | (uint8_t)(mask >> 8); // shift mask down from [15:8] -> [7:0] 
        outb(slave_mask, SLAVE_8259_PORT + 1);
    } else { // master PIC
        master_mask = inb(MASTER_8259_PORT + 1) | (uint8_t)(mask);
        outb(master_mask, MASTER_8259_PORT + 1);
    }

    // restore flags and unmask interrupts (OCW1)
    restore_flags(save);
    sti();
}

/* send_eoi - sends EOI to port
 * 
 * Inputs: uint32_t irq_num - IRQ number (0-15) based on the IA32/lecture computes the correct EOI byte from the argument convers both slave and master
 * Outputs: None
 * Side Effects: sends EOI to PIC ports
 */
void send_eoi(uint32_t irq_num) {
    if (irq_num >= 8) { // interrupt from slave PIC
        outb(EOI | (irq_num & 0x7), SLAVE_8259_PORT);
        outb(EOI | SLAVE_ID, MASTER_8259_PORT);
    } else { // interrupt from master PIC
        outb(EOI | irq_num, MASTER_8259_PORT);
    }
}

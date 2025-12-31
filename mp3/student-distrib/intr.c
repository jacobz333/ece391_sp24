#include "lib.h"
#include "x86_desc.h"
#include "intr.h"
#include "asm_wrapper.h"
#include "syscall.h"

/* flag to determine if exception was raised during program execution */
uint8_t exception_flag;

/* populate_IDT - fills the IDT
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: fills the IDT
 */
void populate_IDT(void) {
    int i;  /* loop index */

    // zero out the IDT
    for (i = 0; i < 256; i++) {
        SET_IDT_ENTRY_TRAP_GATE(idt[i], NULL, NULL, DPL_UNPRIVILEGED, 0x0);
    }

    // setup IDT entries 0 - 19
    SET_IDT_ENTRY_TRAP_GATE(idt[0x00],                  &Divide_Error_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x01],               &Debug_Expection_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_INT_GATE (idt[0x02],                 &NMI_Interrupt_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x03],                    &Breakpoint_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x04],                      &Overflow_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x05],          &Bound_Range_Exceeded_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x06],                &Invalid_Opcode_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x07],          &Device_Not_Available_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x08],                  &Double_Fault_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x09],   &Coprocessor_Segment_Overrun_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x0A],                   &Invalid_TSS_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x0B],           &Segment_Not_Present_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x0C],           &Stack_Segment_Fault_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x0D],            &General_Protection_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x0E],                    &Page_Fault_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x0F],                &Assertion_Fail_Wrap, KERNEL_CS, DPL_UNPRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x10],      &FPU_Floating_Point_Error_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x11],               &Alignment_Check_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x12],                 &Machine_Check_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_TRAP_GATE(idt[0x13], &SIMD_Floating_Point_Exception_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);

    // setup IDT entries for PIC
    SET_IDT_ENTRY_INT_GATE (idt[0x20],                           &PIT_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_INT_GATE (idt[0x21],                      &Keyboard_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);
    SET_IDT_ENTRY_INT_GATE (idt[0x28],                           &RTC_Wrap, KERNEL_CS,   DPL_PRIVILEGED, 0x1);

    // setup syscall IDT entry
    SET_IDT_ENTRY_TRAP_GATE(idt[0x80],                   &System_Call_Wrap, KERNEL_CS, DPL_UNPRIVILEGED, 0x1);

    // initialize to 0
    exception_flag = 0;
}

/* Divide_Error_Handler - interrupt handler for divide error
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: halts running program
 */
void Divide_Error_Handler(void) {
    cli();
    printf("Divide Error\n");
    exception_flag = 1;
    sti();
    halt(NULL);
}

/* Debug_Expection_Handler - interrupt handler for debug expection
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: halts running program
 */
void Debug_Expection_Handler(void) {
    cli();
    printf("Debug Exception\n");
    exception_flag = 1;
    sti();
    halt(NULL);
}

/* NMI_Interrupt_Handler - interrupt handler for NMI interrupt
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: blue screens the OS
 */
void NMI_Interrupt_Handler(void) {
    cli();
    printf("NMI Interrupt\n");
    while (1) { // blue screen here
    };
    sti();
}

/* Breakpoint_Handler - interrupt handler for breakpoints
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: halts running program
 */
void Breakpoint_Handler(void) {
    cli();
    printf("Breakpoint\n");
    exception_flag = 1;
    sti();
    halt(NULL);
}

/* Overflow_Handler - interrupt handler for overflow
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: halts running program
 */
void Overflow_Handler(void) {
    cli();
    printf("Overflow\n");
    exception_flag = 1;
    sti();
    halt(NULL);
}

/* Bound_Range_Exceeded_Handler - interrupt handler for bound range exceeded
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: halts running program
 */
void Bound_Range_Exceeded_Handler(void) {
    cli();
    printf("Bound Range Exceeded\n");
    exception_flag = 1;
    sti();
    halt(NULL);
}

/* Invalid_Opcode_Handler - interrupt handler for invalid opcode
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: halts running program
 */
void Invalid_Opcode_Handler(void) {
    cli();
    printf("Invalid Opcode\n");
    exception_flag = 1;
    sti();
    halt(NULL);
}

/* Device_Not_Available_Handler - interrupt handler for device not available
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: halts running program
 */
void Device_Not_Available_Handler(void) {
    cli();
    printf("Device Not Available\n");
    exception_flag = 1;
    sti();
    halt(NULL);
}

/* Double_Fault_Handler - interrupt handler for double fault
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: blue screens the OS
 */
void Double_Fault_Handler(void) {
    cli();
    printf("Double Fault\n");
    while(1) { // blue screen here
    };
    sti();
}

/* Coprocessor_Segment_Overrun_Handler - interrupt handler for coprocessor segment overrun
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: blue screens the OS
 */
void Coprocessor_Segment_Overrun_Handler(void) {
    cli();
    //clear();
    printf("Coprocessor Segment Overrun\n");
    while(1) {
    };
    sti();
}

/* Invalid_TSS_Handler - interrupt handler for invalid TSS
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: blue screens the OS
 */
void Invalid_TSS_Handler(void) {
    cli();
    //clear();
    printf("Invalid TSS\n");
    while(1) {
    };
    sti();
}

/* Segment_Not_Present_Handler - interrupt handler for segment not present
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: blue screens the OS, prints old EIP and error code
 */
void Segment_Not_Present_Handler(void) {
    cli();
    //clear();
    printf("Segment Not Present\n\tECODE: %x#########################\n\tOLD EIP: %x#########################\n", ecode, oldeip);
    while(1) {
    };
    sti();
}

/* Stack_Segment_Fault_Handler - interrupt handler for stack segment fault
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: blue screens the OS
 */
void Stack_Segment_Fault_Handler(void) {
    cli();
    //clear();
    printf("Stack Segment Fault\n");
    while(1) {
    };
    sti();
}

/* General_Protection_Handler - interrupt handler for general protection
 * Inputs: None
 * Outputs: None
 * Side Effects: blue screens the OS, prints old EIP and error code
 */
void General_Protection_Handler(void) {
    cli();
    //clear();
    printf("General Protection\n\tECODE: %x\n\tOLD EIP: %x\n", ecode, oldeip);
    while(1) {
    };
    sti();
}

/* Page_Fault_Handler - interrupt handler for page fault
 * Inputs: None
 * Outputs: None
 * Side Effects: 
 */
void Page_Fault_Handler(void) {
    cli();
    //clear();
    printf("Page Fault\n\tECODE: %x\n\tOLD EIP: %x\n\tSOURCE ADDR: %x\n", ecode, oldeip, source);
    exception_flag = 1;
    sti();
    halt(NULL);
}

/* Assertion_Fail_Handler - interrupt handler for assertion fail
 * Inputs: None
 * Outputs: None
 * Side Effects: halts running program
 */
void Assertion_Fail_Handler(void) {
    cli();
    printf("Assertion Fail\n");
    exception_flag = 1;
    sti();
    halt(NULL);
}

/* FPU_Floating_Point_Error_Handler - interrupt handler for FPU floating point error
 * Inputs: None
 * Outputs: None
 * Side Effects: blue screens the OS
 */
void FPU_Floating_Point_Error_Handler(void) {
    cli();
    //clear();
    printf("FPU Floating Point Error\n");
    while(1) {
    };
    sti();
}

/* Alignment_Check_Handler - interrupt handler for alignment check
 * Inputs: None
 * Outputs: None
 * Side Effects: blue screens the OS
 */
void Alignment_Check_Handler(void) {
    cli();
    //clear();
    printf("Alignment Check\n");
    while(1) {
    };
    sti();
}

/* Machine_Check_Handler - interrupt handler for machine check
 * Inputs: None
 * Outputs: None
 * Side Effects: blue screens the OS
 */
void Machine_Check_Handler(void) {
    cli();
    //clear();
    printf("Machine Check\n");
    while(1) {
    };
    sti();
}

/* SIMD_Floating_Point_Exception_Handler - interrupt handler for SIMD floating point exception
 * Inputs: None
 * Outputs: None
 * Side Effects: blue screens the OS
 */
void SIMD_Floating_Point_Exception_Handler(void) {
    cli();
    //clear();
    printf("SIMD Floating Point Exception\n");
    while(1) {
    };
    sti();
}

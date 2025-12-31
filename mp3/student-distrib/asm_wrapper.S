/* asm_wrapper.S
 * defines convenient x86 wrappers for interrupt handlers
 * 
 * vim:ts=4 noexpandtab
 */
#include "asm_wrapper.h"

/* debugging variables */
.globl ecode, oldeip, source
.align	4

ecode:
    .long 0
source:
    .long 0

oldeip:
    .long 0
.align	4

# interrupt handler x86 wrappers            interrupt vector numbers
.globl  Divide_Error_Wrap                   # 0x00
.globl  Debug_Expection_Wrap                # 0x01 (only present in real mode)
.globl  NMI_Interrupt_Wrap                  # 0x02
.globl  Breakpoint_Wrap                     # 0x03
.globl  Overflow_Wrap                       # 0x04
.globl  Bound_Range_Exceeded_Wrap           # 0x05
.globl  Invalid_Opcode_Wrap                 # 0x06
.globl  Device_Not_Available_Wrap           # 0x07
.globl  Double_Fault_Wrap                   # 0x08
.globl  Coprocessor_Segment_Overrun_Wrap    # 0x09 (only present in protected mode)
.globl  Invalid_TSS_Wrap                    # 0x0A
.globl  Segment_Not_Present_Wrap            # 0x0B
.globl  Stack_Segment_Fault_Wrap            # 0x0C
.globl  General_Protection_Wrap             # 0x0D
.globl  Page_Fault_Wrap                     # 0x0E
.globl  Assertion_Fail_Wrap                 # 0x0F (Intel reserved, but used for assertion failure in tests.c)
.globl  FPU_Floating_Point_Error_Wrap       # 0x10 
.globl  Alignment_Check_Wrap                # 0x11
.globl  Machine_Check_Wrap                  # 0x12
.globl  SIMD_Floating_Point_Exception_Wrap  # 0x13

.globl  PIT_Wrap                            # 0x20
.globl  RTC_Wrap                            # 0x21
.globl  Keyboard_Wrap                       # 0x28

.globl  System_Call_Wrap                    # 0x80

.align 4

/*
 * Divide Error x86 Wrapper (Interrupt Vector 0x00)
 */
Divide_Error_Wrap:
    SAVE_REG
    call    Divide_Error_Handler
    RESTORE_REG
    iret

/*
 * Debug Exception x86 Wrapper (Interrupt Vector 0x01)
 */
Debug_Expection_Wrap:
    SAVE_REG
    call    Debug_Expection_Handler
    RESTORE_REG
    iret

/*
 * NMI Interrupt x86 Wrapper (Interrupt Vector 0x02)
 */
NMI_Interrupt_Wrap:
    SAVE_REG
    call    NMI_Interrupt_Handler
    RESTORE_REG
    iret

/*
 * Breakpoint x86 Wrapper (Interrupt Vector 0x03)
 */
Breakpoint_Wrap:
    SAVE_REG
    call    Breakpoint_Handler
    RESTORE_REG
    iret

/*
 * Overflow x86 Wrapper (Interrupt Vector 0x04)
 */
Overflow_Wrap:
    SAVE_REG
    call    Overflow_Handler
    RESTORE_REG
    iret

/*
 * Bound Range Exceeded x86 Wrapper (Interrupt Vector 0x05)
 */
Bound_Range_Exceeded_Wrap:
    SAVE_REG
    call    Bound_Range_Exceeded_Handler
    RESTORE_REG
    iret

/*
 * Invalid Opcode x86 Wrapper (Interrupt Vector 0x06)
 */
Invalid_Opcode_Wrap:
    SAVE_REG
    call    Invalid_Opcode_Handler
    RESTORE_REG
    iret

/*
 * Device Not Available x86 Wrapper (Interrupt Vector 0x07)
 */
Device_Not_Available_Wrap:
    SAVE_REG
    call    Device_Not_Available_Handler
    RESTORE_REG
    iret

/*
 * Double Fault x86 Wrapper (Interrupt Vector 0x08)
 *      error code of 0 is pushed onto the stack prior
 */
Double_Fault_Wrap:
    SAVE_REG
    call    Double_Fault_Wrap
    RESTORE_REG
    addl    $4, %esp            # discard error code
    iret

/*
 * Coprocessor Segment Overrun x86 Wrapper (Interrupt Vector 0x09)
 *      Note: add 4 to ESP?
 */
Coprocessor_Segment_Overrun_Wrap:
    SAVE_REG
    call    Coprocessor_Segment_Overrun_Handler
    RESTORE_REG
    addl    $4, %esp
    iret

/*
 * Invalid TSS x86 Wrapper (Interrupt Vector 0x0A)
 */
Invalid_TSS_Wrap:
    SAVE_REG
    call    Invalid_TSS_Handler
    RESTORE_REG
    iret

/*
 * Segment Not Present x86 Wrapper (Interrupt Vector 0x0B)
 *      Note: ???
 */
Segment_Not_Present_Wrap:
    pushl   %eax
    movl	(%esp), %eax
    movl	%eax, ecode 
    movl	4(%esp), %eax
    movl	%eax, oldeip
    popl	%eax 
    SAVE_REG
    call    Segment_Not_Present_Handler
    RESTORE_REG
    addl    $4, %esp
    iret

/*
 * Stack Segment Fault x86 Wrapper (Interrupt Vector 0x0C)
 *      Note: add 4 to ESP?
 */
Stack_Segment_Fault_Wrap:
    SAVE_REG
    call    Stack_Segment_Fault_Handler
    RESTORE_REG
    addl    $4, %esp
    iret

/*
 * General_Protection x86 Wrapper (Interrupt Vector 0x0D)
 *      Note: ???
 */
General_Protection_Wrap:
    pushl   %eax
    movl	(%esp), %eax
    movl	%eax, ecode
    movl	4(%esp), %eax
    movl	%eax, oldeip
    popl	%eax 
    SAVE_REG
    call    General_Protection_Handler
    RESTORE_REG
    addl    $4, %esp
    iret

/*
 * Page Fault x86 Wrapper (Interrupt Vector 0x0E)
 *      Note: add 4 to ESP?
 */
Page_Fault_Wrap:
    movl	(%esp), %eax
    movl	%eax, ecode
    movl	4(%esp), %eax
    movl	%eax, oldeip
    movl    %cr2, %eax
    movl    %eax, source
    popl	%eax 
    SAVE_REG
    call    Page_Fault_Handler
    RESTORE_REG
    addl    $4, %esp
    iret

/*
 * Assertion Fail x86 Wrapper (Interrupt Vector 0x0F)
 */
Assertion_Fail_Wrap:
    SAVE_REG
    call    Assertion_Fail_Handler
    RESTORE_REG
    iret

/*
 * FPU Floating Point Error x86 Wrapper (Interrupt Vector 0x10)
 */
FPU_Floating_Point_Error_Wrap:
    SAVE_REG
    call    FPU_Floating_Point_Error_Handler
    RESTORE_REG
    iret

/*
 * Alignment Check x86 Wrapper (Interrupt Vector 0x11)
 *      Note: add 4 to ESP?
 */
Alignment_Check_Wrap:
    SAVE_REG
    call    Alignment_Check_Handler
    RESTORE_REG
    addl    $4, %esp
    iret

/*
 * Machine Check x86 Wrapper (Interrupt Vector 0x12)
 */
Machine_Check_Wrap:
    SAVE_REG
    call    Machine_Check_Handler
    RESTORE_REG
    iret

/*
 * SIMD Floating Point Exception x86 Wrapper (Interrupt Vector 0x13)
 */
SIMD_Floating_Point_Exception_Wrap:
    SAVE_REG
    call    SIMD_Floating_Point_Exception_Handler
    RESTORE_REG
    iret

/*
 * PIT x86 Wrapper (Interrupt Vector 0x20)
 */
PIT_Wrap:
    SAVE_REG
    call    PIT_Handler
    RESTORE_REG
    iret

/*
 * Keyboard x86 Wrapper (Interrupt Vector 0x21)
 */
Keyboard_Wrap:
    SAVE_REG
    call    Keyboard_Handler
    RESTORE_REG
    iret

/*
 * RTC x86 Wrapper (Interrupt Vector 0x28)
 */
RTC_Wrap:
    SAVE_REG
    call    RTC_Handler
    RESTORE_REG
    iret

/*
 * System Call x86 Wrapper (Interrupt Vector 0x80)
 */
System_Call_Wrap:
# save registers except EAX, EBX, ECX, EDX          
    pushl   %gs                     
    pushl   %fs
    pushl   %es
    pushl   %ds
    pushl   %ebp
    pushl   %edi
    pushl   %esi

# call syscall dispatcher
    pushl   %edx                    # caller setup
    pushl   %ecx
    pushl   %ebx
    call    System_Call_Dispatcher  # push EIP, syscall(EBX, ECX, EDX)
    addl    $12, %esp               # caller teardown

# restore registers except EAX, EBX, ECX, EDX
    popl    %esi                    
    popl    %edi
    popl    %ebp
    popl    %ds
    popl    %es
    popl    %fs
    popl    %gs
# pop user context and return to userspace
    iret                            

/* intr.h - defines common interrupt handlers and initialization
 * vim:ts=4 noexpandtab
 */
#ifndef _INTR_H
#define _INTR_H

#include "types.h"

/* external x86 wrappers from asm_wrapper.S */
extern void Divide_Error_Wrap(void);
extern void Debug_Expection_Wrap(void);
extern void NMI_Interrupt_Wrap(void);
extern void Breakpoint_Wrap(void);
extern void Overflow_Wrap(void);
extern void Bound_Range_Exceeded_Wrap(void);
extern void Invalid_Opcode_Wrap(void);
extern void Device_Not_Available_Wrap(void);
extern void Double_Fault_Wrap(void);
extern void Coprocessor_Segment_Overrun_Wrap(void);
extern void Invalid_TSS_Wrap(void);
extern void Segment_Not_Present_Wrap(void);
extern void Stack_Segment_Fault_Wrap(void);
extern void General_Protection_Wrap(void);
extern void Page_Fault_Wrap(void);
extern void Assertion_Fail_Wrap(void);
extern void FPU_Floating_Point_Error_Wrap(void);
extern void Alignment_Check_Wrap(void);
extern void Machine_Check_Wrap(void);
extern void SIMD_Floating_Point_Exception_Wrap(void);
extern void PIT_Wrap(void);
extern void Keyboard_Wrap(void);
extern void RTC_Wrap(void);
extern void System_Call_Wrap(void);

/* shared global variables for debugging */
extern uint32_t oldeip;
extern uint32_t ecode;
extern uint32_t source;
/* populates IDT entries */
void populate_IDT(void);

#endif /* _INTR_H */

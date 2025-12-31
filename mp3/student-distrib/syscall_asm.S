/* syscall_asm.S
 * defines syscall dispatcher, execute, and halt asm helpers
 * 
 * vim:ts=4 noexpandtab
 */

.globl System_Call_Dispatcher, execute_asm, halt_asm

# void System_Call_Dispatcher(uint32_t param0, uint32_t param1, uint32_t param2);
#   
# Inputs: 
#   param0 - EBX
#   param1 - ECX
#   param2 - EDX
# Outputs:
#   None
# Stack (EBP offset):
#   wrapper return address  | + 4
#   user program EBP        |   0
# 
System_Call_Dispatcher:
    cmpl	$10, %eax       # validate syscall number
    ja		_Sys_Call_Bad_Call
    cmpl    $0, %eax
    je		_Sys_Call_Bad_Call
    jmp     *_Sys_Call_Jump_Table(, %eax, 4)

_Sys_Call_Bad_Call:
    movl    $-1, %eax       # return -1 for syscall error
    ret                     # pop EIP

# 0 is a placeholder for syscall 0, which does not exist (halt is syscall 1)
_Sys_Call_Jump_Table: 
    .long 0, halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn



# void execute_asm(uint32_t ss, uint32_t esp, uint32_t eflags, uint32_t cs, uint32_t eip);
#   
# Inputs: 
#   ss - stack segment
#   esp - stack pointer
#   eflags - eflags
#   cs - code segment
#   eip - program EIP
# Outputs:
#   None
# Stack (EBP offset):
#   unsigned long eip       | +24
#   unsigned long cs        | +20
#   unsigned long eflags    | +16
#   unsigned long esp       | +12
#   unsigned long ss        | + 8
#   return address          | + 4
#   old EBP                 |   0
#   SS (USER_DS)            | - 4
#   ESP (132MB)             | - 8
#   EFLAGS                  | -12
#   CS (USER_CS)            | -16
#   EIP                     | -20
# 
execute_asm:
# callee setup
# ----------------------
    pushl   %ebp
    movl    %esp, %ebp
# function body
# ----------------------
# push iret context onto stack
    pushl   8(%ebp)
    pushl   12(%ebp)
    pushl   16(%ebp)
    pushl   20(%ebp)
    pushl   24(%ebp)
# iret, should also pop iret context off the stack
    iret

_execute_asm_loop: # should never enter here really
    hlt
    jmp     _execute_asm_loop
# callee teardown, should never enter here really
# ----------------------
    leave
    ret


# void halt_asm(uint32_t parent_ebp, uint32_t ret_val);
#   this function should never return back to its caller
#   uses the given EBP and return value to return the parent's program execute syscall to the wrapper
# Inputs: 
#   parent_ebp - EBP of the parent's call on execute
#   ret_val - value to return
# Outputs:
#   None
# Stack (EBP offset):
#   unsigned long ret_val       | +12
#   unsigned long parent_ebp    | + 8
#   return address              | + 4
#   old EBP                     |   0
halt_asm:
# callee setup
# ----------------------
    pushl   %ebp
    movl    %esp, %ebp
# function body
# ----------------------
    movl    12(%ebp), %eax      # EAX <- retval

    movl    8(%ebp), %ebp       # EBP <- parent_ebp
    movl    %ebp, %esp          # ESP <- parent_ebp

    leave                       # ESP <- parent_ebp + 4, EBP <- M[parent_ebp]
    ret                         # pop EIP (repoints EIP to the parent process)

_halt_asm_loop: # should never enter here really
    hlt
    jmp     _halt_asm_loop

# callee teardown, should never enter here really
# ----------------------
    leave
    ret

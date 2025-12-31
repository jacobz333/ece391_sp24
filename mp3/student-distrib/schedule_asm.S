.globl swtch_ctx

# void swtch_ctx(uint32_t saved_ebp);
#   
# Inputs: saved_ebp - saved EBP of the other PID_Handler to switch to
#   
# Outputs:
#   None
# Stack (EBP offset):
#   saved_ebp                   | + 8
#   PIT_Handler return address  | + 4
#   PIT_Handler EBP             |   0
# 
swtch_ctx:
# callee setup
# ----------------------
    pushl   %ebp
    movl    %esp, %ebp
# function body
# ----------------------
    movl    8(%ebp), %ebp       # EBP <- saved_ebp
    movl    %ebp, %esp          # ESP <- saved_ebp

    leave                       # ESP <- saved_ebp + 4, EBP <- M[saved_ebp]
    ret                         # pop EIP (send us back to PIT_Wrap)

_swtch_ctx_loop: # should never enter here
    hlt
    jmp     _swtch_ctx_loop

# callee teardown, should never enter here
# ----------------------
    leave
    ret

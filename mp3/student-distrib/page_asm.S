/* page_asm.S
 * defines x86 functions page initialization
 * due to control registers only accessible thru assembly
 * 
 * vim:ts=4 noexpandtab
 */

.globl set_paging_regs, enable_paging, flush_tlb
.align 4

/*
* void set_paging_regs(void)
* description: sets CR registers to values that configure paging
* inputs: none
* outputs none
* side effects: sets the registers cr4 and cr3
*/
set_paging_regs:
# callee setup
# ----------------------
    pushl   %ebp
    movl	%esp, %ebp
# function body
# ----------------------
    # configure CR4 register
    movl    %cr4, %eax
    orl	    $0x00000010, %eax   # enable PSE bit to support both 4KB and 4MB pages
    andl    $0xFFFFFFDF, %eax   # disable PAE bit
    movl    %eax, %cr4

    # set page directory base address in CR3
    movl    %cr3, %eax
    leal    page_dir, %edx
    orl     %edx, %eax          # set CR3[31:12] to address of page_dir
    movl    %eax, %cr3
# callee teardown
# ----------------------
    leave
    ret


/*
* void enable_paging(void)
* description: sets CR register 0 to enable paging. Called after configuration of page directory
* inputs: none
* outputs none
* side effects: sets the registers cr0
*/
enable_paging:
# callee setup
# ----------------------
    pushl   %ebp
    movl	%esp, %ebp
# function body
# ----------------------
    movl    %cr0, %eax
    orl	    $0x80000001, %eax   # set PG and PE bits to enable paging and protected mode on (both need to be on, see IA32 Section 2.5)
    movl    %eax, %cr0
# callee teardown
# ----------------------
    leave
    ret


/* void flush_tlb(void) - flushes the TLB
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: refresh CR3 to flush the TLB
 */
flush_tlb: # 
# callee setup
# ----------------------
    pushl   %ebp
    movl	%esp, %ebp
# function body
# ----------------------
    movl    %cr3, %eax      # flush the TLB by refreshing CR3
    movl    %eax, %cr3
# callee teardown
# ----------------------
    leave
    ret

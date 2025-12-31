#include "lib.h"
#include "x86_desc.h"
#include "page.h"
#include "./drivers/terminal.h"

/* page tables, 4KB aligned */
pte_desc_t page_table[PAGE_TABLE_NUM] __attribute__ ((aligned (4096)));
pte_desc_t user_page_table[PAGE_TABLE_NUM] __attribute__ ((aligned (4096)));

/* init_Paging - paging initialization
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: initializes paging
 */
void init_Paging(void) {
    uint32_t i; /* loop index */
    
    // set up paging registers
    set_paging_regs();

    // zero the entire page table
    for (i = 0; i < PAGE_TABLE_NUM; i++) {
        SET_PT_ENTRY(page_table[i], _4KB*i, 0x0, PAGE_UNPRIVILEGED, 0x1, 0x0); // each page is 4KB
    }
    
    // initialize the video memory page
    SET_PT_ENTRY(page_table[VMEM_BASE_ADDR >> 12], VMEM_BASE_ADDR, 0x0, PAGE_PRIVILEGED, 0x1, 0x1);
    
    // initialize the saved video memory pages
    for (i = 0; i < MAX_TERMINALS; i++) {
        SET_PT_ENTRY(page_table[(VMEM_BASE_ADDR >> 12) + i + 1], VMEM_BASE_ADDR + (i+1)*_4KB, 0x0, PAGE_PRIVILEGED, 0x1, 0x1);
    }

    // initialize the page directory
    SET_4KB_PD_ENTRY(page_dir[0], page_table, 0x0, PAGE_PRIVILEGED, 0x1, 0x1);
    // initialize the kernel page (which is stored directly in page directory)
    SET_4MB_PD_ENTRY(page_dir[1], KERNEL_MEM_BASE_ADDR, 0x0, PAGE_PRIVILEGED, 0x1, 0x1);

    // enable paging
    enable_paging();
}

/* set_user_page - sets a 4MB page at virtual address [128MB, 132MB)
 * 
 * Inputs: pid - pid of user process
 * Outputs: None
 * Side Effects: flushes the TLB, changes the user page to the given pid
 */
void set_user_page(uint32_t pid) { 
    // places PID page
    SET_4MB_PD_ENTRY(page_dir[USER_MEM_PD_ENTRY], USER_MEM_BASE_ADDR + pid * _4MB, 0x0, PAGE_UNPRIVILEGED, 0x1, 0x1);
    
    // flush TLB everytime we change the page directory
    flush_tlb();
}

/* active and shown terminal indices from terminal.c */
extern uint32_t TA_idx;
extern uint32_t TS_idx;

/* set_video_mem_page - sets a 4KB page at for user video memory [VIRTUAL_VMEM_BASE_ADDR, VIRTUAL_VMEM_BASE_ADDR + 4KB)
 * 
 * Inputs: present - the user video memory page should be present (1) or not present (0)
 * Outputs: None
 * Side Effects: flushes the TLB
 */
void set_video_mem_page(uint32_t present) {
    // set up user page table to hold the user video page
    SET_4KB_PD_ENTRY(page_dir[USER_MEM_PD_ENTRY+1], user_page_table, 0x0, PAGE_UNPRIVILEGED, 0x1, present & 1);

    // set the video memory page according to shown and active terminals
    if (TA_idx != TS_idx) {
        SET_PT_ENTRY(user_page_table[(VIRTUAL_VMEM_BASE_ADDR >> 12) & 0x3FF], VMEM_BASE_ADDR + (TA_idx+1)*_4KB, 0x0, PAGE_UNPRIVILEGED, 0x1, present & 1);
    } else {
        SET_PT_ENTRY(user_page_table[(VIRTUAL_VMEM_BASE_ADDR >> 12) & 0x3FF], VMEM_BASE_ADDR, 0x0, PAGE_UNPRIVILEGED, 0x1, present & 1);
    }
    
    // flush TLB everytime we change the page table
    flush_tlb();
}

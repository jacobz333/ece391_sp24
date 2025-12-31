/* page.h - page register and page initialization functions
 * vim:ts=4 noexpandtab
 */
#ifndef _PAGE_H
#define _PAGE_H

#include "lib.h"

/* useful macros */
#define _4KB                    0x00001000
#define _4MB                    0x00400000

/* beginning address of video memory page */
#define VMEM_BASE_ADDR          0x000B8000
/* beginning address of kernel 4MB page */
#define KERNEL_MEM_BASE_ADDR    0x00400000
/* User programs start a 8mb physical memory */
#define USER_MEM_BASE_ADDR      0x00800000
/* User programs start a 128MB virtual memory */
#define VIRTUAL_USER_BASE_ADDR  0x08000000
/* User programs can use this fixed virtual address to access video memory (arbitrary) */
#define VIRTUAL_VMEM_BASE_ADDR  0x08401000

/* 128MB virtual address, page directory is divided up into 4mb slices 128mb/4mb = 32 */
#define USER_MEM_PD_ENTRY       32

/* number of page directory entries */
#define PAGE_DIR_NUM       1024 // number of page directory entries in page directory
#define PAGE_TABLE_NUM     1024 // number of page table entires in page table

/* Page privilege levels */
#define PAGE_PRIVILEGED     0x0
#define PAGE_UNPRIVILEGED   0x1

/* x86 paging initialization functions from page_asm.S */
extern void set_paging_regs(void);
extern void enable_paging(void);
extern void flush_tlb(void);

/* paging initialization function */
void init_Paging(void);

/* sets a user page using virtual addresses [128MB, 132MB) */
void set_user_page(uint32_t);

/* sets a user page for video memory access */
void set_video_mem_page(uint32_t present);

#endif /* _PAGE_H */

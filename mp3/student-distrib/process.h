#ifndef _PROCESS_H
#define _PROCESS_H

#include "lib.h"
#include "./drivers/fsys.h"

#define MAX_PROCESSES 8             // 8 even tho the MP doc wants to support up to 6 simultaneous processes
#define PROGRAM_START 0x00048000    // address to load user program to
#define _8KB          8192          // 8kb constant for determining pcb location
#define MAX_FDS       8             // max number of fds

/* process control block struct, contains parent info for returning and file info for running */
typedef struct pcb_t {
    uint32_t id;                    // process ID (same as pid)
    uint32_t parent_pid;            // pid of parent process
    uint32_t parent_esp0;           // ESP0 of the parent process
    uint32_t parent_ebp;            // EBP of the parent process
    uint8_t open_files;             // one hot encoded for unused (0) and used (1) file descriptors
    uint32_t vidmap_inuse;          // flag to determine if process is using (1) or not using (0) video memory
    char cmd_args[129];             // arguments into the program
    file_desc_t file_desc_arr[8];   // file descriptor array
} pcb_t;

uint32_t activeProcesses;   // one hot encoded for inactive (0) and active (1) processes
uint32_t currentPID;        // pid of the current process
pcb_t* current_PCB;         // pcb of the current process

/* Helper to restore parent process */
int32_t restore_parent(void);

#endif /* _PROCESS_H */

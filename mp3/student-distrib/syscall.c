#include "lib.h"
#include "x86_desc.h"

#include "syscall.h"
#include "asm_wrapper.h"
#include "process.h"
#include "page.h"
#include "intr.h"

#include "./drivers/fsys.h"
#include "./drivers/rtc.h"
#include "./drivers/terminal.h"

/* TSS struct */
extern tss_t tss;
/* jumptables from their respective drivers */
extern fops_jumptable_t rtc_jmptable;
extern fops_jumptable_t file_jmptable;
extern fops_jumptable_t directory_jmptable;
extern fops_jumptable_t stdin_jmptable;
extern fops_jumptable_t stdout_jmptable;
/* one hot encoded for inactive (0) and active (1) processes */
extern uint32_t activeProcesses;
/* flag to determine if exception was raised during program  execution */
extern uint8_t exception_flag;

/* stdio file descriptors, shared for all processes to copy from */
static file_desc_t stdout_file_desc = {
    &stdout_jmptable,
    0,                      // no inode associated with stdout
    0,                      // no file position associated with stdout
    0                       // set to 0, unused
};
static file_desc_t stdin_file_desc = {
    &stdin_jmptable,
    0,                      // no inode associated with stdin
    0,                      // no file position associated with stdin
    0                       // set to 0, unused
};


extern terminal_t* terminal_active;

/* halt - syscall to halt the running executable and switch contexts back to the caller's
 * 
 * Inputs: uint8_t status - return value for the execute it is running on, indicates endning status for the executable
 * Outputs: None. halt_asm should send this back to the parent program's execute
 * Side Effects: Reverts all process related variables(pages, housekeeping variables) back to the parent pcb as well as returns into the parent PID, abandoning the current PID
 */
int32_t halt(uint8_t status) {
    /* critical section to re-setup PID and processes */
    cli();

    uint32_t parent_ebp = current_PCB->parent_ebp; /* save parent's execute C function EBP */
    
    // check attempt to halt base shell
    if (currentPID < MAX_TERMINALS) {
        activeProcesses &= ~(1 << currentPID);
        execute((uint8_t*)"shell");
    }

    /* restore the parent ESP0, page, and other stuff */
    restore_parent();

    // update current terminal's esp0
    terminal_active->saved_esp0 = tss.esp0;
    // update current PCB/PID
    terminal_active->currentPCB = current_PCB;
    terminal_active->currentPID = currentPID;

    // check if program was halted from exception or not
    if (exception_flag) {
        exception_flag = 0;
        sti(); /* critical section end */
        halt_asm(parent_ebp, 256); // return 256 when halting from exception
    } else {
        sti(); /* critical section end */
        halt_asm(parent_ebp, (uint32_t) status);
    }
    
    // should never go here
    return -1;
}

/* execute - does a full context switch into the command indicated by a text array
 * 
 * Inputs: uint8_t* status - char buff containing the command corresponding arguments for execution
 * Outputs: uint32_t, indicates successful execute
 * Side Effects: generates a new PCB and PID. sets all the return values to be able to restore context as well as copying user program into memory and swapping the page
 */
int32_t execute(const uint8_t* command) {
    uint32_t i;                     /* loop index */
    uint32_t args_idx;              /* starting index inside command buffer for arguments */
    char exe_fname[33];             /* executable file name */
    char args[128];                 /* arguments from command */
    dentry_t dentry;                /* dentry to fill */
    uint32_t file_size;             /* executable file size in bytes */
    pcb_t* process_pcb;             /* PCB of process to be executed */
    uint32_t program_eip;           /* EIP recovered from executable bytes [24, 27] */
    int8_t pid = -1;                /* pid of process to be executed, -1 means not assign/not found */
    char temp_buf[3];               /* temporary buffer to fill when we check for the ELF magic string */
    char elf_str[4] = "ELF";        /* ELF magic string to check with */

    /* critical section to set up PID and processes */
    cli();
    /* 
     * parse commands arguments (space delimited)
     *      first arg is executable file name
     *      other args are arguments to the executable itself
     */ 
    args_idx = 0;
    // find the first space or end of string
    while (command[args_idx] != ' ' && command[args_idx] != '\0') {
        args_idx++;
    }
    // copy the executable file name (up to 32 bytes)
    if (args_idx >= 32) {
        strncpy(exe_fname, (int8_t*)command, 32);
        exe_fname[32] = '\0';
    } else {
        strncpy(exe_fname, (int8_t*)command, args_idx);
        exe_fname[args_idx] = '\0';
    }
    // copy the rest of the arguments 
    strncpy(args, (int8_t*)&command[args_idx], 128 - args_idx);

    // check if filename from argument is valid and fill dentry
    if (read_dentry_by_name((uint8_t*)exe_fname, &dentry) == -1) {
        sti(); // unmask interrupts before we leave
        return -1;
    }

    // check if file is executable file by checking the ELF magic bytes
    if (read_data(dentry.inode_idx, 1, (uint8_t*)temp_buf, 3) == -1) { // try reading from disk
        sti(); // unmask interrupts before we leave
        return -1;
    }
    if (strncmp(elf_str, temp_buf, 3)) { // check ELF bytes
        sti(); // unmask interrupts before we leave
        return -1;
    }

    // try to find an active pid
    for (i = 0; i < MAX_PROCESSES; i++) {
        if (!CHECK_FLAG(activeProcesses, i)) { // check if bit i is 0
            pid = i;
            break;
        }
    }
    if (pid == -1) { // no inactive pid's available
        printf("too many processes!\n");
        sti(); // unmask interrupts before we leave
        return -1;
    }

    

    // allocate a 4MB page at physical address 8MB (virtual memory address 128MB)
    set_user_page(pid);

    // load user program from disk into allocated page - User-level Program Loader (load from FS to program page)
    if ((file_size = get_file_length(dentry.file_name)) == -1) {
        printf("failed to load executable!\n");
        // reset user page
        set_user_page(currentPID);
        sti(); // unmask interrupts before we leave
        return -1;
    }
    // copy executable file from disk to virtual memory
    if (read_data(dentry.inode_idx, 0, (uint8_t*)0x8048000, file_size) == -1) {
        printf("failed to load executable!\n");
        // reset user page
        set_user_page(currentPID);
        sti(); // unmask interrupts before we leave
        return -1;
    }

    // get the EIP for the executable from bytes [24, 27]
    if (read_data(dentry.inode_idx, 24, (uint8_t*) &program_eip, 4) == -1) {
        printf("failed to load executable!\n");
        // reset user page
        set_user_page(currentPID);
        sti(); // unmask interrupts before we leave
        return -1;
    }

    // initialize pcb and allocate kernel memory for program stack
    process_pcb = (pcb_t*) (USER_MEM_BASE_ADDR - (pid+1)*_8KB); //8mb - 8kb
    process_pcb->id = pid;
    process_pcb->parent_pid = currentPID;
    process_pcb->open_files = 0x03;                     // set bits 1 and 0 for stdio
    process_pcb->vidmap_inuse = 0;                      // not using vidmap
    process_pcb->file_desc_arr[0] = stdin_file_desc;    // fd=0 stdin
    process_pcb->file_desc_arr[1] = stdout_file_desc;   // fd=1 stdout
    if (args[0] == '\0') { // no arguments or argument too long
        process_pcb->cmd_args[0] = '\0'; // indicate no arguments
    } else {
        strcpy((int8_t*)process_pcb->cmd_args, (int8_t*)&args[1]); // copy all the arguments, ignore beginning space
    }

    // update process data
    activeProcesses |= 1 << pid;
    currentPID = pid;
    current_PCB = process_pcb;

    // save current/parent process execute EBP's to current PCB, needed to halt out of later
    register uint32_t saved_ebp asm("ebp");
    current_PCB->parent_ebp = saved_ebp;

    // set SS0 to kernel’s stack segment in the TSS (not really needed because TSS stack segment shouldnt change)
    tss.ss0 = KERNEL_DS;
    // set ESP0 to to-be-executed/child process' kernel-mode stack into the TSS
    current_PCB->parent_esp0 = tss.esp0;
    tss.esp0 = USER_MEM_BASE_ADDR - pid*_8KB; // 8KB align stack pointer for all programs
    
    // update current terminal's esp0
    terminal_active->saved_esp0 = tss.esp0;
    // update current PCB/PID
    terminal_active->currentPCB = current_PCB;
    terminal_active->currentPID = currentPID;

    sti();
    /* critical section end */

    /*
     * context switch
     *      SS - user's stack segment should be equivalent to USER_DS
     *      ESP - set the stack pointer to the bottom of the 4 MB page already holding the executable image.
     *      EFLAG - set bit 1 (always 1) and bit 9 (IF, enable interrupts for user programs) 
     *      CS - user code segment USER_CS
     *      EIP - the entry point from bytes 24-27 of the executable in the allocated page
     */
    execute_asm(USER_DS, VIRTUAL_USER_BASE_ADDR + _4MB, 0x00000202, USER_CS, program_eip); /* iret */

    // asm_halt will exit out execute for us back into the wrapper
    return -1;
}

/* read - does a read syscall on a file or function
 * 
 * Inputs: int32_t fd - integer that indexes into process FD array to find the file to read, *buf - externally provided buffer to be read into, nbytes - uint32_t* number of bytes to read
 * Outputs: uint32_t - indicates successful read 
 * Side Effects: Writes copied info from file into the buffer
 */

int32_t read(int32_t fd, void* buf, int32_t nbytes) {
    file_desc_t* file_desc_ptr; /* current file desciptor */

    // check if the fd is in range [0, 8), cannot read from stdout (fd=1)
    if (fd < 0 || fd == 1 || MAX_FDS <= fd) {
        return -1;
    }

    file_desc_ptr = &(current_PCB->file_desc_arr[fd]);

    // check fd bit of open_files for file availability
    if (!CHECK_FLAG(current_PCB->open_files, fd)) {
        return -1; // attempting to read from unopen fd
    }

    // get the file descriptor and read
    return file_desc_ptr->fops_table_ptr->read(fd, buf, nbytes);
}

/* write - does a write syscall on a file or function
 * 
 * Inputs: int32_t fd - integer that indexes into process FD array to find the file to write to, *buf - externally provided buffer to be written from, nbytes - uint32_t* number of bytes to write
 * Outputs: uint32_t - indicates successful write
 * Side Effects: Writes info from the buffer into the targted file.
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes) {
    // check if the fd is in range [1, 8), cannot write to stdin (fd=0)
    if (fd < 1 || MAX_FDS <= fd) {
        return -1;
    }

    // check fd bit of open_files for file availability
    if (!CHECK_FLAG(current_PCB->open_files, fd)) {
        return -1; // attempting to write into unopen fd
    }

    // get the file descriptor and write
    return (current_PCB->file_desc_arr[fd]).fops_table_ptr->write(fd, buf, nbytes);
}

/* open - adds a file descriptor to fd to have operations done on
 * 
 * Inputs: const uint8_t* filename - a name to the file that needs to be added to the fd array
 * Outputs: uint32_t - indicates successful open. If the file is not found, there are too many files, or the read dentry fails then we exit
 * Side Effects: alters the current pcb fd array and open files to include the newly open file
 */
int32_t open(const uint8_t* filename) {
    uint32_t fd;                                            /* index to fill within file descriptors array */
    dentry_t dentry;                                        /* dentry to fill when retrieving file data */
    file_desc_t* file_desc_ptr;                             /* current file desciptor */
    
    // calculate an available fd index by bit shifting
    for (fd = 2; fd < MAX_FDS; fd++) {
        if (!CHECK_FLAG(current_PCB->open_files, fd)) { // check bit fd of open_files 
            break; // bit fd of open_files is 0 (fd is available)
        }
    }

    // check if there are any available file descriptors
    if (fd == MAX_FDS) { // no available file descriptors
        return -1;
    }

    // try to find the given filename
    if (read_dentry_by_name((uint8_t*)filename, &dentry) == -1) {
        return -1;
    }

    // set our current file descriptor to fill
    file_desc_ptr = &(current_PCB->file_desc_arr[fd]);

    // conditionally determine jump table from file type
    switch (dentry.file_type) {
        case RTC_FILE_TYPE:
            file_desc_ptr->fops_table_ptr = &rtc_jmptable;
            break;
        case DIRECTORY_FILE_TYPE:
            file_desc_ptr->fops_table_ptr = &directory_jmptable;
            break;
        case REGULAR_FILE_TYPE:
            file_desc_ptr->fops_table_ptr = &file_jmptable;
            break;
    }
    // initialize file descriptor
    file_desc_ptr->inode_num = dentry.inode_idx;
    file_desc_ptr->file_pos = 0;      // reset file position to 0
    file_desc_ptr->flags = 0;         // set to 0, unused
    
    // call the respective file's open syscall
    if (file_desc_ptr->fops_table_ptr->open(filename) == -1) {
        return -1;
    }

    // set the corresponding bit in open_files only after we know the respective open syscall worked
    current_PCB->open_files |= 1 << fd;

    // return the filled fd
    return fd;
}

/* close - removes a file from the fd array and calls the close function assigned to the file
 * 
 * Inputs: int32_t fd - an index into the currentPCB fd array for file removal
 * Outputs: uint32_t - indicates successful close or unsccuessful close
 * Side Effects: alters the current_pcb fd array to delete the specified file descriptor off the list
 */
int32_t close(int32_t fd) {
    file_desc_t* file_desc_ptr = &(current_PCB->file_desc_arr[fd]); /* current file desciptor */

    // check if the fd is in range [2, 8), cannot close stdin, stdout
    if (fd < 2 || MAX_FDS <= fd) {
        return -1;
    }

    // check fd bit of open_files for file availability
    if (!CHECK_FLAG(current_PCB->open_files, fd)) {
        return -1; // attempting to close into unopen fd
    }

    // call the respective file's close syscall
    if (file_desc_ptr->fops_table_ptr->close(fd) == -1) {
        return -1;
    }

    // clear respective file descriptor for safety (not really needed if we never use it)
    file_desc_ptr->fops_table_ptr = NULL;
    file_desc_ptr->inode_num = 0;
    file_desc_ptr->file_pos = 0;
    file_desc_ptr->flags = 0;

    // clear the corresponding bit in open_files only after we know the respective close syscall worked
    current_PCB->open_files &= ~(1 << fd);

    // success
    return 0;
}

/* getargs - gets the arguments of a program
 * 
 * Inputs: buf - user buffer to fill
 *         nbytes - number of bytes to read
 * Outputs: 0 for success, -1 for no arguments or argument too long
 * Side Effects: fills buf with the contents of cmd_args
 */
int32_t getargs(uint8_t* buf, int32_t nbytes) {
    // check if arguments were invalid
    if (current_PCB->cmd_args[0] == '\0') {
        return -1;
    }

    // copy the command arguments
    strncpy((int8_t*)buf, current_PCB->cmd_args, nbytes);
    return 0;
}

/* vidmap - sets a user page for video memory
 * 
 * Inputs: screen_start - user pointer to give user video memory page to
 * Outputs: 0 for success, -1 for invalid user pointer
 * Side Effects: sets a video memory page, sets the vidmap_inuse flag
 */
int32_t vidmap(uint8_t** screen_start) {
    // validate screen_start is within user memory
    if ((uint32_t)screen_start < VIRTUAL_USER_BASE_ADDR || VIRTUAL_USER_BASE_ADDR + _4MB <= (uint32_t)screen_start) {
        return -1;
    }

    // set the video memory page
    set_video_mem_page(1);

    // set using vidmap flag
    current_PCB->vidmap_inuse = 1;

    // return virtual address for vidmap to user
    *screen_start = (uint8_t*)VIRTUAL_VMEM_BASE_ADDR;

    // success
    return 0;
}

/* set_handler
 * 
 * Inputs: signum - unused
 *         handler_address - unused
 * Outputs: -1
 * Side Effects: does nothing
 */
int32_t set_handler(int32_t signum, void* handler_address) {
    return -1;
}

/* sigreturn
 * 
 * Inputs: None
 * Outputs: -1
 * Side Effects: does nothing
 */
int32_t sigreturn() {
    return -1;
}

#include "x86_desc.h"
#include "process.h"
#include "page.h"
#include "syscall.h"

/* restore_parent - helper function that restores parent context
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: Reverts the esp to parent context, changes the user program page to point to the parent process, chagnes currentPCB and PID to parent
 */
int32_t restore_parent() {
    uint32_t i; /* loop index */

    // close all open files except stdin (fd=0), stdout (fd=1)
    for (i = 2; i < MAX_FDS; i++) {
        if (current_PCB->open_files & (1 << i)) {
            (current_PCB->file_desc_arr[i]).fops_table_ptr->close(i); // no error checking because close always returns 0
        }
    }

    // restore ESP0 to the ESP0 of the parent process
    tss.esp0 = current_PCB->parent_esp0;

    // masks out the current process indicating availability
    activeProcesses &= ~(1 << (current_PCB->id));
    // changes id to parent id
    currentPID = current_PCB->parent_pid;
    // changes current pcb to the parent pcb, current one is just overwritten anyway
    current_PCB = (pcb_t*) (USER_MEM_BASE_ADDR - (current_PCB->parent_pid + 1)*_8KB); //8mb - 8kb

    // set the user page to the parent PID
    set_user_page(currentPID);

    // set virtual page for vidmap
    set_video_mem_page(current_PCB->vidmap_inuse);

    return 0;
}

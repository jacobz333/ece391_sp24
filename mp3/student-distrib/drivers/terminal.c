#include "../lib.h"
#include "../x86_desc.h"
#include "terminal.h"
#include "../page.h"

#define NUM_COLS    80
#define NUM_ROWS    25
#define ATTRIB      0x7

/* file operations jumptable for stdin */
fops_jumptable_t stdin_jmptable = {
    terminal_read,
    NULL, // cannot write to stdin
    terminal_open,
    NULL  // cannot close stdin
};

/* file operations jumptable for stdout */
fops_jumptable_t stdout_jmptable = {
    NULL, // cannot read from stdout
    terminal_write,
    terminal_open,
    NULL  // cannot close stdout
};

uint32_t TA_idx;                        /* current active terminal */
uint32_t TS_idx;                        /* current shown terminal */
terminal_t* terminal_active;            /* currently active terminal ptr*/
terminal_t* terminal_shown;             /* currently shown terminal ptr*/
terminal_t terminal_arr[MAX_TERMINALS]; /* array storing all terminals */

/* init_terminals - initializes the terminal array and terminal information
 * Inputs: none
 * Outputs: none
 * Side Effects: fills the terminal array, sets active and shown terminals to 0
 */
void init_terminals() {
    uint32_t i; /* loop idx */

    /* fill terminal array */
    for (i = 0; i < MAX_TERMINALS; i++) {
        terminal_arr[i].saved_ebp = NULL; // null for initialization, filled in the first three PITs
        terminal_arr[i].saved_esp0 = USER_MEM_BASE_ADDR - i*_8KB; // 8KB-aligned stack pointers for base shell

        terminal_arr[i].currentPCB = (pcb_t*)(USER_MEM_BASE_ADDR - (i+1)*_8KB); /* terminal idx = pid */
        terminal_arr[i].currentPID = i; /* base shell pid = terminal idx */

        terminal_arr[i].cursor_x = 0;
        terminal_arr[i].cursor_y = 0;
        memset(&terminal_arr[i].keyboard_buffer, 0x00, 128); // zero keyboard buffer
        terminal_arr[i].keyboard_idx = 0;
        terminal_arr[i].terminal_newline = 0;
    
        terminal_arr[i].rtc_interrupt_occurred = 0;
        terminal_arr[i].rtc_freq_bit = 8; // terminals start with 2 Hz RTC
    }

    /* start at PID0 */
    TA_idx = 0;
    TS_idx = 0;
    terminal_active = &(terminal_arr[TA_idx]);
    terminal_shown = &(terminal_arr[TS_idx]);
}

/* set_shown_terminal - sets the shown terminal
 * Inputs: next_TS_idx - idx of next shown terminal
 * Outputs: none
 * Side Effects: saves the cursor location, swaps video memorys, restores cursor location of next shown terminal
 */
void set_shown_terminal(uint32_t next_TS_idx) {
    // save terminal cursor location
    terminal_shown->cursor_x = getX();
    terminal_shown->cursor_y = getY();

    // save video memory
    memcpy((void*)(VMEM_BASE_ADDR + (TS_idx+1)*_4KB), (void*)VMEM_BASE_ADDR, _4KB);
    // set video memory to the saved video memory
    memcpy((void*)VMEM_BASE_ADDR, (void*)(VMEM_BASE_ADDR + (next_TS_idx+1)*_4KB), _4KB);

    // update terminal shown
    TS_idx = next_TS_idx;
    terminal_shown = &(terminal_arr[TS_idx]);

    // update cursor and screen coords
    update_cursor(terminal_shown->cursor_x, terminal_shown->cursor_y);
    editScreenCoords(terminal_shown->cursor_x, terminal_shown->cursor_y);
}

/* TSS struct */
extern tss_t tss;

/* set_active_terminal - sets the active terminal
 * Inputs: next_TA_idx - idx of next active terminal
 * Outputs: none
 * Side Effects: saves the information of current active terminal
 *               restores the information of next active terminal
 *               saves and updates tss.esp0, currentPID, current_PCB
 *               remaps the virtual user page at 128MB
 */
void set_active_terminal(uint32_t next_TA_idx) {
    // save current terminal's esp0
    terminal_active->saved_esp0 = tss.esp0;
    // save current PCB/PID
    terminal_active->currentPCB = current_PCB;
    terminal_active->currentPID = currentPID;

    // save terminal screen coords
    terminal_active->cursor_x = getX();
    terminal_active->cursor_y = getY();

    // set vidmem pointer for printfs
    if (next_TA_idx != TS_idx) {
        setVidPointer((char*)(VMEM_BASE_ADDR + (next_TA_idx+1)*_4KB));
    } else {
        setVidPointer((char*)VMEM_BASE_ADDR);
    }
    
    // update terminal active
    TA_idx = next_TA_idx;
    terminal_active = &(terminal_arr[TA_idx]);

    // change screen coords and update cursor if needed
    if (next_TA_idx == TS_idx) {
        update_cursor(terminal_active->cursor_x, terminal_active->cursor_y);
    }
    editScreenCoords(terminal_active->cursor_x, terminal_active->cursor_y);

    // update esp0
    tss.esp0 = terminal_active->saved_esp0;
    // update current PCB/PID
    current_PCB = terminal_active->currentPCB;
    currentPID = terminal_active->currentPID;
    // update virtual user page mapping
    set_user_page(currentPID);
}

/* backspace - erases last character written from buffer and from the screen and resets screen coords to that location
 * 
 * Inputs: uint8_t* buf - terminal buffer pointer, uint32_t* bufIdx - index of the buffer pointing to the most recently typed character
 * Outputs: None
 * Side Effects: modifies video memory by erasing character and setting screen coords, erases the last character in the buffer and decrements the bufidx
 */
void backspace(uint8_t* buf, uint32_t* bufIdx){
    uint16_t cx = getX();   //current coords
    uint16_t cy = getY();
    uint8_t i;
    uint8_t j;
    

    uint8_t prebp;          //last character written saved
    /*does the buffer erase here checks to prevent out of bounds*/
    if(*bufIdx != 0){
        prebp = buf[(*bufIdx-1)];
        buf[(*bufIdx-1)%128] = 0x00;//erase last chracter
        *bufIdx = (*bufIdx-1);//decrement
    }
    else{
        prebp = 0;
    }
   
    /*Handles the erase size. Tab will erase more to account for the whole tab. normally one. if at beginning dont erase*/
    if(prebp == 0x9){
        j = 4;
    }
    else if(prebp != 0){
        j = 1;
    }
    else{
        j = 0;
    }

    //keyboard display portion
    
    for(i = 0; i<j; i++){
        if(cx == 0){
            if(cy == 0){
                return;
            }
            else{
                cx = NUM_COLS-1; //last character on terminal row
                cy--;
            }
        }
        else{
            cx--;
        }
    }
    
    //puts a blank chracter and edits the screen cord on that erased char
    editScreenCoords(cx,cy);
    putc(0x20);     
    editScreenCoords(cx,cy);
    update_cursor(cx, cy);
    //keyboard buffer needs to be added
    
}

/* terminal_read - reads an from the terminal into a buffer offered in parameter
 * 
 * Inputs: uint8_t* fd - file descriptor array for process, uint8_t* buf - input buffer, uint32_t n - used as a pointer to the keyboard buffer index
 * Outputs: None
 * Side Effects: places characters onto the terminal and scrolls potentially
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes){
    uint32_t i = 0;
    uint32_t blen = 0;
    
    setIdx(0);
    while (!terminal_active->terminal_newline) {    // waits for a newline flag to be raised to write the buffer
        // must wait for active terminal, should not continue if terminal shown != active
    }
    for (i = 0; i < 128; i++) {
        ((char*)buf)[i] = terminal_active->keyboard_buffer[i];
        terminal_active->keyboard_buffer[i] = 0;
        blen++;
        if (((char*)buf)[i] == '\n') {
            break;
        }
    }
    return blen;
}

/* terminal_write - writes an input buffer to the terminal given a length n
 * 
 * Inputs: uint8_t* fd - file descriptor array for process, uint8_t* buf - input buffer, uint32_t n - number of chars to write
 * Outputs: None
 * Side Effects: places characters onto the terminal and scrolls potentially
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes){
    uint32_t i;
    
    for(i = 0; i<nbytes; i++){
        if (((char*)buf)[i] != '\0') {
            putc(((uint8_t*)buf)[i]);
        }
    }
    return 0;
}

/* terminal_open - doesnt do much
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: return
 */
int32_t terminal_open(const uint8_t* filename){
    return 0;
}

/* terminal_close - doesnt do much
 * 
 * Inputs: None
 * Outputs: None
 * Side Effects: return
 */
int32_t terminal_close(int32_t fd){
    return 0;
}

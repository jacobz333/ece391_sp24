#include "../lib.h"
#include "keyboard.h"
#include "terminal.h"
#include "../page.h"

/* important ASCII references */
static uint8_t codeASCIINS[62] = {
    0x00, /* null */
    0x00, /* escape pressed */
    0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30,
    0x2D,   /* - */
    0x3D,   /* = */
    0x08,   /* backspace */
    0x09,   /* tab */
    0x71, 0x77, 0x65, 0x72, 0x74, 0x79, 0x75, 0x69, 0x6F, 0x70, 0x5B, 0x5D, /*enter as a new line*/0xA, /*control as a hashtag*/ 0x00, 0x61, 0x73, 0x64, 0x66, 0x67, 0x68, 0x6A, 0x6B, 0x6C, 0x3B, 0x27, 0x60, /*leftshift as #*/ 0x00, 0x5C, 0x7A, 0x78, 0x63, 0x76, 0x62, 0x6E, 0x6D, 0x2C, 0x2E, 0x2F,/*rightshift as a hashtag*/ 0x00, /*keypad*/ 0x2A, /*leftalt as a hashtag*/ 0x00, 0x20, 0x00, 0x00, 0x00, 0x00};
static uint8_t codeASCIIYS[62] = {
    0x00  /*null*/,
    0x00, /*escape*/
    0x21, 0x40, 0x23, 0x24, 0x25, 0x5E, 0x26, 0x2A, 0x28, 0x29,
    0x5F,   /* - */
    0x2B,   /* = */
    0x8,    /*backspace*/
    0x9,    /*tab*/
    0x51, 0x57, 0x45, 0x52, 0x54, 0x59, 0x55, 0x49, 0x4F, 0x50, 0x7B, 0x7D, /*enter as a new line*/0xA, /*control as a hashtag*/ 0x00, 0x41, 0x53, 0x44, 0x46, 0x47, 0x48, 0x4A, 0x4B, 0x4C, 0x3A, 0x22, 0x7E, /*leftshift as #*/ 0x00, 0x7C, 0x5A, 0x58, 0x43, 0x56, 0x42, 0x4E, 0x4D, 0x3C, 0x3E, 0x3F,/*rightshift as a hashtag*/ 0x00, /*keypad*/ 0x2A, /*leftalt as a hashtag*/ 0x00, /*space*/ 0x20, 0x00, 0x00, 0x00, 0x00};

/* active and shown terminals from terminal.c */
extern uint32_t TA_idx;
extern uint32_t TS_idx; 
extern terminal_t* terminal_active;
extern terminal_t* terminal_shown;

void setIdx(uint32_t v){
    terminal_active->keyboard_idx = v;
    terminal_active->terminal_newline = 0;
}


/* Keyboard_Handler - keyboard interrupt handler
 * 
 * Inputs: inb from 8042 port
 * Outputs: NONE
 * Side Effects: prints to terminal as well as updating the flags and freezing when newline is hit to signal to terminal read
 */
void Keyboard_Handler(void) {
    // disable interrupts
    cli();

    // save active terminal screen coords
    terminal_active->cursor_x = getX();
    terminal_active->cursor_y = getY();
    // set vidmem pointer to shown terminal
    setVidPointer((char*)VMEM_BASE_ADDR);
    // change screen coords to shown terminal
    editScreenCoords(terminal_shown->cursor_x, terminal_shown->cursor_y);

    uint8_t scancode = inb(KBD_DATA_PORT);
    // printf("scancode = %x\n", scancode);
    uint32_t i;
    /*This segment of of statements handles special keys like shift, caps and contro*/
    if(scancode == 0x3A) {//handles the keyboard condition codes like shift and capslock detects a capslock press to enable
        shiftcaps ^= 0x01;
        keycode = 0x00; 
    } else if (scancode == 0x36 || scancode == 0x2A) {//same logic for shift though enable and disable is controlled by press and release keys
        shiftcaps |= 0x02;
        keycode = 0x00;   
    } else if(scancode == 0xB6 || scancode == 0xAA) {//release shift
        shiftcaps &= ~0x02;
        keycode = 0x00;  
    } else if(scancode == 0x1D) {                    //press control
        shiftcaps |= 0x04;
        keycode = 0x00;
    } else if(scancode == 0x9D) {                    //release control
        shiftcaps &= ~0x04;
        keycode = 0x00;
    } else if (scancode == 0x0E) {                   // backspace
        backspace((uint8_t*)terminal_shown->keyboard_buffer, &terminal_shown->keyboard_idx);
        keycode = 0x00;
    } else if (scancode == 0x38) {                  // press alt left and right
        keycode = 0x00;
        shiftcaps |= 0x08;
    } else if (scancode == 0xB8) {                   // release alt left and right
        keycode = 0x00;
        shiftcaps &= ~0x08;
    } else if (scancode >= 0x3B + MAX_TERMINALS) {   // scancode outside of relevant range
        keycode = 0x00;
    // alt condition
    } else if ((shiftcaps & 0x08) > 0) {
        if (0x3B <= scancode && scancode < 0x3B + MAX_TERMINALS) {
            set_shown_terminal(scancode - 0x3B);    // use offset from 0x3b to find correct F#
        }
    /* this section converts scancodes and flags into valid keycodes */
    } else { // typable characters
        if (codeASCIINS[scancode] < 0x7B && codeASCIINS[scancode] > 0x60) {//handles the shifting and capslock for the characters
            if ((shiftcaps & 0x04) > 0 && scancode == 0x26){//CTRL + L
                editScreenCoords(0,0);
                clear();
                update_cursor(0,0);
                keycode = 0x00;
                for(i = 0; i<128; i++){
                    terminal_shown->keyboard_buffer[i] = 0x00;
                }
                terminal_shown->keyboard_idx = 0;
            } 
            else if ((shiftcaps & 0x02)>>1 != (shiftcaps & 0x01)) {//shift + character
                keycode = codeASCIIYS[scancode];
            } else {                                                //std character
                keycode = codeASCIINS[scancode];
            }
            
        } else {//non letter characters
            if ((shiftcaps & 0x02)) {
                keycode = codeASCIIYS[scancode];
            } else {
                keycode = codeASCIINS[scancode];
            }
        }
    }

    /*This section determines the use of the valid character*/
    if (keycode != 0x00) {
        // printf("%c", keycode)
        if(terminal_shown->keyboard_idx == 127){
            if(keycode == '\n' || terminal_shown->terminal_newline){//Forces a do nothing after read completion occurs
                terminal_shown->keyboard_buffer[127] = '\n';
                printf("%c", keycode);
                terminal_shown->terminal_newline = 1; 
            }
        }
        else{

            terminal_shown->keyboard_buffer[terminal_shown->keyboard_idx] = keycode;
            terminal_shown->keyboard_idx++;
            printf("%c", keycode);
            if(keycode == '\n' || terminal_shown->terminal_newline){//Forces a do nothing after read completion occurs
                terminal_shown->terminal_newline = 1; 
            } 
        }
        
    }

    // save shown terminal screen coords
    terminal_shown->cursor_x = getX();
    terminal_shown->cursor_y = getY();
    // set vidmem pointer to active terminal if not shown
    if (TA_idx != TS_idx) {
        setVidPointer((char*)(VMEM_BASE_ADDR + (TA_idx+1)*_4KB));
    }
    
    // change screen coords
    editScreenCoords(terminal_active->cursor_x, terminal_active->cursor_y);

    // end interrupt
    send_eoi(KBD_IRQ);

    // enable interrupts
    sti();
}

/* init_Keyboard - keyboard initialization
 * 
 * Inputs: None
 * Outputs: None
 */
void init_Keyboard(void) {
    // disable interrupts (cannot interrupt keyboard initialization)
    cli();

    // unmask keyboard interrupts
    enable_irq(KBD_IRQ);

    // enable interrupts
    sti();
}

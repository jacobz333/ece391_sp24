/* keyboard.h - defines convenient ASCII references and keyboard initialization
 * vim:ts=4 noexpandtab
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "../types.h"
#include "../lib.h"
#include "i8259.h"

#define KBD_DATA_PORT   0x60
#define KBD_IRQ         1

/* important keyboard variables */
uint8_t shiftcaps; //|bit7-2 = 0||shift(1 shift down 0 shift up)||caps(caps enabled)(caps disabled)|
uint8_t keycode;
uint8_t scancode;


/*resets the current writing line and newline flag*/
void setIdx(uint32_t v);

/* interrupt handler for keyboard */
void Keyboard_Handler(void);

/* initializes the keyboard */
void init_Keyboard(void);

#endif /* _KEYBOARD_H */

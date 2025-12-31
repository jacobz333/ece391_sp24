// All necessary declarations for the Tux Controller driver must be in this file

#ifndef TUXCTL_H
#define TUXCTL_H

#define TUX_SET_LED _IOR('E', 0x10, unsigned long)
#define TUX_READ_LED _IOW('E', 0x11, unsigned long*)
#define TUX_BUTTONS _IOW('E', 0x12, unsigned long*)
#define TUX_INIT _IO('E', 0x13)
#define TUX_LED_REQUEST _IO('E', 0x14)
#define TUX_LED_ACK _IO('E', 0x15)

/* convenient bit masks */
#define MASK_LOW_4			0x0F	// mask lower 4 bits
#define MASK_HIGH_4			0xF0	// mask upper 4 bits

/*
 * button buffer format (active high):
 * ____7_______6______5____4____3___2___1_____0____
 * | right | left | down | up | C | B | A | START |
 * ------------------------------------------------
 */
#define TUX_RIGHT_BTN_MASK  (1 << 7)
#define TUX_LEFT_BTN_MASK   (1 << 6)
#define TUX_DOWN_BTN_MASK   (1 << 5)
#define TUX_UP_BTN_MASK     (1 << 4)
#define TUX_C_BTN_MASK      (1 << 3)
#define TUX_B_BTN_MASK      (1 << 2)
#define TUX_A_BTN_MASK      (1 << 1)
#define TUX_START_BTN_MASK  (1 << 0)

#endif


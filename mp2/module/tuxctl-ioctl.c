/* tuxctl-ioctl.c
 *
 * Driver (skeleton) for the mp2 tuxcontrollers for ECE391 at UIUC.
 *
 * Mark Murphy 2006
 * Andrew Ofisher 2007
 * Steve Lumetta 12-13 Sep 2009
 * Puskar Naha 2013
 */

#include <asm/current.h>
#include <asm/uaccess.h>

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/file.h>
#include <linux/miscdevice.h>
#include <linux/kdev_t.h>
#include <linux/tty.h>
#include <linux/spinlock.h>

#include "tuxctl-ld.h"
#include "tuxctl-ioctl.h"
#include "mtcp.h"

#define debug(str, ...) \
	printk(KERN_DEBUG "%s: " str, __FUNCTION__, ## __VA_ARGS__)

/*
 * struct to store TUX information
 */
typedef struct {
	struct tty_struct* tty; 	// tty shared across functions 
	bool button_interrupt_gen;	// track when tux is generating button interrupts
	char last_LEDs[4];			// track last LEDs
	bool LED_user_mode;			// track LED user mode
	bool setting_LEDs;			// track when tux is setting LEDs
	/*
	 * button buffer format (active high):
	 * ____7_______6______5____4____3___2___1_____0____
	 * | right | left | down | up | C | B | A | START |
	 * ------------------------------------------------
	 */
	char button_buf;			// byte storing the button state
} tux_info_t;

static tux_info_t tux_info;

/* set LEDs bit offsets */
#define LED_DP_BIT			4
#define LED_DP_OFFSET		24
#define LED_MASK_OFFSET 	16

/*
 * byte array storing hexadecimal to 7-segment display mapping.
 * Assumes dp bit is 0
 * 
 * Mapping from 7-segment to bits
 * 	The 7-segment display is:
 *		  _A
 *		F| |B
 *		  -G
 *		E| |C
 *		  -D .dp
 *	The map from bits to segments is:
 * 	__7___6___5___4____3___2___1___0__
 * 	| A | E | F | dp | G | C | B | D | 
 * 	+---+---+---+----+---+---+---+---+
 */
static char hex_to_led[16] = {
	0xE7, // 0b11100111 - 0x0
	0x06, // 0b00000110 - 0x1
	0xCB, // 0b11001011 - 0x2
	0x8F, // 0b10001111 - 0x3
	0x2E, // 0b00101110 - 0x4
	0xAD, // 0b10101101 - 0x5
	0xED, // 0b11101101 - 0x6
	0x86, // 0b10000110 - 0x7
	0xEF, // 0b11101111 - 0x8
	0xAF, // 0b10101111 - 0x9
	0xEE, // 0b11101110 - 0xA
	0x6D, // 0b01101101 - 0xB
	0xE1, // 0b11100001 - 0xC
	0x4F, // 0b01001111 - 0xD
	0xE9, // 0b11101001 - 0xE
	0xE8  // 0b11101000 - 0xF
};



/************************ Protocol Local Helpers *************************/

/*
 * _set_LEDs
 *   DESCRIPTION: sets the LEDs given its mask and LED data
 *   INPUTS: LED_mask -- byte whose lower 4 bits correspond to which LEDs to set
 * 			 LED_data -- 4 char array storing LED data to set to
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: saves the LEDs into the tux info struct and sets the setting LEDs flag
 */
static void _set_LEDs(char LED_mask, char LED_data[4]) {
	char toSend_buf[6]; // buffer holding bytes to send. setting LEDs uses up to 6 bytes
	int i;				// loop index
	int byte_cnt = 6;	// number of bytes to send, starting at 2 (command + mask), also used to index buffer

	// led set command byte
	toSend_buf[0] = MTCP_LED_SET;
	// led mask byte
	toSend_buf[1] = MASK_LOW_4;

	// led data bytes
	for (i = 0; i < 4; i++) {
		if ((LED_mask >> i) & 1) { // check if LED is being set or not
			toSend_buf[i+2] = LED_data[i];
			// byte_cnt++; // increase number of bytes to send
			tux_info.last_LEDs[i] = LED_data[i]; // save the new LED data
		} else {
			toSend_buf[i+2] = LED_data[i] & (1 << LED_DP_BIT); // turn LEDs off
			tux_info.last_LEDs[i] = LED_data[i] & (1 << LED_DP_BIT);
		}
	}

	// send data
	if(tuxctl_ldisc_put(tux_info.tty, toSend_buf, byte_cnt)) {
		// printk("error while sending MTCP_LED_SET to tux\n");
		return;
	}

	// set setting LEDs flag
	tux_info.setting_LEDs = true;
}

/*
 * _set_button_interrupt_gen
 *   DESCRIPTION: sets the button interrupt generation on the tux
 *   INPUTS: button_interrupts -- boolean to set the tux to
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes the button interrupt generation on the tux and tux info struct
 */
static void _set_button_interrupt_gen(bool button_interrupts) {
	char toSend_buf[1]; // buffer holding bytes to send

	// button interrupt generation command byte
	toSend_buf[0] = (button_interrupts) ? MTCP_BIOC_ON : MTCP_BIOC_OFF;

	// send data
	if(tuxctl_ldisc_put(tux_info.tty, toSend_buf, 1)) {
		// printk("error while sending button interrupt generation to tux\n");
		return;
	}

	// update tux info struct
	tux_info.button_interrupt_gen = button_interrupts;
}

/*
 * _set_LED_to_user_mode
 *   DESCRIPTION: sets the LEDs to user mode
 *   INPUTS: none
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes the LEDs to user mode on the tux and tux info struct
 */
static void _set_LED_to_user_mode(void) {
	char toSend_buf[1]; // buffer holding bytes to send

	// set LED to user mode command byte
	toSend_buf[0] = MTCP_LED_USR;
	
	// send data
	if(tuxctl_ldisc_put(tux_info.tty, toSend_buf, 1)) {
		// printk("error while sending MTCP_LED_USR to tux\n");
		return;
	}

	// update tux info struct
	tux_info.LED_user_mode = true;
}

/************************ Protocol Implementation *************************/

/* tuxctl_handle_packet()
 * IMPORTANT : Read the header for tuxctl_ldisc_data_callback() in 
 * tuxctl-ld.c. It calls this function, so all warnings there apply 
 * here as well.
 */
void tuxctl_handle_packet (struct tty_struct* tty, unsigned char* packet)
{
    unsigned char byte0, byte1, byte2;	// packets for byte 1, 2, 3

    byte0 = packet[0]; /* Avoid printk() sign extending the 8-bit */
    byte1 = packet[1]; /* values when printing them. */
    byte2 = packet[2];

    // printk("packet : %x %x %x\n", byte0, byte1, byte2);
	switch (byte0) {
		case MTCP_ACK:
			// turn off tux setting LEDs flag
			if (tux_info.setting_LEDs) {
				tux_info.setting_LEDs = false;
			}
			return;
		case MTCP_BIOC_EVENT:
			/* 
			 * Response packet (active low):
			 * byte 0 - MTCP_BIOC_EVENT
			 * byte 1
			 * __7_____4___3___2___1_____0____
			 * | 1 X X X | C | B | A | START |
			 * -------------------------------
			 * byte 2  
			 * __7_____4_____3______2______1_____0___
			 * | 1 X X X | right | down | left | up |
			 * --------------------------------------
			 * 
			 * button buffer format (active high):
			 * ____7_______6______5____4____3___2___1_____0____
			 * | right | left | down | up | C | B | A | START |
			 * ------------------------------------------------
			 */
			tux_info.button_buf = ~(((byte2 & 0x09) << 4) | ((byte2 & 0x02) << 5) | ((byte2 & 0x04) << 3) | (byte1 & MASK_LOW_4)); // 0x09 masks bits 0 and 3, flip left and down bits

			return;
		case MTCP_RESET:
			// set button interrupt generation
			_set_button_interrupt_gen(tux_info.button_interrupt_gen);

			// set LED mode
			if (tux_info.LED_user_mode) {
				_set_LED_to_user_mode();
			}

			// set the old LEDs
			_set_LEDs(MASK_LOW_4, tux_info.last_LEDs); // set all 4 LEDs

			return;
		case MTCP_ERROR:
			// printk("tux sends error code!\n");
			return;
		default:
			return;
	}
}

/******** IMPORTANT NOTE: READ THIS BEFORE IMPLEMENTING THE IOCTLS ************
 *                                                                            *
 * The ioctls should not spend any time waiting for responses to the commands *
 * they send to the controller. The data is sent over the serial line at      *
 * 9600 BAUD. At this rate, a byte takes approximately 1 millisecond to       *
 * transmit; this means that there will be about 9 milliseconds between       *
 * the time you request that the low-level serial driver send the             *
 * 6-byte SET_LEDS packet and the time the 3-byte ACK packet finishes         *
 * arriving. This is far too long a time for a system call to take. The       *
 * ioctls should return immediately with success if their parameters are      *
 * valid.                                                                     *
 *                                                                            *
 ******************************************************************************/
int 
tuxctl_ioctl (struct tty_struct* tty, struct file* file, 
	      unsigned cmd, unsigned long arg)
{	
	int i;					// loop index
	char LED_data[4];		// LED data to send for set LEDs ioctl
	int dp_bit;				// dp bit for set LEDs ioctl

	// printk("tux_info =\n\t{button_interrupt_gen=0x%x,\n\t last_LEDs={0x%x, 0x%x, 0x%x, 0x%x},\n\t LED_user_mode=0x%x\n\t setting_LEDs=0x%x}\n", tux_info.button_interrupt_gen, tux_info.last_LEDs[0], tux_info.last_LEDs[1], tux_info.last_LEDs[2], tux_info.last_LEDs[3], tux_info.LED_user_mode, tux_info.setting_LEDs);
    switch (cmd) {
		case TUX_INIT:
			// initialize tux info struct
			tux_info.tty = tty;
			tux_info.button_interrupt_gen = false;
			for (i = 0; i < 4; i++) {
				tux_info.last_LEDs[i] = 0x00;
			}
			tux_info.LED_user_mode = false;
			tux_info.setting_LEDs = false;

			// turn on button interrupts
			_set_button_interrupt_gen(true);

			// set LEDs to user mode
			_set_LED_to_user_mode();

			// clear the LEDs
			_set_LEDs(MASK_LOW_4, tux_info.last_LEDs);

			return 0;
		case TUX_BUTTONS:
			// check if pointer is nullptr
			if (arg == (unsigned long) NULL) {
				// printk("invalid input pointer!");
				return -EINVAL;
			}
			// copy button data to user pointer
			if(copy_to_user((unsigned char*) arg, &(tux_info.button_buf), 1)) {
				// printk("copy to user error!\n");
				return -EINVAL;
			}

			return 0;
		case TUX_SET_LED:
			// cannot set LEDs if tux is already setting LEDs. prevents spam calls to ioctl set LED
			if (tux_info.setting_LEDs) {
				// printk("setting LEDs too early!");
				return -EINVAL;
			}
			
			// convert arg to hex LED data bytes
			for (i = 0; i < 4; i++) {
				dp_bit = (arg >> (LED_DP_OFFSET + i)) & 1; // get dp bit
				LED_data[i] = hex_to_led[(unsigned int)(arg >> 4*i) & MASK_LOW_4] | (dp_bit << LED_DP_BIT); // get hex data according to hex_to_led and set dp bit according to dp_bit
			}

			// set the LEDs
			_set_LEDs((arg >> LED_MASK_OFFSET) & MASK_LOW_4, LED_data);
			
			return 0;
		default:
			return -EINVAL;
    }
}


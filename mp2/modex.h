/* 
 * tab:4
 *
 * modex.h - header file for mode X 320x200 graphics
 *
 * "Copyright (c) 2004-2009 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:        Steve Lumetta
 * Version:       2
 * Creation Date: Thu Sep  9 23:08:21 2004
 * Filename:      modex.h
 * History:
 *    SL    1    Thu Sep  9 23:08:21 2004
 *        First written.
 *    SL    2    Sat Sep 12 13:35:41 2009
 *        Integrated original release back into main code base.
 */

/* 
 * ------------------------
 * Jacob Zheng - 02/25/2024
 * ------------------------
 */

#ifndef MODEX_H
#define MODEX_H

#include "text.h"

// seconds per minute
#define SEC_IN_MIN  60

/* structure used to hold status bar information */
typedef struct {
    int level_num; 
    int remaining_fruit;
    int time_sec;
    unsigned char bg_palette_idx;
    unsigned char text_palette_idx;
} status_bar_info_t;

status_bar_info_t status_bar_info;

/* 
 * IMAGE  is the whole screen in mode X: 320x200 pixels in our flavor.
 * SCROLL is the scrolling region of the screen.
 *
 * X_DIM   is a horizontal screen dimension in pixels.
 * X_WIDTH is a horizontal screen dimension in 'natural' units
 *         (addresses, characters of text, etc.)
 * Y_DIM   is a vertical screen dimension in pixels.
 */
#define IMAGE_X_DIM     320   /* pixels; must be divisible by 4             */
#define IMAGE_Y_DIM     200   /* pixels                                     */
#define STATUS_BAR_Y_DIM 18   // pixels
#define IMAGE_X_WIDTH   (IMAGE_X_DIM / 4)          /* addresses (bytes)     */
#define SCROLL_X_DIM    IMAGE_X_DIM                /* full image width      */
#define SCROLL_Y_DIM    (IMAGE_Y_DIM - STATUS_BAR_Y_DIM) // scrolling image width
#define SCROLL_X_WIDTH  (IMAGE_X_DIM / 4)          /* addresses (bytes)     */

/* VGA colors */
#define VGA_WHITE           {0x3F, 0x3F, 0x3F}
#define VGA_GREY            {0x2D, 0x2D, 0x2D}
#define VGA_BROWN           {0x29, 0x0B, 0x0B}
#define VGA_BLACK           {0x00, 0x00, 0x00}
#define VGA_RED             {0x3F, 0x00, 0x00}
#define VGA_ORANGE          {0x3F, 0x07, 0x00}
#define VGA_YELLOW          {0x3F, 0x3F, 0x00}
#define VGA_LIGHT_GREEN     {0x07, 0x3F, 0x00}
#define VGA_GREEN           {0x00, 0x3F, 0x00}
#define VGA_TURQUOISE       {0x00, 0x3F, 0x07}
#define VGA_CYAN            {0x00, 0x3F, 0x3F}
#define VGA_LIGHT_BLUE      {0x00, 0x07, 0x3F}
#define VGA_BLUE            {0x00, 0x00, 0x3F}
#define VGA_VIOLET          {0x07, 0x00, 0x3F}
#define VGA_MAGENTA         {0x3F, 0x00, 0x3F}
#define VGA_PINK            {0x3F, 0x00, 0x07}

/* useful palette indices */
#define PLAYER_PALETTE_IDX      0x20
#define WALL_PALETTE_IDX        0x22
#define STATUS_BAR_PALETTE_IDX  0x23

/* text buffer is stored in row-major order */
#define TEXT_BUF_SIZE           ((TEXT_WIDTH+2) * BUFFER_HEIGHT)         // size of text buffer in bytes

/*
 * NOTES
 *
 * Mode X is an (originally) undocumented variant of mode 13h, the first
 * IBM 256-color graphics mode.  Each pixel uses a single byte to specify
 * one of the 256 possible colors, and a palette is used to map each color
 * into an 18-bit space (6-bit red, green, and blue intensities).
 *
 * The map from memory as seen by the host processor is non-linear, and was
 * originally designed to allow high-performance hardware designs.  Video
 * memory is in words of 32 bits, and is divided into four planes.  In mode
 * X, groups of four pixels are mapped into a single host address and written
 * individually or together by setting a bit mask of planes to be written
 * (a VGA register).
 *
 * each four pixels counts as one (one-byte) address ->
 * 0123012301230123012301230123012301230123012301230123012301230123
 *
 * The mapping is more contorted than with mode 13h, but allows use of
 * graphics tricks such as double-buffering.
 *
 * The design here is strongly influenced by the fact that we are running
 * in a virtual machine in which writes to video memory are exorbitantly
 * expensive.  In particular, writing a chunk of 16kB with a single x86
 * instruction (REP MOVSB) is much faster than writing two hundred bytes
 * with many instructions.
 *
 * That said, the design is not unreasonable, and is only slightly different
 * than was (and is) often used today.
 *
 * Double-buffering uses two sections of memory to allow a program to
 * draw the next screen to be displayed without having the partially drawn
 * screen visible on the monitor (which causes flicker).  When the drawing
 * is complete, the video adapter is told to display the new screen instead
 * of the old one, and the memory used for the old screen is then used to
 * draw a third screen, the video adapter is switched back, and the process
 * starts again.
 *
 * In our variant of double-buffering, we use non-video memory as the
 * scratch pad, copy the drawn screen as a whole into one of two buffers
 * in video memory, and switch the picture between the two buffers.  The
 * cost of the copy is negligible; the cost of writing to video memory
 * instead is quite high (under QEMU).
 *
 * In order to reduce drawing time, we reuse most of the screen data between
 * video frames.  New data are drawn only when the viewing window moves
 * within a logical space defined by the program.  For example, if this
 * window shifts one pixel to the left, only the left border of the screen
 * is drawn.  Other data are left untouched in most cases.
 */

/* configure VGA for mode X; initializes logical view to (0,0) */
extern int set_mode_X(
    void (*horiz_fill_fn)(int, int, unsigned char[SCROLL_X_DIM]),
    void (*vert_fill_fn)(int, int, unsigned char[SCROLL_Y_DIM]));

/* return to text mode */
extern void clear_mode_X();

/* set logical view window coordinates */
extern void set_view_window(int scr_x, int scr_y);

/* show the logical view window on the monitor */
extern void show_screen();

/* show the status bar on the monitor */
extern void show_status();

/* clear the video memory in mode X */
extern void clear_screens();

/*
 * draw a 12x12 block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not drawn)
 */
extern void draw_full_block(int pos_x, int pos_y, unsigned char* blk);

/*
 * saves a 12x12 block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not saved)
 */
extern void save_player_block(int pos_x, int pos_y);

/*
 * draw a 12x12 block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not drawn); draws player given mask; otherwise draws the background
 */
extern void draw_player(int pos_x, int pos_y, unsigned char* blk, unsigned char* mask);

/*
 * erases from a 12x12 block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not touched); erases the player given its mask
 */
extern void erase_player(int pos_x, int pos_y, unsigned char* mask);

/*
 * saves the text block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not saved)
 */
extern void save_text_block(int pos_x, int pos_y);

/*
 * draw the text block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not drawn); draws transparent colors given mask; otherwise draws the background
 */
extern void draw_text(int pos_x, int pos_y, unsigned char* mask);

/*
 * erases from a text block with upper left corner at logical position
 * (pos_x,pos_y); any part of the block outside of the logical view window
 * is clipped (cut off and not touched); erases the text given its mask
 */
extern void erase_text(int pos_x, int pos_y, unsigned char* mask);

/* draw a horizontal line at vertical pixel y within the logical view window */
extern int draw_horiz_line(int y);

/* draw a vertical line at horizontal pixel x within the logical view window */
extern int draw_vert_line(int x);

/* sets a single palette to the given color and at the given palette index */
extern void set_palette(unsigned char color[3], unsigned char idx);



#endif /* MODEX_H */

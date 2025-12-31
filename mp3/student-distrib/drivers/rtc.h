#ifndef _RTC_H
#define _RTC_H

#include "../lib.h"
#include "i8259.h" //disable and enable can be called 
#include "fsys.h"

/* RTC information */
#define RTC_IRQ     8
#define RTC_PORT    0x70

/* global rtc counter */
uint32_t rtc_counter;

/* initializes RTC */
void init_RTC(void);

/* RTC interrupt handler */
void RTC_Handler(void);

/* RTC read syscall */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

/* RTC write syscall */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);

/* RTC open syscall */
int32_t rtc_open(const uint8_t* filename);

/* RTC close syscall */
int32_t rtc_close(int32_t fd);

#endif /* _RTC_H */

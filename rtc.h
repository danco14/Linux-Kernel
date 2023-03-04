/* rtc.h - Defines and starts the rtc and handler */

#ifndef _RTC_H
#define _RTC_H

#include "types.h"

/* Ports and registers used to initialize the RTC. */
#define RTC_PORT0 0x70
#define RTC_PORT1 0x71
#define REGISTER_A 0x8A
#define REGISTER_B 0x8B
#define REGISTER_C 0x8C
#define SLAVE_PIN 2
#define RTC_IRQ_NUM 8
#define FREQ_1024 0x06

#ifndef ASM

/* Flag to allow prints for test cases */
extern uint32_t rtc_test_flag;
extern uint32_t rtc_read_test_flag;

/* Initialize RTC */
void rtc_init(void);

/* Handler for RTC interrupts */
void rtc_interrupt_handler(void);

/* RTC device driver open */
int32_t rtc_open(const uint8_t* filename);

/* RTC device driver close */
int32_t rtc_close(int32_t fd);

/* RTC device driver read */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);

/* Set the frequency of the RTC */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);


#endif /* ASM */

#endif /* _RTC_H */

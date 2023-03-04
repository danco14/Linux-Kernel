#ifndef _LINKAGE_H
#define _LINKAGE_H

#include "types.h"

/* Virtual address of the user stack */
#define USER_ESP 0x83FFFFF

#ifndef ASM

/* Linkage and jump table for system calls */
extern int32_t system_call_handler();

/* Save registers for keyboard handler */
extern void keyboard_linkage();

/* Save registers for rtc handler */
extern void rtc_linkage();

/* Save registers for pit handler */
extern void pit_linkage();

#endif /* ASM */

#endif /* _LINKAGE_H */

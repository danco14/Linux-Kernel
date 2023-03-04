/* idt_init.h - Initializes the interrupt descriptor table */

#ifndef _IDT_INIT_H
#define _IDT_INIT_H

#include "x86_desc.h"

/* Initilize the idt in the boot.S */
void initialize_idt(void);

#endif

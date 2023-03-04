/* idt_init.c - Initializes the interrupt descriptor table */

#include "idt_init.h"
#include "x86_desc.h"
#include "rtc.h"
#include "lib.h"
#include "pit.h"
#include "i8259.h"
#include "kb.h"
#include "linkage.h"
#include "syscalls.h"
#include "paging.h"

#define NUM_EXCEPTION 32
#define SYS_CALL_INDEX 0x80

#define EIGHT_MB 0x800000
#define FOUR_MB 0x400000
#define PROG_OFFSET 0x00048000
#define RUNNING 0
#define STOPPED 1

/*
 * irq1_handler
 *    DESCRIPTION: Handles IRQ1 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ1
 */
void irq1_handler(void){
  send_eoi(1);
  printf("keyboard interrupt\n");
}

/*
 * irq2_handler
 *    DESCRIPTION: Handles IRQ2 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ2
 */
void irq2_handler(void){
  send_eoi(2);
  printf("slave interrupt\n");
}

/*
 * irq3_handler
 *    DESCRIPTION: Handles IRQ3 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ3
 */
void irq3_handler(void){
  send_eoi(3);
  printf("irq3 interrupt\n");
}

/*
 * irq4_handler
 *    DESCRIPTION: Handles IRQ4 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ4
 */
void irq4_handler(void){
  send_eoi(4);
  printf("serial port interrupt\n");
}

/*
 * irq5_handler
 *    DESCRIPTION: Handles IRQ5 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ5
 */
void irq5_handler(void){
  send_eoi(5);
  printf("irq_5\n");
}

/*
 * irq6_handler
 *    DESCRIPTION: Handles IRQ6 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ6
 */
void irq6_handler(void){
  send_eoi(6);
  printf("irq_6\n");
}

/*
 * irq7_handler
 *    DESCRIPTION: Handles IRQ7 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ7
 */
void irq7_handler(void){
  send_eoi(7);
  printf("irq_7");
}

/*
 * irq9_handler
 *    DESCRIPTION: Handles IRQ9 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ9
 */
void irq9_handler(void){
  send_eoi(9);
  printf("irq_9\n");
}

/*
 * irq10_handler
 *    DESCRIPTION: Handles IRQ10 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ10
 */
void irq10_handler(void){
  send_eoi(10);
  printf("irq_10\n");
}

/*
 * irq11_handler
 *    DESCRIPTION: Handles IRQ11 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ11
 */
void irq11_handler(void){
  send_eoi(11);
  printf("irq_11, eth0(network)\n");
}

/*
 * irq12_handler
 *    DESCRIPTION: Handles IRQ12 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ12
 */
void irq12_handler(void){
  send_eoi(12);
  printf("irq_12, PS/2 mouse interrupt\n");
}

/*
 * irq13_handler
 *    DESCRIPTION: Handles IRQ13 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ13
 */
void irq13_handler(void){
  send_eoi(13);
  printf("irq_13\n");
}

/*
 * irq14_handler
 *    DESCRIPTION: Handles IRQ14 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ014
 */
void irq14_handler(void){
  send_eoi(14);
  printf("irq_14, ide0(hard drive interrupt)\n");
}

/*
 * irq15_handler
 *    DESCRIPTION: Handles IRQ15 interrupts
 *    OUTPUTS: Prints IRQ number to screen, and sends EOI for IRQ15
 */
void irq15_handler(void){
  send_eoi(15);
  printf("irq_15\n");
}

/*
 * sys_call_handler
 *    DESCRIPTION:
 *    INPUTS:
 *    OUTPUTS:
 *    RETURN VALUE:
 *    SIDE EFFECTS:
 */
void sys_call_handler(void){
	printf("System call handler here!\n");
}

/*
 * test_interrupt
 *    DESCRIPTION:
 *    INPUTS:
 *    OUTPUTS:
 *    RETURN VALUE:
 *    SIDE EFFECTS:
 */
void test_interrupt(void){
	printf("Steve Lumetta is an AI coded by Steven Lumetta\n");
}

/*
 * exception_func
 *    DESCRIPTION: A macro to handle exceptions
 *    INPUTS: x - The type of exception
 *            msg - Exception type to print to screen
 *    OUTPUTS: Prints the exception type to the screen
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Causes the program to enter an inifinite loop
 */
#define EXCEPTION_MAKER(x,msg) \
	void x(void){ \
    cli(); \
		printf("Exception: %s\n",msg); \
    pcb_t* cur_pcb = get_pcb_add(); /* Get the current process pcb */ \
    process_num--; \
    process_array[(cur_pcb->pid)-1]=-1; \
    sched_arr[cur_sched_term].process_num = cur_pcb->parent_pid; \
    sched_arr[cur_sched_term].esp = cur_pcb->parent_esp; \
    sched_arr[cur_sched_term].ebp = cur_pcb->parent_ebp; \
    set_page_dir_entry(USER_PROG, EIGHT_MB + (cur_pcb->parent_pid - 1)*FOUR_MB); \
    if(cur_pcb->vidmem)sched_arr[cur_sched_term].vid_map = 0; \
    int i; \
    for(i = 0; i <= MAX_FD_NUM; i++) close(i);\
    asm volatile ("      \n\
       movl %%cr3, %%eax \n\
       movl %%eax, %%cr3" \
       :  \
       :  \
       : "eax"  \
    );  \
      tss.esp0 = cur_pcb->parent_esp; /* Set TSS esp0 back to parent stack pointer */ \
      asm volatile ("      \n\
         movl $256,%%eax \n\
         movl %0,%%ebp     \n\
         sti              \n\
         leave             \n\
         ret" \
         :  \
         : "r"(cur_pcb->parent_ebp)\
         : "ebp","eax"  \
      );  \
     }

/* List of excetions */
EXCEPTION_MAKER(DIVIDE_ERROR, "Divide Error");
EXCEPTION_MAKER(RESERVED, "RESERVED");
EXCEPTION_MAKER(NMI, "NMI");
EXCEPTION_MAKER(BREAKPOINT, "Breakpoint");
EXCEPTION_MAKER(OVERFLOW, "Overflow");
EXCEPTION_MAKER(BOUND_RANGE_EXCEEDED, "BOUND Range Exceeded");
EXCEPTION_MAKER(INVALID_OPCODE, "Invalid OPCODE");
EXCEPTION_MAKER(DEVICE_NOT, "Device Not Available");
EXCEPTION_MAKER(DOUBLE_FAULT, "Double Fault");
EXCEPTION_MAKER(SEGMENT_OVERRUN, "Coprocessor Segment Overrun");
EXCEPTION_MAKER(INVALID_TSS, "Invalid TSS");
EXCEPTION_MAKER(SEGMENT_NOT_PRESENT, "Segment Not Present");
EXCEPTION_MAKER(STACK_SEGMENT_FAULT, "Stack Segment Fault") ;
EXCEPTION_MAKER(GENERAL_PROTECTION, "General Protection");
EXCEPTION_MAKER(PAGE_FAULT, "Page Fault");
EXCEPTION_MAKER(MATH_FAULT, "x87 FPU Floating-Point Error(Math Fault)");
EXCEPTION_MAKER(ALIGNMENT_CHECK, "Alignment Check");
EXCEPTION_MAKER(MACHINE_CHECK, "Machine Check");
EXCEPTION_MAKER(SIMD_FLOATING_POINT_EXCEPTION, "SIMD Floating-point exception");

/*
 * exception_func
 *    DESCRIPTION: The default exception handler
 *    INPUTS: none
 *    OUTPUTS: Prints that an exception occurred
 *    RETURN VALUE: none
 *    SIDE EFFECTS: none
 */
void exception_func(void){
    printf("This is exception\n");
}

/*
 * initialize_idt
 *    DESCRIPTION: Initializes the entries in the IDT
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Sets the entries in the IDT
 */
void initialize_idt(void){
  	int init_counter; /* Loop variable */

    /* Go through each exception */
  	for(init_counter = 0; init_counter < NUM_EXCEPTION; init_counter++){
  		idt[init_counter].present = 1; // sets present bit to 1 to show it is not empty
  		idt[init_counter].dpl = 0; //set dpl to 0 to indicate it has high priority
  		idt[init_counter].reserved0 = 0; // this bits are to indicate the field 0S1110000000 where S is the size bit
  		idt[init_counter].size = 1; //Sets the gate size to 32 bits
  		idt[init_counter].reserved1 = 1;
  		idt[init_counter].reserved2 = 1;
  		idt[init_counter].reserved3 = 1;
  		idt[init_counter].reserved4 = 0;
  		idt[init_counter].seg_selector = KERNEL_CS; //Sets the segment selector to the Kernel's code segment
      /* Sets the default exception handler */
  		SET_IDT_ENTRY(idt[init_counter], exception_func);
  	}

    /* Go through each interrupt */
  	for(init_counter = NUM_EXCEPTION; init_counter < NUM_VEC; init_counter++){
      /* Skip 0x80 for the system call entry */
  		if(init_counter == SYS_CALL_INDEX){
  		  continue;
  		}

  		idt[init_counter].present = 1;      /* Sets the prsent bit to 1 to show it is not empty */
  		idt[init_counter].dpl = 0;          /* set dpl to 0 to indicate it has a high priority */
  		idt[init_counter].reserved0 = 0;    /* Sets reserved this field is meant to indicate 0s110000000 where S is the size bit */
  		idt[init_counter].size = 1;         /* Sets the gate size ot 32 bits */
  		idt[init_counter].reserved1 = 1;
  		idt[init_counter].reserved2 = 1;
  		idt[init_counter].reserved3 = 0;
  		idt[init_counter].reserved4 = 0;
  		idt[init_counter].seg_selector = KERNEL_CS; /* Sets the segment selector to Kernel's code segment */
      /* Set the default interrupt handler to test_interrupt */
  		SET_IDT_ENTRY(idt[init_counter], test_interrupt);
  	}

    /* Initialize system call entry in IDT */
  	idt[SYS_CALL_INDEX].present = 1;      /* Sets to present to indicate that is it present */
  	idt[SYS_CALL_INDEX].dpl = 3;          /* Set dpl to 3 for a low priority level */
  	idt[SYS_CALL_INDEX].reserved0 = 0;    /* These bits are set to idicate the field 0s111000000 where S is the size bit */
  	idt[SYS_CALL_INDEX].size = 1;         /* Sets the gate size to 32 bits */
  	idt[SYS_CALL_INDEX].reserved1 = 1;
  	idt[SYS_CALL_INDEX].reserved2 = 1;
  	idt[SYS_CALL_INDEX].reserved3 = 0;
  	idt[SYS_CALL_INDEX].reserved4 = 0;
  	idt[SYS_CALL_INDEX].seg_selector=KERNEL_CS;  /* Sets the segment selector to the Kernel's code segment */
    /* Set system call IDT entry to sys_call_handler */
  	SET_IDT_ENTRY(idt[SYS_CALL_INDEX], system_call_handler);

    /* Set exception entries in the IDT */
    SET_IDT_ENTRY(idt[0], DIVIDE_ERROR);
  	SET_IDT_ENTRY(idt[1], RESERVED);
    SET_IDT_ENTRY(idt[2], NMI);
    SET_IDT_ENTRY(idt[3], BREAKPOINT);
    SET_IDT_ENTRY(idt[4], OVERFLOW);
    SET_IDT_ENTRY(idt[5], BOUND_RANGE_EXCEEDED);
    SET_IDT_ENTRY(idt[6], INVALID_OPCODE);
    SET_IDT_ENTRY(idt[7], DEVICE_NOT);
    SET_IDT_ENTRY(idt[8], DOUBLE_FAULT);
    SET_IDT_ENTRY(idt[9], SEGMENT_OVERRUN);
    SET_IDT_ENTRY(idt[10], INVALID_TSS);
    SET_IDT_ENTRY(idt[11], SEGMENT_NOT_PRESENT);
    SET_IDT_ENTRY(idt[12], STACK_SEGMENT_FAULT);
    SET_IDT_ENTRY(idt[13], GENERAL_PROTECTION);
    SET_IDT_ENTRY(idt[14], PAGE_FAULT);
    SET_IDT_ENTRY(idt[16], MATH_FAULT);
    SET_IDT_ENTRY(idt[17], ALIGNMENT_CHECK);
    SET_IDT_ENTRY(idt[18], MACHINE_CHECK);
    SET_IDT_ENTRY(idt[19], SIMD_FLOATING_POINT_EXCEPTION);

    /* Set IRQ entries in the IDT */
    SET_IDT_ENTRY(idt[0x28], rtc_linkage);
    SET_IDT_ENTRY(idt[0x21], keyboard_linkage);
  	SET_IDT_ENTRY(idt[0x20], pit_linkage);
    SET_IDT_ENTRY(idt[0x22], irq2_handler);
    SET_IDT_ENTRY(idt[0x23], irq3_handler);
  	SET_IDT_ENTRY(idt[0x24], irq4_handler);
	  SET_IDT_ENTRY(idt[0x25], irq5_handler);
	  SET_IDT_ENTRY(idt[0x26], irq6_handler);
    SET_IDT_ENTRY(idt[0x27], irq7_handler);
    SET_IDT_ENTRY(idt[0x29], irq9_handler);
    SET_IDT_ENTRY(idt[0x2A], irq10_handler);
    SET_IDT_ENTRY(idt[0x2B], irq11_handler);
    SET_IDT_ENTRY(idt[0x2C], irq12_handler);
  	SET_IDT_ENTRY(idt[0x2D], irq13_handler);
  	SET_IDT_ENTRY(idt[0x2E], irq14_handler);
  	SET_IDT_ENTRY(idt[0x2F], irq15_handler);
}

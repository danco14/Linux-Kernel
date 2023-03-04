#include "lib.h"
#include "pit.h"
#include "i8259.h"
#include "syscalls.h"
#include "paging.h"
#include "x86_desc.h"

#define OSCILLATOR_FREQ 1193182      /* PIT oscillator runs at approximately 1.193182 MHz */
#define INTERRUPT_INTERVAL 100    /* we want PIT interrupts every 100Hz = 10ms */

int32_t prev_sched_term = -1; /* the scheduled process right before scheduling switch*/
int32_t cur_sched_term = 0;	/* the next scheduled process */
sched_node sched_arr[SCHED_SIZE];	/* scheduling data strucute */

/*
 * pit_init
 *    DESCRIPTION: Initialize the PIT and set up the scheduling structure
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: enables PIT interrupts at desired frequency and set up scheduling data structure
 */
void pit_init(void){
  uint32_t divisor = OSCILLATOR_FREQ / INTERRUPT_INTERVAL; /* frequency divisor of PIT */
  uint32_t divisor_low = divisor & 0xFF; /* low byte of divisor */
  uint32_t divisor_high = (divisor >> 8) & 0xFF; /* high byte of divisor */

	sched_arr[0].process_num = 1;
  sched_arr[0].video_buffer = FIRST_SHELL;
	sched_arr[0].vid_map = 0;
  sched_arr[0].esp = EIGHT_MB;
  sched_arr[0].ebp = EIGHT_MB;

  sched_arr[1].process_num = 2;
  sched_arr[1].video_buffer = SECOND_SHELL;
	sched_arr[1].vid_map = 0;
  sched_arr[1].esp = -1;
  sched_arr[1].ebp = -1;

  sched_arr[2].process_num = 3;
  sched_arr[2].video_buffer = THIRD_SHELL;
	sched_arr[2].vid_map = 0;
  sched_arr[2].esp = -1;
  sched_arr[2].ebp = -1;

  outb(0x36, PIT_COMMAND_PORT); /* 0x36 = command to set PIT to repeating mode */
  outb(divisor_low, PIT_CHANNEL0); /* write low and high byte of divisor to channel 0 */
  outb(divisor_high, PIT_CHANNEL0);

  /* Enable PIT interrupts on PIC */
  enable_irq(PIT_IRQ_NUM);
}

/*
 * switch_process
 *    DESCRIPTION: Switch from currently scheduled process to next scheduled process
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: restores info of next process and saves current process ebp,esp
 *									remaps video memory paging
 *									starts executing the next scheduled process
 *									remaps vidmap system call's paging if needed
 *									changes the terminal that terminal_write prints to
 */
void switch_process(){
	/* Set TSS to next process */
  tss.esp0 = EIGHT_MB - (sched_arr[cur_sched_term].process_num - 1)*EIGHT_KB;

	/* Remap user page */
  set_page_dir_entry(USER_PROG, EIGHT_MB + (sched_arr[cur_sched_term].process_num - 1)*FOUR_MB);

	/* Remap video memory paging along with user video memory if process uses vidmap */
  if(cur_sched_term == cur_terminal){
		if(sched_arr[cur_sched_term].vid_map == 1){
			/* scheduled process is same as currently viewing terminal, set user video mem to map to physical video mem */
			set_page_table2_entry(USER_VIDEO_MEM, VIDEO_MEM_ADDR);
		}
		/* scheduled process is same as currently viewing terminal, set video mem to map to physical video mem */
    set_page_table1_entry(VIDEO_MEM_ADDR, VIDEO_MEM_ADDR);
  } else{
		if(sched_arr[cur_sched_term].vid_map == 1){
			/* set user video mem to map to scheduled process' video buffer */
			set_page_table2_entry(USER_VIDEO_MEM, sched_arr[cur_sched_term].video_buffer);
		}
		/* set video mem to map to scheduled process' video buffer */
    set_page_table1_entry(VIDEO_MEM_ADDR, sched_arr[cur_sched_term].video_buffer);
  }

	/* Flush TLB */
	asm volatile ("      \n\
     movl %%cr3, %%eax \n\
     movl %%eax, %%cr3"
     :
     :
     : "eax"
	);

	/* set terminal write to print to the terminal that is being scheduled */
	print_terminal = cur_sched_term;

	/* Save esp and ebp */
	asm volatile("    \n\
     movl %%esp, %0 \n\
     movl %%ebp, %1"
     : "=r"(sched_arr[prev_sched_term].esp), "=r"(sched_arr[prev_sched_term].ebp)
  );

	/* Check if terminal has been accessed before */
	if(sched_arr[cur_sched_term].esp == -1){
		/* Unmask PIC interrupts*/
	  enable_irq(PIT_IRQ_NUM);

	  asm volatile("       \n\
	    movl %0, %%ecx     \n\
	    jmp context_switch"
	    :
	    : "r"(program_addr_test)
	    : "ecx"
	  );
	}

	/* Restore esp and ebp */
  asm volatile("    \n\
     movl %0, %%esp \n\
     movl %1, %%ebp"
     :
     : "r"(sched_arr[cur_sched_term].esp), "r"(sched_arr[cur_sched_term].ebp)
  );
}

/*
 * pit_interrupt_handler
 *    DESCRIPTION: Interrupt handler for the PIT, will call a process switch to switch to next scheduled process
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: switch to next scheduled process
 */
void pit_interrupt_handler(void){
  unsigned long flags; /* Hold the current flags */

  /* Mask interrupt flags */
  cli_and_save(flags);

  /* Turn off PIC interrupts */
  disable_irq(PIT_IRQ_NUM);

  /* Send EOI signal */
  send_eoi(PIT_IRQ_NUM);

  /* Check if interrupts are allowed */
	if(prev_sched_term == -1){
		/* Unmask PIC interrupts*/
	  enable_irq(PIT_IRQ_NUM);

	  /* Re-enable interrupts and restores flags */
	  restore_flags(flags);

		return;
	}

	/* move to next scheduled process */
	prev_sched_term = cur_sched_term;

  /* Increment schedule number */
  cur_sched_term = (cur_sched_term + 1) % SCHED_SIZE;

  /* Change process */
  switch_process();

  /* Unmask PIC interrupts*/
  enable_irq(PIT_IRQ_NUM);

  /* Re-enable interrupts and restores flags */
  restore_flags(flags);
}

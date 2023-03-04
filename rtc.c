#include "lib.h"
#include "rtc.h"
#include "i8259.h"
#include "syscalls.h"
#include "pit.h"

#define MAX_FREQ 1024

/* Flag to allow prints for test cases */
uint32_t rtc_test_flag = 0;
uint32_t rtc_read_test_flag = 0;

/* Flags to show interrupts occured */
volatile uint32_t interrupt_flags[3] = {0, 0, 0};

/* Number of occured interrupts */
int32_t count[3] = {0, 0, 0};

/* Frequency requests of processes */
int32_t frequencies[3] = {-1, -1, -1};

/*
 * rtc_init
 *    DESCRIPTION: Initializes RTC
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Changes register B and register C, and enables RTC interrupts on PIC
 */
void rtc_init(void){
    /* Disable non-maskable interrupts and select register B */
    outb(REGISTER_B, RTC_PORT0);

    /* Store the current register B value */
    uint8_t prevB = inb(RTC_PORT1);

    /* Select register B again */
    outb(REGISTER_B, RTC_PORT0);

    /* 0x40 allows periodic RTC interrupts */
    outb(prevB | 0x40, RTC_PORT1);

    outb(REGISTER_A, RTC_PORT0);
    uint8_t prevA = inb(RTC_PORT1);
    outb(REGISTER_A, RTC_PORT0);

    /* Give the new rate to register A */

    outb((prevA & 0xF0) | FREQ_1024, RTC_PORT1);

    /* Enable RTC PIC interrupts */
    enable_irq(RTC_IRQ_NUM);
}

/*
 * rtc_interrupt_handler
 *    DESCRIPTION: Handler for rtc interrupts
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Throws away RTC input and sends EOI
 */
void rtc_interrupt_handler(void){
  unsigned long flags; /* Hold the current flags */
  /* Mask interrupt flags */
  cli_and_save(flags);

  /* Turn off PIC interrupts */
  disable_irq(RTC_IRQ_NUM);

  /* Send EOI signal */
  send_eoi(RTC_IRQ_NUM);

  int32_t i; /* Loop variable */
  /* Check if required frequencies have been reached */
  for(i = 0; i < 3; i++){
    if(frequencies[i] != -1){
      if(++count[i] >= frequencies[i]){
        /* Simulate interrupt */
        interrupt_flags[i] = 1;
        count[i] = 0;
      }
    }
  }

  //test_interrupts();
  if(rtc_test_flag){
    uint8_t c = 'f';
    putc(c);
  }

  /* Read register C */
  outb(REGISTER_C, RTC_PORT0);

  /* Clear contents of RTC to allow RTC interrupts again */
  inb(RTC_PORT1);

  // /* Unmask PIC interrupts*/
  enable_irq(RTC_IRQ_NUM);

  /* Re-enable interrupts and restores flags */
  restore_flags(flags);
}

/*
 * rtc_open
 *    DESCRIPTION: Initialize RTC frequency
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success
 *    SIDE EFFECTS: Makes RTC rate 2Hz
 */
int32_t rtc_open(const uint8_t* filename){
  unsigned long flags; /* Hold current flag values */

  /* Mask interrupts and save flags */
  cli_and_save(flags);

  /* Set frequency to 2 Hz */
  frequencies[cur_sched_term] = MAX_FREQ / 2;

  /* Restore the interrupt flags */
  restore_flags(flags);

  /* Return success */
  return 0;
}

/*
 * rtc_close
 *    DESCRIPTION: Closes the file
 *    INPUTS: int32_t fd - file to close
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success
 *    SIDE EFFECTS: none
 */
int32_t rtc_close(int32_t fd){
  frequencies[cur_sched_term] = -1;
  return 0;
}

/*
 * rtc_read
 *    DESCRIPTION: Waits for an RTC interrupt to occur
 *    INPUTS: int32_t fd - the file to read
 *            void* buf - a buffer, does nothing
 *            int32_t - number of bytes in the buffer
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success
 *    SIDE EFFECTS: none
 */
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes){
  cli();

  /* Set number of occured interrupts */
  //count[cur_sched_term] = 0;
  interrupt_flags[cur_sched_term] = 0;

  sti();

  /* Block until an RTC interrupt occurs */
  while(!interrupt_flags[cur_sched_term]) {

  }

  /* Return success */
  return 0;
}
/*
 * rtc_write
 *    DESCRIPTION: Change the RTC rate
 *    INPUTS: int32_t fd - file to write to
 *            const void* buf - frequency to set
 *            int32_t nbytes - size of the buffer
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, or -1 for failure
 *    SIDE EFFECTS: Changes the frequency the RTC operates at
 */
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes){
  unsigned long flags; /* Hold current flag values */
  int32_t freq; /* Frequency from the buffer */
  int32_t rate; /* Rate from frequency */

  /* Mask interrupts and save flags */
  cli_and_save(flags);

  /* Check for valid inputs */
  if(nbytes != 4 || (int32_t*)buf == NULL){
    /* Restore the interrupt flags */
    restore_flags(flags);
    /* Return failure */
    return -1;
  }

  /* Get the frequency */
  freq = *(int32_t*)buf;

  /* Check for valid frequency and set the rate */
  switch(freq){
    case 2:
      rate = MAX_FREQ / 2;
      break;
    case 4:
      rate = MAX_FREQ / 4;
      break;
    case 8:
      rate = MAX_FREQ / 8;
      break;
    case 16:
      rate = MAX_FREQ / 16;
      break;
    case 32:
      rate = MAX_FREQ / 32;
      break;
    case 64:
      rate = MAX_FREQ / 64;
      break;
    case 128:
      rate = MAX_FREQ / 128;
      break;
    case 512:
      rate = MAX_FREQ / 512;
      break;
    case 1024:
      rate = MAX_FREQ / 1024;
      break;
    default:
      restore_flags(flags);
      /* Return failure */
      return -1;
  }

  /* Set the process' rate */
  frequencies[cur_sched_term] = rate;

  /* Restore the interrupt flags */
  restore_flags(flags);

  /* Return success */
  return 0;
}

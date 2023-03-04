#ifndef _PIT_H
#define _PIT_H

#include "types.h"

#define PIT_IRQ_NUM 0
#define PIT_CHANNEL0 0x40
#define PIT_COMMAND_PORT 0x43
#define SCHED_SIZE 3

#ifndef ASM

/* State of scheduled process */
typedef struct sched{
	int32_t process_num;
  int32_t video_buffer;
	int32_t vid_map;
  int32_t esp;
  int32_t ebp;
} sched_node;

/* Terminal currently scheduled to execute */
extern int32_t cur_sched_term;

/* Previously scheduled terminal */
extern int32_t prev_sched_term;

/* State of scheduled process */
extern sched_node sched_arr[SCHED_SIZE];

/* initialize the pit */
void pit_init(void);

/* pit interrupt handler */
void pit_interrupt_handler(void);

#endif /* ASM */

#endif /* _PIT_H */

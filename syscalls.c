#include "syscalls.h"
#include "x86_desc.h"
#include "paging.h"
#include "lib.h"
#include "kb.h"
#include "linkage.h"
#include "pit.h"

#define PROG_OFFSET     0x00048000
#define RUNNING         0
#define STOPPED         1
#define MAX_PROG_SIZE   FOUR_MB - PROG_OFFSET

/* Function pointers for rtc */
jump_table rtc_table = {rtc_write, rtc_read, rtc_open, rtc_close};

/* Function pointers for file */
jump_table file_table = {file_write, file_read, file_open, file_close};

/* Function pointers for directory */
jump_table dir_table = {dir_write, dir_read, dir_open, dir_close};

/* Function pointers for stdin */
jump_table stdin_table = {invalid_write, terminal_read, terminal_open, terminal_close};

/* Function pointers for stdout */
jump_table stdout_table = {terminal_write, invalid_read, terminal_open, terminal_close};

/* Process number: 1st process has pid 1, 0 means no processes have been launched */
int32_t process_num = 0;


/*
 * halt
 *    DESCRIPTION: Halt system call, restores parent process paging and data and closes fds
 *    INPUTS: uint8_t status - return value from halt
 *    OUTPUTS: none
 *    RETURN VALUE: Returns to parent execute
 *    SIDE EFFECTS: halt current process, return to parent process(shell)
 */
int32_t halt(uint8_t status){
  /* If shell tries to halt, just launch shell again */
  pcb_t* cur_pcb = get_pcb_add();   /* Get the current process pcb */

  if(cur_pcb->pid == 1 || cur_pcb->pid == 2 || cur_pcb->pid == 3){
    asm volatile("       \n\
      movl %0, %%ecx     \n\
      jmp context_switch"
      :
      : "r" (program_addr_test)
      : "ecx"
    );
  }

  int i; /* Loop variable */

  /* Close all files in the pcb */
  for(i = 0; i <= MAX_FD_NUM; i++){
    close(i);
  }

  process_num--; /* Decrement process number */
  process_array[cur_pcb->pid - 1] = -1; /* Mark process slot as free */
  tss.esp0 = cur_pcb->parent_esp;   /* Set TSS esp0 back to parent stack pointer */

  sched_arr[cur_sched_term].process_num = cur_pcb->parent_pid;
  sched_arr[cur_sched_term].esp = cur_pcb->parent_esp;
  sched_arr[cur_sched_term].ebp = cur_pcb->parent_ebp;

  /* Remap user program paging back to parent program */
  set_page_dir_entry(USER_PROG, EIGHT_MB + (cur_pcb->parent_pid - 1)*FOUR_MB);

  /* Check if program was using video memory */
  if(cur_pcb->vidmem){
    //disable_page_entry(USER_VIDEO_MEM);
    sched_arr[cur_sched_term].vid_map = 0;
  }

  /* Flush tlb */
  asm volatile ("      \n\
     movl %%cr3, %%eax \n\
     movl %%eax, %%cr3"
     :
     :
     : "eax"
  );

  /*
  inline assembly:
    1st line: zero extend bl and move value into eax so that the correct status is returned from halt
    2nd line: set ebp to parent process ebp
    3rd andd 4th line: jump back to parent's system call linkage
  */
  asm volatile ("       \n\
     movzbl %%bl, %%eax \n\
     movl %0, %%ebp     \n\
     leave              \n\
     ret"
     :
     : "r" (cur_pcb->parent_ebp)
     : "ebp", "eax"
  );

  return 0;
}

/*
 * launch
 *    DESCRIPTION: Create 3 shell programs
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, -1 for failure
 *    SIDE EFFECTS: 3 shell programs are created
 */
int32_t launch(){
  cli();

  int32_t i;
  /* Mark process slots as free */
  for(i = 0; i < MAX_PROGS; i++) process_array[i] = -1;

  dentry_t file_dentry;
  /* Get shell's data */
  if(read_dentry_by_name((uint8_t*)"shell", &file_dentry) == -1){
    /* Return failure */
    sti();
    return -1;
  }

  /* Read the executable */
  uint8_t ELF_buf[NAME_LENGTH];
  uint32_t size; /* Size of executable file */
  if((size = read_data(file_dentry.inode_num, 0, ELF_buf, NAME_LENGTH)) == -1){
    /* Return failure */
    sti();
    return -1;
  }

  /* Check if file is an executable */
  if(ELF_buf[0] != 0x7F || ELF_buf[1] != 0x45 || ELF_buf[2] != 0x4C || ELF_buf[3] != 0x46){
    /* Return failure */
    sti();
    return -1;
  }

  /* Create pcb */
  pcb_t pcb;

  for(i = SHELL_NUM - 1; i >= 0; i--){
    /* Set up shell page */
    set_page_dir_entry(USER_PROG, EIGHT_MB + i*FOUR_MB);

    /* Flush tlb */
    asm volatile ("      \n\
       movl %%cr3, %%eax \n\
       movl %%eax, %%cr3"
       :
       :
       : "eax"
    );

    /* Copy executable to 128MB */
    if((size = read_data(file_dentry.inode_num, 0, (uint8_t*)(USER_PROG + PROG_OFFSET), MAX_PROG_SIZE)) == -1){
      /* Return failure */
      return -1;
    }

    /* Mark process as in use */
    process_array[i] = 1;
  }

  pcb.parent_pid = 0;
  /* Load stdin and stdout jump table and mark as in use */
  pcb.fdt[0].jump_ptr = &stdin_table;
  pcb.fdt[1].jump_ptr = &stdout_table;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;
  pcb.vidmem = 0;
  /* Set arguments to an empty string */
  strncpy((int8_t*)pcb.args, (int8_t*)"", BUF_LENGTH);
  /* Mark remaining file descriptors as not in use */
  for(i = 2; i <= MAX_FD_NUM; i++){
    pcb.fdt[i].flags = -1;
  }

  /* Set count of process numbers to number of terminals */
  process_num = SHELL_NUM;

  /* Place pcb in kernel memory */
  for(i = 0; i < SHELL_NUM; i++){
    pcb.pid = i + 1;
    memcpy((void *)(EIGHT_MB - pcb.pid*EIGHT_KB), &pcb, sizeof(pcb));
  }

  /* Set TSS to point to kernel stack */
  tss.esp0 = EIGHT_MB;
  tss.ss0 = KERNEL_DS;

  /* Get address of first instruction */
  uint32_t program_addr = *(uint32_t*)(ELF_buf + 24);

  /* Save instruction address */
  program_addr_test = program_addr;

  /* Allow pit interrupts */
  prev_sched_term = 0;

  /* Put program_addr into ecx and jump to the context switch */
  asm volatile("       \n\
    movl %0, %%ecx     \n\
    jmp context_switch"
    :
    : "r"(program_addr)
    : "ecx"
  );

  return 0;
}

/*
 * execute
 *    DESCRIPTION: Loads executable into memory and returns to user program
 *    INPUTS: const uint8_t* command - command to execute
 *    OUTPUTS: none
 *    RETURN VALUE: -1 for failure, 0 to 255 for success, 256 for an exception
 *    SIDE EFFECTS: Copies executable program and pcb into memory
 */
int32_t execute(const uint8_t* command){
  /* Limit number of programs */
  if(process_num >= MAX_PROGS){
    /* Return failure */
    return -1;
  }

  /* Check for a user level command */
  /*
  if(process_num > 0 && (command < (const uint8_t*)(USER_PROG) || command >= (const uint8_t*)(USER_PROG + FOUR_MB))){*/
    /* Return failure */ /*
    return -1;
  } */
	uint8_t filename[NAME_LENGTH + 1]; /* Name of the file, with null terminate */
  int32_t i = 0; /* Loop variable */

  if(process_num == 0){
    for(i = 0; i < MAX_PROGS; i++) process_array[i] = -1;
  }

  i = 0;

  /* Mask interrupts */
  cli();

  /* Get the file name */
  while(command[i] != '\0' && command[i] != ' ' && i < NAME_LENGTH){
    filename[i] = command[i];
    i++;
  }

  filename[i] = '\0';

  dentry_t file_dentry;
  /* Check for a valid file name */
  if(read_dentry_by_name(filename, &file_dentry) == -1){
    /* Return failure */
    sti();
    return -1;
  }

  /* Read the executable */
  uint8_t ELF_buf[NAME_LENGTH];
  uint32_t size; /* Size of executable file */
  if((size = read_data(file_dentry.inode_num, 0, ELF_buf, NAME_LENGTH)) == -1){
    /* Return failure */
    sti();
    return -1;
  }

  /* Check if file is an executable */
  if(ELF_buf[0] != 0x7F || ELF_buf[1] != 0x45 || ELF_buf[2] != 0x4C || ELF_buf[3] != 0x46){
    /* Return failure */
    sti();
    return -1;
  }

  uint8_t args[BUF_LENGTH];
  args[0] = '\0'; /* Default args is empty string */

  /* Get argument */
  const uint8_t* command_args = &(command[i]); //start of args
  i = 0;
  int32_t j = 0;

  /* Get rid of leading spaces in arguments */
  while(command_args[i] == ' '){
    i++;
  }
  command_args += i; /* Start of arg without leading spaces */

  /* Copy characters over */
  while(j < BUF_LENGTH && command_args[j] != '\0'){
    args[j] = command_args[j];
    j++;
  }

  /* Null terminate the arguments */
  if(j < BUF_LENGTH){
    args[j] = '\0';
  }
  args[BUF_LENGTH-1] = '\0';

  /* Create pcb */
  pcb_t pcb;
  /* Find a free process slot */
  for(i = 0; i < MAX_PROGS; i++){
    if(process_array[i] == -1){
      pcb.pid = i + 1;
      process_array[i] = 1; /* Mark process slot as in use */
      break;
    }
  }

  /* Set up user page */
  set_page_dir_entry(USER_PROG, EIGHT_MB + (pcb.pid - 1)*FOUR_MB);

  /* Flush tlb */
  asm volatile ("      \n\
     movl %%cr3, %%eax \n\
     movl %%eax, %%cr3"
     :
     :
     : "eax"
  );

  /* Copy executable to 128MB */
  if((size = read_data(file_dentry.inode_num, 0, (uint8_t*)(USER_PROG + PROG_OFFSET), MAX_PROG_SIZE)) == -1){
    /* Return failure */
    return -1;
  }

  pcb.parent_pid = sched_arr[cur_sched_term].process_num;
  /* Load stdin and stdout jump table and mark as in use */
  pcb.fdt[0].jump_ptr = &stdin_table;
  pcb.fdt[1].jump_ptr = &stdout_table;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;
  pcb.vidmem = 0;
  /* Copy arguments to the pcb */
  strncpy((int8_t*)pcb.args, (const int8_t*)args, BUF_LENGTH);
  /* Mark remaining file descriptors as not in use */
  for(i = 2; i <= MAX_FD_NUM; i++){
    pcb.fdt[i].flags = -1;
  }

  /* Increment process count */
  process_num++;

  /* Set parent esp and ebp for child processes */
  pcb.parent_esp = EIGHT_MB - (pcb.parent_pid - 1)*EIGHT_KB;
  //pcb.parent_ebp = sched_arr[cur_sched_term].ebp;

  asm volatile("     \n\
     movl %%ebp, %0"
     : "=r"(pcb.parent_ebp)
  );

  /* Store terminal's current running process */
  // terminals[cur_terminal].cur_pid = pcb.pid;
  sched_arr[cur_sched_term].process_num = pcb.pid;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - pcb.pid*EIGHT_KB), &pcb, sizeof(pcb));

  /* Set TSS to point to kernel stack */
  tss.esp0 = EIGHT_MB - (pcb.pid - 1)*EIGHT_KB;
  tss.ss0 = KERNEL_DS;

  /* Get address of first instruction */
  uint32_t program_addr = *(uint32_t*)(ELF_buf + 24);

  /* Put program_addr into ecx and jump to the context switch */
  asm volatile("       \n\
    movl %0, %%ecx     \n\
    jmp context_switch"
    :
    : "r"(program_addr)
    : "ecx"
  );

  /* Return success */
  return 69;
}

/*
 * read
 *    DESCRIPTION: Reads at a given file descriptor
 *    INPUTS: int32_t fd - file descriptor to read
 *            void* buf - buffer to read to
 *            int32_t nbytes - number of bytes to read
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t read(int32_t fd, void* buf, int32_t nbytes){
  cli(); /* Mask interrupts */

  /* Check for a valid fd */
	if(fd < 0 || fd > MAX_FD_NUM){
		sti();
    /* Return failure */
		return -1;
	}

  /* Pointer to current pcb */
	file_desc curr_file = get_pcb_add()->fdt[fd];

  /* Check if file descriptor is in use */
	if(curr_file.flags == -1){
		sti();
    /* Return failure */
		return -1;
	}

  sti(); /* Restore interrupts */

  /* Jump to read */
	return curr_file.jump_ptr->read(fd, buf, nbytes);
}

/*
 * write
 *    DESCRIPTION: Writes to a given file descriptor
 *    INPUTS: int32_t fd - file descriptor to write to
 *            const void* buf - buffer to write from
 *            int32_t nbytes - number of bytes to write
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t write(int32_t fd, const void* buf, int32_t nbytes){
  cli(); /* Mask interrupts */

  /* Check for a valid fd */
	if(fd < 0 || fd > MAX_FD_NUM){
		sti();
    /* Return failure */
		return -1;
	}

  /* Pointer to current pcb */
	file_desc curr_file = get_pcb_add()->fdt[fd];

  /* Check if descriptor is in use */
	if(curr_file.flags == -1){
		sti();
    /* Return failure */
		return -1;
	}

  sti(); /* Restore interrupts */

  /* Jump to write */
	return curr_file.jump_ptr->write(fd, buf, nbytes);
}

/*
 * open
 *    DESCRIPTION: Creates a new file descriptor in the pcb
 *    INPUTS: const uint8_t* filename - file to open
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t open(const uint8_t* filename){
  /* Check for NULL */
  if(filename == NULL || strncmp((int8_t*)filename, (int8_t*)"", strlen((int8_t*)filename)) == 0){
   /* Return failure */
	 return -1;
  }

  /* Open stdin */
  if(strncmp((int8_t*)filename, (int8_t*)"stdin", strlen((int8_t*)filename)) == 0){
		terminal_open(filename);
    /* Return fd */
		return 0;
	}

  /* Open stdout */
	if(strncmp((int8_t*)filename, (int8_t*)"stdout", strlen((int8_t*)filename)) == 0){
		terminal_open(filename);
    /* Return fd */
		return 1;
	}

	dentry_t dentry;

  /* Get the dentry of the file */
	if(read_dentry_by_name((uint8_t*)filename, &dentry) == -1){
    /* Return failure */
		return -1;
	}

  /* Pointer to the current pcb */
  pcb_t* pcb_start = get_pcb_add();

	int i; /* Loop variable */
  /* Go through each file descriptor */
	for(i = 2; i <= MAX_FD_NUM; i++){
    /* Find file descriptor not in use */
		if(pcb_start->fdt[i].flags == -1){
      /* Load jump table and set inode number */
			switch(dentry.file_type){
        /* RTC */
        case 0: pcb_start->fdt[i].jump_ptr = &rtc_table;
                pcb_start->fdt[i].inode = 0;
                rtc_open((uint8_t*)filename);
                break;
        /* Directory */
        case 1: pcb_start->fdt[i].jump_ptr = &dir_table;
                pcb_start->fdt[i].inode = 0;
                dir_open((uint8_t*)filename);
                break;
        /* File */
        case 2: pcb_start->fdt[i].jump_ptr = &file_table;
                pcb_start->fdt[i].inode = dentry.inode_num;
                file_open((uint8_t*)filename);
                break;
        /* Invalid file type */
        default: return -1;
      }
      /* Mark file descriptor as in use and intialize file position */
      pcb_start->fdt[i].flags = 1;
      pcb_start->fdt[i].file_position = 0;

      /* Return index */
      return i;
		}
	}

  /* Return failure */
	return -1;
}

/*
 * close
 *    DESCRIPTION: Closes a file descriptor in the pcb
 *    INPUTS: int32_t fd - file descriptor to close
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t close(int32_t fd){
  /* Get a pointer to the pcb */
  pcb_t* pcb_start = get_pcb_add();

  /* Check for an invalid fd */
  if(fd < 2 || fd > MAX_FD_NUM){
    /* Return failure */
		return -1;
	}

  /* Check if file desciptor is in use */
	if(pcb_start->fdt[fd].flags == 1){
    /* Jump to file's close and mark as not in use */
    pcb_start->fdt[fd].jump_ptr->close(fd);
    pcb_start->fdt[fd].flags = -1;
    /* Return success */
		return 0;
	}

  /* Return failure */
	return -1;
}

/*
 * getargs
 *    DESCRIPTION:
 *    INPUTS: uint8_t* buf - buffer to copy commands to
 *            int32_t nbytes - size of buffer
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t getargs(uint8_t* buf, int32_t nbytes){
  /* Get pcb */
  pcb_t* cur_pcb = get_pcb_add();

  /* Check for valid input, +1 is for null terminator */
  if(strlen((const int8_t*)cur_pcb->args)+1 > nbytes || buf == NULL || (cur_pcb->args)[0] == '\0' || buf < (uint8_t*)(USER_PROG) || buf >= (uint8_t*)(USER_PROG + FOUR_MB)){
    /* Return failure */
    return -1;
  }

  /* Copy the arguments to userspace */
  strncpy((int8_t*)buf, (const int8_t*)cur_pcb->args, nbytes);

  /* Return success */
  return 0;
}

/*
 * vidmap
 *    DESCRIPTION: Sets up a user page for video memory
 *    INPUTS: uint8_t** screen_start - address of
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, -1 for failure
 *    SIDE EFFECTS: Enables a new page
 */
int32_t vidmap(uint8_t** screen_start){
  /* Check for a valid pointer */
  if(screen_start == NULL || screen_start < (uint8_t**)(USER_PROG) || screen_start >= (uint8_t**)(USER_PROG + FOUR_MB)){
    /* Return failure */
    return -1;
  }

  /* Mark pcb as having video memory page */
  get_pcb_add()->vidmem = 1;
  sched_arr[cur_sched_term].vid_map = 1;

  /* Add page to page table */
  set_page_table2_entry(USER_VIDEO_MEM, VIDEO_MEM_ADDR);

  /* Flush tlb */
  asm volatile ("      \n\
     movl %%cr3, %%eax \n\
     movl %%eax, %%cr3"
     :
     :
     : "eax"
  );

  /* Return virtual address of page */
  *screen_start = (uint8_t *)USER_VIDEO_MEM;

  /* Return success */
  return 0;
}

/*
 * set_handler
 *    DESCRIPTION: Does nothing for now
 *    INPUTS: int32_t signum -
 *            void* handler_address -
 *    OUTPUTS: none
 *    RETURN VALUE: -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t set_handler(int32_t signum, void* handler_address){
  /* Return failure */
  return -1;
}

/*
 * sigreturn
 *    DESCRIPTION: Does nothing for now
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t sigreturn(){
  /* Return failure */
  return -1;
}

/*
 * invalid_read
 *    DESCRIPTION: Function for jump tables with no read
 *    INPUTS: Not used
 *    OUTPUTS: none
 *    RETURN VALUE: -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t invalid_read(int32_t fd, void* buf, int32_t nbytes){
  /* Return failure */
  return -1;
}

/*
 * get_pcb_add
 *    DESCRIPTION: Function for jump tables with no write
 *    INPUTS: Not used
 *    OUTPUTS: none
 *    RETURN VALUE: -1 for failure
 *    SIDE EFFECTS: none
 */
int32_t invalid_write(int32_t fd, const void* buf, int32_t nbytes){
  /* Return failure */
  return -1;
}

/*
 * get_pcb_add
 *    DESCRIPTION: Gets a pointer of the current process' pcb
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: A pcb_t pointer
 *    SIDE EFFECTS: none
 */
pcb_t* get_pcb_add(){
  int32_t esp; /* esp value */

  /* Get current esp value */
  asm volatile("\n\
    movl %%esp, %0"
    : "=r" (esp)
  );

  /* 8kB = 2^13, so mask everything below 13th bit */
  return (pcb_t*)(esp & (EIGHT_MB - EIGHT_KB));
}

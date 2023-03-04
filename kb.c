#include "kb.h"
#include "x86_desc.h"
#include "i8259.h"
#include "lib.h"
#include "pit.h"
#include "paging.h"
#include "lib.h"

#define IRQ_NUM           1
#define RECENT_RELEASE    0x80
#define LEFT_SHIFT        42
#define RIGHT_SHIFT       54
#define CAPS_LOCK         58
#define NEW_LINE          28
#define BACK_SPACE        14
#define CTRL              29

// some useful indices from the keyboard array
#define L_CHAR            38
#define Q_CHAR            16
#define P_CHAR            25
#define A_CHAR            30
#define L_CHAR            38
#define Z_CHAR            44
#define M_CHAR            50
#define F1_CHAR           59
#define F2_CHAR           60
#define F3_CHAR           61

#define CAP_OFFSET        90
#define UP_BOUND          128
#define ALT_CHAR 		      56
#define ALT_RELEASE 	    184


unsigned long flags; /* Hold current flags */

// Table to map the scan_code not actualy 256 character in length
uint8_t kbdus[256] =
{
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock */
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
	  0,  27, '!', '@', '#', '$', '%', '^', '&', '*',	/* 9 */
  '(', ')', '_', '+', '\b',	/* Backspace */
  '\t',			/* Tab */
  'Q', 'W', 'E', 'R',	/* 19 */
  'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',	/* 39 */
 '"', '~',   0,		/* Left shift */
 '|', 'Z', 'X', 'C', 'V', 'B', 'N',			/* 49 */
  'M', '<', '>', '?',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
    0,	/* 59 - F1 key ... > */
    0,   0,   0,   0,   0,   0,   0,   0,
    0,	/* < ... F10 */
    0,	/* 69 - Num lock*/
    0,	/* Scroll Lock */
    0,	/* Home key */
    0,	/* Up Arrow */
    0,	/* Page Up */
  '-',
    0,	/* Left Arrow */
    0,
    0,	/* Right Arrow */
  '+',
    0,	/* 79 - End key*/
    0,	/* Down Arrow */
    0,	/* Page Down */
    0,	/* Insert Key */
    0,	/* Delete Key */
    0,   0,   0,
    0,	/* F11 Key */
    0,	/* F12 Key */
    0,	/* All other keys are undefined */
};

/*
 * terminal_open
 *    DESCRIPTION: Function to be called to open the terminal driver
 *    INPUTS: None
 *    OUTPUTS: None
 *    Return Value: 0 for success
 *    SIDE EFFECTS: Initializes some of the global variables to 0
 */
int32_t terminal_open(const uint8_t* filename){
		terminals[cur_terminal].buf_index = 0;
    return 0;
}

/*
 * terminal_write
 *    DESCRIPTION: Writes characters to the screen based on array that is passed in
 *    INPUTS: void* buf - Array to write to the screen
 *		        int32_t nbytes - The Number of bytes to write to the screen
 *    OUTPUTS: Character from copy_buf
 *    RETURN VALUE: -1 for failure, number of bytes written
 *    SIDE EFFECTS: None
 */
int terminal_write(int32_t fd, const void* buf, int32_t nbytes){
  /* Check for an invalid buffer */
	if(buf == NULL || nbytes < 0){
    /* Return failure */
		return -1;
	}

	int i; /* Loop variable */

  /* Disable interrupts */
  cli_and_save(flags);

  /* Print to the screen */
	for(i = 0; i < nbytes; i++){
		putc(*(uint8_t*)(buf+i));
	}

  /* Restore interrupts */
  restore_flags(flags);

  /* Number of bytes written */
	return nbytes;
}

/*
 * terminal_close
 *    DESCRIPTION: Closes the terminal driver
 *    INPUTS: int32_t fd - file to close
 *    OUTPUTS: None
 *    RETURN VALUE: 0 for success
 *    SIDE EFFECTS: None
 */
int32_t terminal_close(int32_t fd){
		terminals[cur_terminal].buf_index = 0; // Sets the index back to 0
    return 0;
}

/*
 * terminal_read
 *    DESCRIPTION: Reads characters from the keyboard buffer into the buffer
 *    INPUTS: void* buf - Array to read into
 *		        int32_t nbytes - number of bytes to read into the buffer
 *    OUTPUTS: None
 *    RETURN VALUE: -1 for failure, Number of bytes read
 *    SIDE EFFECTS: None
 */
int terminal_read(int32_t fd, void* buf, int32_t nbytes){
    int32_t bytes_read = 0; /* Number of bytes read */

		if(buf == NULL || nbytes < 0){
      /* Return failure */
			return -1;
		}

    terminals[cur_sched_term].line_buffer_flag = 0; /* Sets the flag for determining whether enter is pressed */

    /* Wait until the line is finsied */
		while(!terminals[cur_sched_term].line_buffer_flag){
		}

    cli();

    /* Print the number of bytes desired or the number of bytes typed */
    bytes_read = nbytes < terminals[cur_terminal].buf_index ? nbytes : terminals[cur_terminal].buf_index;
    memcpy(buf, (void*)terminals[cur_terminal].kb_buf, bytes_read);

    terminals[cur_terminal].buf_index = 0;

    sti();
    return bytes_read;
}

/*
 * keyboard_init
 *    DESCRIPTION: Enables keyboard interrupts
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Unmasks IRQ1 for keyboard interrupts on PIC
 */
void keyboard_init(void){
    /* Enable keyboard IRQ on PIC */
		enable_irq(IRQ_NUM);
}


/* Some helper funstions for keyboard_interrupt_handler*/

 /*
  * caps_and_shift
  *    DESCRIPTION: Checks if caps lock and shift are pressed
  *    INPUTS: none
  *    OUTPUTS: none
  *    RETURN VALUE: 1 for pressed, 0 for not pressed
  *    SIDE EFFECTS: none
  */
int32_t caps_and_shift (void) {
  return ((terminals[cur_terminal].caps_lock == 1) && (terminals[cur_terminal].shift_pressed == 1));
}

 /*
  * in_char_range
  *    DESCRIPTION: Checks if scan code is in between q to p (16-25), a to l (30-38), z to m (44-50)
  *    INPUTS: uint8_t scan_code - scan code of the pressed character
  *    OUTPUTS: none
  *    RETURN VALUE: 1 for in range, 0 for not in range
  *    SIDE EFFECTS: none
  */
int32_t in_char_range (uint8_t scan_code) {
  return ((scan_code >= Q_CHAR && scan_code <= P_CHAR) || (scan_code >= A_CHAR && scan_code <= L_CHAR) || (scan_code >= Z_CHAR && scan_code <= M_CHAR));
}

/*
 * print_scancode
 *    DESCRIPTION: prints the scan_code
 *    INPUTS: uint32_t scan_code - scan_code of the key pressed
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: prints the matching characters of the scan_code
 */
void print_scancode (uint8_t scan_code) {
  /* Check for a full terminal buffer */
  if((terminals[cur_terminal].buf_index >= (BUF_LENGTH - 1) && scan_code != NEW_LINE) || terminals[cur_terminal].line_buffer_flag){
    return;
  }

  putc(kbdus[scan_code]); // Prints this scan code to the screen

  /* Store character in terminal buffer */
  terminals[cur_terminal].kb_buf[terminals[cur_terminal].buf_index] = kbdus[scan_code];
  terminals[cur_terminal].buf_index++;
}

 /*
  * caps_no_shift
  *    DESCRIPTION: Checks if caps lock and shift are not pressed
  *    INPUTS: none
  *    OUTPUTS: none
  *    RETURN VALUE: 1 for not pressed, 0 for pressed
  *    SIDE EFFECTS: none
  */
int caps_no_shift (void) {
  return ((terminals[cur_terminal].caps_lock == 1) && (terminals[cur_terminal].shift_pressed != 1));
}

/*
 * recent_release_exec
 *    DESCRIPTION: Executes when scan_code < RECENT_RELEASE. Handles the different scan_codes
 *    INPUTS: uint32_t scan_code - scan_code of the key pressed
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: none
 */
void recent_release_exec (uint8_t scan_code) {
  if(scan_code == CTRL){ //If the CTRL button is pressed
    terminals[cur_terminal].ctrl_pressed = 1;
  }
  else if(terminals[cur_terminal].alt_pressed == 1 && (scan_code == F1_CHAR || scan_code == F2_CHAR || scan_code == F3_CHAR)){
	int32_t terminal;
	switch(scan_code){
		case F1_CHAR:
			terminal = 0;
			break;
		case F2_CHAR:
			terminal = 1;
			break;
		case F3_CHAR:
			terminal = 2;
			break;
	}

    /* Change terminals */
		change_shell(terminal);

  }
  else if(scan_code == L_CHAR && terminals[cur_terminal].ctrl_pressed == 1) {
    int32_t i;
    clear();
    reset_screen();
    for(i = 0; i < terminals[cur_terminal].buf_index; i++){
      putc(terminals[cur_terminal].kb_buf[i]);
    }
  }
  else if(scan_code == BACK_SPACE){
    /* Deletes a buffer character if it is allowed */
    if(terminals[cur_terminal].buf_index <= 0){
      return;
    }
    terminals[cur_terminal].buf_index--;
    back_space();
  } else if(scan_code == ALT_CHAR){
	  terminals[cur_terminal].alt_pressed = 1;
  }
  else if(scan_code == NEW_LINE){
    print_scancode(scan_code);
    terminals[cur_terminal].line_buffer_flag = 1;
  }
  else if(scan_code == (LEFT_SHIFT) || scan_code == (RIGHT_SHIFT)){
    /* Sets shift_pressed to 1. Used ot indicate an on state */
    terminals[cur_terminal].shift_pressed = 1;
  }
  else if(scan_code == CAPS_LOCK){
    // Mod 2 is so the values of capslock flip between 0 or 1
    terminals[cur_terminal].caps_lock = (terminals[cur_terminal].caps_lock + 1) % 2;
  }
  // Caps and shift are pressed and it is a letter
  else if(caps_and_shift() && in_char_range(scan_code)){
    print_scancode(scan_code);
  }
  // Caps on, shift off and it is a letter
  else if(caps_no_shift() && in_char_range(scan_code)){
    print_scancode(scan_code + CAP_OFFSET);
  }
  else if(terminals[cur_terminal].shift_pressed == 1){
    print_scancode(scan_code + CAP_OFFSET);
  }
  else{
    /* Print character to screen */
    print_scancode(scan_code);
  }
}

/*
 * after_release_exec
 *    DESCRIPTION: Executes when scan_code >= RECENT_RELEASE.
 *    INPUTS: uint32_t scan_code - scan_code of the key pressed
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Resets the flags for ctrl and shift to zero.
 */
void after_release_exec (uint8_t scan_code) {
  /* If the key is recently released, gets rid of the shift_pressed flag */
  if(scan_code == LEFT_SHIFT + RECENT_RELEASE || scan_code == RIGHT_SHIFT + RECENT_RELEASE){
    /* 0 is used to indicate a off state */
    terminals[cur_terminal].shift_pressed = 0;
  }
  else if(scan_code == CTRL + RECENT_RELEASE){
    terminals[cur_terminal].ctrl_pressed = 0;
  }
  else if(scan_code == ALT_RELEASE){
	  terminals[cur_terminal].alt_pressed = 0;
  }
}


/*
 * keyboard_interrupt_handler
 *    DESCRIPTION: Handler for keyboard interrupts
 *    INPUTS: none
 *    OUTPUTS: Character on the screen
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Prints keyboard input to screen, and sends EOI
 */
void keyboard_interrupt_handler(void){
    /* Mask interrupts and store flags */
    cli_and_save(flags);
    /* Mask IRQ on PIC */
		disable_irq(IRQ_NUM);

    /* Send EOI to PIC to indicate interrupt is serviced */
		send_eoi(IRQ_NUM);

    /* Read the keyboard data buffer to get the current character */
		uint8_t scan_code = inb(0x60);

		if((scan_code >= CAP_OFFSET && scan_code <= UP_BOUND) || (scan_code >= (CAP_OFFSET + UP_BOUND))){
      /* Allow interrupts again */
  		restore_flags(flags);
      /* Unmask the IRQ1 on the PIC */
  		enable_irq(IRQ_NUM);
			return;
		}

    /* Print to the viewing terminal */
    print_terminal = cur_terminal;

    /* Map video paging to physical video memory */
    set_page_table1_entry(VIDEO_MEM_ADDR, VIDEO_MEM_ADDR);

    /* Flush TLB */
    asm volatile ("     \n\
      movl %%cr3, %%eax \n\
      movl %%eax, %%cr3"
      :
      :
      : "eax"
    );

    /* Tests to see whether the key is being presed versus being released */
		if(scan_code < RECENT_RELEASE){
      recent_release_exec(scan_code);
		} else{
      after_release_exec(scan_code);
		}

    /* Remap video paging to buffer */
    if(cur_sched_term != cur_terminal){
      set_page_table1_entry(VIDEO_MEM_ADDR, sched_arr[cur_sched_term].video_buffer);

      /* Flush TLB */
      asm volatile ("     \n\
        movl %%cr3, %%eax \n\
        movl %%eax, %%cr3"
        :
        :
        : "eax"
      );
    }

    /* Print to the scheduled terminal */
    print_terminal = cur_sched_term;

    /* Allow interrupts again */
		restore_flags(flags);
    /* Unmask the IRQ1 on the PIC */
		enable_irq(IRQ_NUM);
}

#include "types.h"
#include "lib.h"
#include "x86_desc.h"
#include "paging.h"
#include "file_system.h"

/* Constants for paging */
#define TABLE_ENTRIES  1024
#define RW_NOT_PRESENT 0x02
#define RW_PRESENT     0x03
#define USER_MODE      0x07
#define KERNEL_ADDR    0x400000
#define PT_OFFSET      12
#define PD_OFFSET      22
#define FOUR_MB        0x400000
#define PAGE_INDEX     0x3FF
#define NOT_PRESENT    0xFFFFFFFE

/* Page directory array */
static uint32_t page_directory[TABLE_ENTRIES]  __attribute__((aligned (PAGE_SIZE)));
/* First page table array */
static uint32_t first_page_table[TABLE_ENTRIES]  __attribute__((aligned (PAGE_SIZE)));
/* Second page table array */
static uint32_t second_page_table[TABLE_ENTRIES] __attribute__((aligned (PAGE_SIZE)));

/*
 * set_page_dir_entry
 *    DESCRIPTION: Creates an entry in the page directory
 *    INPUTS: int32_t virtual - the virtual address
 *            int32_t physical - the physical address to map to
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success, -1 for failure
 *    SIDE EFFECTS:
 */
int32_t set_page_dir_entry(int32_t virtual, int32_t physical){
  /* Create entry */
  page_directory[virtual >> PD_OFFSET] = physical | 0x087;

  /* Return success */
  return 0;
}

/*
 * set_page_table1_entry
 *    DESCRIPTION: Enables a page in the 1stpage table
 *    INPUTS: int32_t virtual - virtual address
 *            int32_t physical - physical address to map to
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success
 *    SIDE EFFECTS: none
 */
int32_t set_page_table1_entry(int32_t virtual, int32_t physical){
  /* Set entry to user mode */
  first_page_table[(virtual >> PT_OFFSET) & PAGE_INDEX] = physical | USER_MODE;

  /* Return success */
  return 0;
}

/*
 * set_page_table2_entry
 *    DESCRIPTION: Enables a page in the 2nd page table
 *    INPUTS: int32_t virtual - virtual address
 *            int32_t physical - physical address to map to
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success
 *    SIDE EFFECTS: none
 */
int32_t set_page_table2_entry(int32_t virtual, int32_t physical){
  /* Set entry to user mode */
  second_page_table[(virtual >> PT_OFFSET) & PAGE_INDEX] = physical | USER_MODE;

  /* Return success */
  return 0;
}

/*
 * disable_page_entry
 *    DESCRIPTION: Disables a page in the page table
 *    INPUTS: int32_t virtual - address of page to close
 *    OUTPUTS: none
 *    RETURN VALUE: 0 for success
 *    SIDE EFFECTS: none
 */
int32_t disable_page_entry(int32_t virtual){
  /* Mark page as not present */
  second_page_table[(virtual >> PT_OFFSET) & PAGE_INDEX] &= NOT_PRESENT;

  /* Return success */
  return 0;
}

/*
 * get_dir
 *    DESCRIPTION: Get an entry from page directory
 *    INPUTS: uint32_t i - Index into page directory
 *    OUTPUTS: none
 *    RETURN VALUE: An entry from the page directory
 *    SIDE EFFECTS: none
 */
uint32_t get_dir(unsigned int i){
  return page_directory[i];
}

/*
 * get_page
 *    DESCRIPTION: Get an entry from page table
 *    INPUTS: uint32_t i - Index into page table
 *    OUTPUTS: none
 *    RETURN VALUE: An entry from the page table
 *    SIDE EFFECTS: none
 */
uint32_t get_page(unsigned int i){
  return first_page_table[i];
}

/*
 * init_paging
 *    DESCRIPTION: Begins paging initialization
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: none
 */
void init_paging(void){
    /* Initialize page directory array */
    init_page_directory();

    /* Initialize first page table array */
    init_page_table();

    /* Turn on paging */
    enable_paging();
}

/*
 * init_page_directory
 *    DESCRIPTION: Initializes the page directory
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Sets kernel page and first page table, all others not present
 */
void init_page_directory(void){
    int i; /* Variable to loop over table entries */

    /* Go through each entry in the table */
    for(i = 0; i < TABLE_ENTRIES; i++){
      /* Set entry to not present */
      page_directory[i] = RW_NOT_PRESENT;
    }

    /* Set the first entry to the address of the page table, and mark it as present */
    page_directory[0] = ((unsigned int)first_page_table) | RW_PRESENT;

    /*
     * Set the second entry to the address of the kernel (4 MB). The page size bit must be
     * enabled. The entry is marked as present as well.
     */
    page_directory[KERNEL_ADDR >> PD_OFFSET] = (KERNEL_ADDR | 0x083);

    /* Enable the second page table */
    page_directory[USER_VIDEO_MEM >> PD_OFFSET] = ((unsigned int)second_page_table) | USER_MODE;
}

/*
 * init_page_table
 *    DESCRIPTION: Initializes the first page table
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Sets video memory page, all others not present
 */
void init_page_table(void){
    int i; /* Variable to loop */

    /* Go through each entry in the page table */
    for(i = 0; i < TABLE_ENTRIES; i++){
      /* Address of each page is every 4 kB (PAGE_SIZE), and entry is not present */
      first_page_table[i] = (i * PAGE_SIZE) | RW_NOT_PRESENT;
      second_page_table[i] = (i * PAGE_SIZE) | RW_NOT_PRESENT;
    }

    /* Set video memory page to present, read/write, and supervisor mode */
    first_page_table[VIDEO_MEM_ADDR >> PT_OFFSET] |= RW_PRESENT;

    /*Sets up temporary buffers for when the process is not the visible process*/
    first_page_table[FIRST_SHELL >> PT_OFFSET] |= RW_PRESENT;
    first_page_table[SECOND_SHELL >> PT_OFFSET] |= RW_PRESENT;
    first_page_table[THIRD_SHELL >> PT_OFFSET] |= RW_PRESENT;
}

/*
 * enable_paging
 *    DESCRIPTION: Initializes paging
 *    INPUTS: none
 *    OUTPUTS: none
 *    RETURN VALUE: none
 *    SIDE EFFECTS: Sets cr3 to hold the page directory address, and enables paging in cr0 and cr4
 */
void enable_paging(void){
    /* The most significant bit turns on paging */
    uint32_t enable = 0x80000001;

    /*
     * Macro to store address of page directory in cr3. Also enables paging in cr0, and sets the
     * Page Size bit in cr4 to allow for 4MB pages.
     */
    asm volatile ("               \n\
                movl %%cr4, %%eax \n\
                or $0x10, %%eax   \n\
                mov %%eax, %%cr4  \n\
                movl %0, %%eax    \n\
                movl %%eax, %%cr3 \n\
                movl %%cr0, %%eax \n\
                orl %1,%%eax      \n\
                movl %%eax, %%cr0"
            :
            : "r"(page_directory), "r"(enable)
            : "eax"
    );
}

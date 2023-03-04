#ifndef _PAGING_H
#define _PAGING_H

#define VIDEO_MEM_ADDR 0xB8000
#define USER_VIDEO_MEM 0x4500000
#define FIRST_SHELL    (VIDEO_MEM_ADDR + PAGE_SIZE)
#define SECOND_SHELL   (VIDEO_MEM_ADDR + 2*PAGE_SIZE)
#define THIRD_SHELL    (VIDEO_MEM_ADDR + 3*PAGE_SIZE)
#define PAGE_SIZE      4096

#ifndef ASM

/* Create an entry in the page directory */
int32_t set_page_dir_entry(int32_t virtual, int32_t physical);

/* Create an entry in the 2nd page table */
int32_t set_page_table2_entry(int32_t virtual, int32_t physical);

/* Create an entry in the 1st page table */
int32_t set_page_table1_entry(int32_t virtual, int32_t physical);

int32_t disable_page_entry(int32_t virtual);

/* Get a page directory entry */
uint32_t get_dir(uint32_t i);

/* Get a page table entry */
uint32_t get_page(uint32_t i);

/* Initialize the page directory */
void init_page_directory(void);

/* Initialize the page table */
void init_page_table(void);

/* Create page directories and page tables */
void init_paging(void);

/* Activate paging */
void enable_paging(void);

#endif /* ASM */

#endif /* _PAGING_H */

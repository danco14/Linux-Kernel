#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "paging.h"
#include "kb.h"
#include "file_system.h"
#include "rtc.h"
#include "syscalls.h"

#define SYSCALL_NUM 0x80
#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 *
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) &&
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

/*
 * idt_test_2
 *		ASSERTS: Reserved entries are correct
 *		INPUTS: None
 *    OUTPUTS: PASS/FAIL
 *		SIDE EFFECTS: None
 *		COVERAGE: IDT entries
 *		FILES: x86_desc.h
 */
int idt_test_2(){
  TEST_HEADER;

  int result=PASS;
  int i;
  for(i=0;i<32;i++){ //32 is used to indicate the number of exceptions that are present in the IDT table
    if(!(idt[i].reserved0==0 &&
         idt[i].reserved1==1 &&
         idt[i].reserved2==1 &&
         idt[i].reserved3==1 &&
         idt[i].reserved4==0 )){ //This idicates the field 01110000000 if it is not true asserts a failure
      	assertion_failure();
      result=FAIL;
    }
  }

  for(i=32;i<256;i++){ // 32 is used to indicate the number of exceptions in the idt talbe while 256 is the number of values that are in the table
    if(!(idt[i].reserved0==0 &&
         idt[i].reserved1==1 &&
         idt[i].reserved2==1 &&
         idt[i].reserved3==0 &&
         idt[i].reserved4==0)){ //This indicates the field 01100000000 if this is not true asserts a failure
      	assertion_failure();
      	result=FAIL;
    }
  }

  return result;
}

/*
 * idt_test_3
 *		ASSERTS: dpl and size are correct
 *		INPUTS: None
 *    OUTPUTS: None
 *		SIDE EFFECTS: None
 *		COVERAGE: IDT entries
 *		FILES: x86_desc.h
 */
int idt_test_3(){
  TEST_HEADER;

  int result=PASS;
  int i;
  for(i=0;i<256;i++){ //256 is the number used to indicate the number of entries in the idt table
    if(!(idt[i].dpl==0 && idt[i].size==1) && i !=SYSCALL_NUM){ //Checks if the priority level is set to 0 and the size is set to 32 bits
      	assertion_failure();
      result=FAIL;
    }
  }

  if(!(idt[SYSCALL_NUM].dpl==3 && idt[SYSCALL_NUM].size==1)){ //checks if the priotiy level is set to 3 and the size is set to 1
    assertion_failure();
    result=FAIL;
  }

  return result;
}

/*
 * page_fault_test0
 *		ASSERTS: Dereferencing NULL causes page fault
 *		INPUTS: None
 *    OUTPUTS: FAIL or Exception
 *		SIDE EFFECTS: Exception
 *		COVERAGE: Paging
 *		FILES: Paging.c
 */
void page_fault_test0(){
	TEST_HEADER;

  unsigned int address = NULL;
  printf("Accessing address: %x\n", address);
  (*(int*)address) = 0x1;
  TEST_OUTPUT("page_fault_test0", FAIL);
}

/*
 * page_fault_test1
 *		ASSERTS: Page fault for out of bounds memory access
 *		INPUTS: None
 *    OUTPUTS: Exception or FAIL
 *		SIDE EFFECTS: Exception
 *		COVERAGE: Paging
 *		FILES: Paging.c
 */
void page_fault_test1(){
  TEST_HEADER;

  unsigned int address = 0x3FFFFF;
  printf("Accessing address: %x\n", address);
  (*(int*)address) = 0x1;
  TEST_OUTPUT("page_fault_test1", FAIL);
}

/*
 * page_fault_test2
 *		ASSERTS: Page fault for out of bounds memory access
 *		INPUTS: None
 *    OUTPUTS: Exception or FAIL
 *		SIDE EFFECTS: Exception
 *		COVERAGE: Paging
 *		FILES: paging.c
 */
 void page_fault_test2(){
  TEST_HEADER;

  unsigned int address = 0x800001;
  printf("Accessing address: %x\n", address);
  (*(int*)address) = 0x1;
  TEST_OUTPUT("page_fault_test2", FAIL);
}

/*
 * page_fault_test3
 *		ASSERTS: Exception when accessing just outside of video memory page
 *		INPUTS: None
 *    OUTPUTS: Exception or FAIL
 *		SIDE EFFECTS: Exception
 *		COVERAGE: Paging
 *		FILES: paging.c
 */
void page_fault_test3(){
  TEST_HEADER;

  unsigned int address = 0xB7FFF;
  printf("Accessing address: %x\n", address);
  (*(int*)address) = 0x1;
  TEST_OUTPUT("page_fault_test3", FAIL);
}

/*
 * page_fault_test4
 *		ASSERTS: Page fault for accessing memory just outside kernel memory
 *		INPUTS: None
 *    OUTPUTS: Exception or FAIL
 *		SIDE EFFECTS: Exception
 *		COVERAGE: Paging
 *		FILES: paging.c
 */
void page_fault_test4(){
  TEST_HEADER;

  unsigned int address = 0xC0001;
  printf("Accessing address: %x\n", address);
  (*(int*)address) = 0x1;
  TEST_OUTPUT("page_fault_test3", FAIL);
}

/*
 * page_fault_test5
 *		ASSERTS: Video memory is accessable at beginning address
 *		INPUTS: None
 *    OUTPUTS: Exception or PASS
 *		SIDE EFFECTS:
 *		COVERAGE:
 *		FILES:
 */
void page_fault_test5(){
  TEST_HEADER;

  unsigned int address = 0xB8000;
  printf("Accessing address: %x\n", address);
  (*(int*)address) = 0x1;
  TEST_OUTPUT("page_fault_test5", PASS);
}

/*
 * page_fault_test6
 *		ASSERTS: Kernel memory is accessable at beginning address
 *		INPUTS: None
 *    OUTPUTS: PASS or Exception
 *		SIDE EFFECTS: Exception
 *		COVERAGE: Paging
 *		FILES: paging.c
 */
void page_fault_test6(){
  TEST_HEADER;

  unsigned int address = 0x400000;
  printf("Accessing address: %x\n", address);
  (*(int*)address) = 0x1;
  TEST_OUTPUT("page_fault_test6", PASS);
}

/*
 * divide_zero_test
 *		ASSERTS: Arithmetic exception for dividing by zero
 *		INPUTS: None
 *    OUTPUTS: FAIL or Exception
 *		SIDE EFFECTS: Exception
 *		COVERAGE: Exception in IDT
 *		FILES: x86_desc.h and init_idt.c
 */
void divide_zero_test(){
  TEST_HEADER;

  printf("Attempting division 69/0 \n");
  int x = 69;
  int y = 0;
  x = x / y;
  TEST_OUTPUT("divide_zero_test", FAIL);
}

/*
 * page_directory_test
 *		ASSERTS: Page directory has tables marked not present
 *		INPUTS: None
 *    OUTPUTS: PASS or FAIL
 *		SIDE EFFECTS: None
 *		COVERAGE: Paging
 *		FILES: paging.c
 */
int page_directory_test(){
  TEST_HEADER;

  int result = PASS;
  int i;

  if((get_dir(0) & 0x03) == 0x02){
    return FAIL;
  }

  if((get_dir(1) & 0x03) != 0x03){
    return FAIL;
  }

  for(i=2; i<1024; i++){
    if((get_dir(i) & 0x03) != 0x02){
      result = FAIL;
      break;
    }
  }

  return result;
}

/*
 * page_table_test
 *		ASSERTS: Page table has not present pages and correct addresses
 *		INPUTS: None
 *    OUTPUTS: PASS or FAIL
 *		SIDE EFFECTS: None
 *		COVERAGE: Paging
 *		FILES: paging.c
 */
int page_table_test(){
  TEST_HEADER;

  int result=PASS;
  int i;

  for(i=0;i<1024;i++){
		if(i!=((get_page(i))>>12)){
			return FAIL;
		}
    if(i==0xB8){
      if((get_page(i)&0x03)!=0x03){
        return FAIL;
      }
    }
    else if((get_page(i)&0x03)!=0x02){
    	return FAIL;
    }
  }

  return result;
}

/* Checkpoint 2 tests */

/*
 * rtc_write_test
 *		ASSERTS: RTC write changes the frequency
 *		INPUTS: None
 *    OUTPUTS: RTC clock
 *		SIDE EFFECTS: None
 *		COVERAGE: RTC Driver
 *		FILES: rtc.c
 */
void rtc_write_test(){
	rtc_test_flag = 1;
	int32_t buf = 2;
	uint32_t count = 0;
  clear();
	reset_screen();
	if(rtc_write(0, &buf, 4) == -1){
		printf("Invalid frequency\n");
	}
	count = 0;
	while(count < 1000000000){
		count++;
	}
	clear();
  reset_screen();
	buf = 8;
	rtc_write(0, &buf, 4);
	count = 0;
	while(count < 1000000000){
		count++;
	}
	clear();
  reset_screen();
	buf = 64;
	if(rtc_write(0, &buf, 4) == -1){
		printf("Invalid frequency\n");
	}
	count = 0;
	while(count < 1000000000){
		count++;
	}
	clear();
  reset_screen();
	if(rtc_write(0, NULL, 4) == -1){
		printf("Invalid frequency\n");
	}
	count = 0;
	while(count < 1000000000){
		count++;
	}
	clear();
  reset_screen();
	buf = 52;
	if(rtc_write(0, &buf, 4) == -1){
		printf("Invalid frequency\n");
	}
	count = 0;
	while(count < 1000000000){
		count++;
	}
	clear();
  reset_screen();
	buf = -1;
	if(rtc_write(0, &buf, 4) == -1){
		printf("Invalid frequency\n");
	}
	count = 0;
	while(count < 1000000000){
		count++;
	}
	clear();
  reset_screen();
	buf = 2048;
	if(rtc_write(0, &buf, 4) == -1){
		printf("Invalid frequency\n");
	}
	count = 0;
	while(count < 1000000000){
		count++;
	}
	rtc_test_flag = 0;
}

/*
 * rtc_read_test
 *		ASSERTS: RTC read waits for rtc interrupt
 *		INPUTS: None
 *    OUTPUTS: When RTC waiting stops
 *		SIDE EFFECTS: None
 *		COVERAGE: RTC Drivers
 *		FILES: rtc.c
 */
void rtc_read_test(){
	uint8_t buf[1];
	rtc_read(0, buf, 0);
}

/*
 * rtc_open_test
 *		ASSERTS: RTC open sets the frequency to 2Hz
 *		INPUTS: None
 *    OUTPUTS: Default RTC speed and 2Hz RTC speed
 *		SIDE EFFECTS: None
 *		COVERAGE: RTC Driver
 *		FILES: rtc.c
 */
void rtc_open_test(){
	rtc_test_flag = 1;
	uint32_t count = 0;
	while(count < 100000000){
		count++;
	}
	printf("\nrtc_open called\n");
	uint8_t n[] = "hello";
	rtc_open(n);
}

/*
*Description- Tests the return value for the write function
*Inputs- None
*Outputs-None
*Return Value- None
*Side Effects prints to screen the values written
*/
void write_ret_val(void){
  terminal_open((uint8_t*)"blah");
   unsigned char string[128];
  int i;
  for(i=0;i<128;i++){
  	string[i]='a';
  }

  int ret_val= terminal_write(0,string,12);

  if(ret_val==12){
  	TEST_OUTPUT("write_ret_val",PASS);
  } else{
    TEST_OUTPUT("write_ret_val",FAIL);
  }
	  terminal_close(0);
}


/* read_ret _val
 * Description- Test the return value fr the read function
 * Inputs-None
 * Outpus-None
 * Return Value-None
 * Side Effects-Prints the screen
 */
void read_ret_val(void){
  terminal_open((uint8_t*)"blah");
   unsigned char string[128];

  int ret_val= terminal_read(0, string,1);

  if(ret_val==1){
  	TEST_OUTPUT("read_ret_val",PASS);
  } else{
    TEST_OUTPUT("read_ret_val",FAIL);
  }

  terminal_close(0);
}

/* write_null
 * Description- Test the write funciton for the case that a NULL Pointer is passed
 * Inputs- None
 * Outputs-None
 * Return Value- None
 * Side Effects- None
 */
void write_null(void){
  	terminal_open((uint8_t*)"blah");
  	unsigned char* string=NULL;

  int ret_val=terminal_write(0, string, 1);


  //printf("%d",ret_val);
  if(ret_val==-1){
  	TEST_OUTPUT("write_null",PASS);
  } else{
    TEST_OUTPUT("write_null",FAIL);
  }

    terminal_close(0);
}

/* read_null
 * Description- Tests the Read function for the case that a NULL Pointer is passed
 * Inputs-none
 * Outputs-none
 * Return Value-None
 * Side Effects- None
 */
void read_null(void){
  terminal_open((uint8_t*)"blah");

  unsigned char string[128];
  int ret_val=terminal_read(0, string, 1);
  //printf("%d",ret_val);
  if(ret_val==0){
  	TEST_OUTPUT("read_null",PASS);
  } else{
    TEST_OUTPUT("read_null",FAIL);
  }

  terminal_close(0);
}

/* buffer_write
 * Description- writes "Chief" to the screen to the buffer and onto the screen using the write function
 * Inputs- None
 * Outputs- None
 * Return Value- None
 * Side Effects-None
 */
void buffer_write(void){
	terminal_open((uint8_t*)"blah");
	unsigned char string[]="Chief";
	terminal_write(0, string,(int)strlen((char*)string));

	terminal_close(0);
}

/* buffer_read
 * Description-Description- reads from the buffer inputed to the user
 * Inputs-None
 * Outputs-None
 * Return Value- None
 * Side Effects-None
 * */
void buffer_read(void){
		terminal_open((uint8_t*)"blah");
		unsigned char string[128];
		terminal_read(0, string, 128);
		terminal_close(0);

}

/* buffer_overflow_read
 * Description- tests the buffer for overflow during the read function
 * Inputs- None
 * Outputs-None
 * Return Value-None
 * Side Effects- None
 * */
void buffer_overflow_read(void) {
  terminal_open((uint8_t*)"blah");
  unsigned char string[140];

  int temp = terminal_read(0, string, 140);

  putc('\n');
  // the write should just print out 128 characters
  terminal_write(0, string, temp);
  terminal_close(0);
}


/* buffer_overflow_write
 * Description- test the buffer for overflow during the write function
 * Inputs-None
 * Outputs-None
 * Return Value-None
 * Side Effects- None
 * */
void buffer_overflow_write(void) {


  unsigned char string[160];
  int i;
  for(i=0;i<160;i++){
    string[i]='a';
  }

  terminal_open((uint8_t*)"blah");

  terminal_write(0, string, 60);

  terminal_close(0);
}

/* press_enter_test
 * Description- Test the read and write buffer for the case that
 * Inputs- None
 * Outputs- None
 * Return Value- None
 * Side Effects- None
 * */
void press_enter_test(void){

	terminal_open((uint8_t*)"blah");
	unsigned char string[128];

	int temp=terminal_read(0, string, 128);

	terminal_write(0, string,temp);
	terminal_close(0);
}


/*
 * read_file_test
 *		ASSERTS: file can be read
 *		INPUTS: uint8_t* name
 *    OUTPUTS: None
 *		SIDE EFFECTS: None
 *		COVERAGE: file open file close
 *		FILES: file_system.c
 */
void read_file_test(uint8_t* name){
	int8_t buf[10000];
  volatile uint32_t size;
  int i;
  file_open(name);
  if((size = file_read(69, buf, 10000)) == -1){
    printf("Error\n");
  }
  file_close(0);
  buf[10000] = '\0';
  for(i=0; i<size; i++){
    putc(buf[i]);
  }
}

/*
 * dir_read_test
 *		ASSERTS: dir can be read
 *		INPUTS: None
 *    OUTPUTS: None
 *		SIDE EFFECTS: None
 *		COVERAGE: dir open dir close
 *		FILES: file_system.c
 */
void dir_read_test(){
	uint8_t buf[33];
  int32_t cnt;
  uint8_t dirName[]={'.'};
  dir_open((const uint8_t*)dirName);
  while (0 != (cnt = dir_read (5, (void*)buf, 32))) {
    if(cnt==-1)break;
    puts((int8_t*)buf);
    printf("\n");
  }
  dir_close(0);
}

/*
 * file_index_test
 *		ASSERTS: the file index can be read and has the correct file_name, file_type and inode_num in it
 *		INPUTS: uint32_t* index
 *    OUTPUTS: None
 *		SIDE EFFECTS: None
 *		COVERAGE: dentry_t, read_dentry_by_index
 *		FILES: file_system.c
 */
void file_index_test(uint32_t index){
	dentry_t dentry;
	read_dentry_by_index(index,&dentry);
	printf("File Name: %s\n",dentry.file_name);
	printf("File Type: %d\n",dentry.file_type);
	printf("inode Num: %d\n",dentry.inode_num);
}

/*
 * fread_fail_test
 *		ASSERTS: reading from a file without calling file_open will fail, else pass
 *		INPUTS: None
 *    OUTPUTS: None
 *		SIDE EFFECTS: None
 *		COVERAGE: file_read
 *		FILES: file_system.c
 */
void fread_fail_test(void){
	TEST_HEADER;
	int8_t buf[10000];
	if(file_read(69,buf,100)==-1){
		TEST_OUTPUT("fread_fail_test", PASS);
	}
	else{
		TEST_OUTPUT("fread_fail_test", FAIL);
	}
}

/*
 * dread_fail_test
 *		ASSERTS: reading from dir withouting call dir_open works
 *		INPUTS: None
 *    OUTPUTS: None
 *		SIDE EFFECTS: None
 *		COVERAGE: dir_read
 *		FILES: file_system.c
 */
void dread_fail_test(void){
	TEST_HEADER;
	uint8_t buf[33];
	if(dir_read(69,buf,32)==-1){
		TEST_OUTPUT("dread_fail_test", PASS);
	}
	else{
		TEST_OUTPUT("dread_fail_test", FAIL);
	}
}

/*
 * read_test
 *		ASSERTS: succesful read from the buffer
 *		INPUTS: None
 *    OUTPUTS: None
 *		SIDE EFFECTS: None
 *		COVERAGE: terminal read
 *		FILES: kb.c
 */
void read_test(){
	uint8_t buf[1024];
	int32_t count;
	while(1){
		count = terminal_read(0, buf, 1023);
	}
}

/* Checkpoint 3 tests */

/* Function pointers for stdin (only has terminal read) */
jump_table stdin_table_1 = {NULL, terminal_read, terminal_open, terminal_close};

/* function pointers for stdout(only has terminal write) */
jump_table stdout_table_1 = {terminal_write, NULL, terminal_open, terminal_close};

/* jump table for files*/
jump_table file_table_1 = {file_write, file_read, file_open, file_close};
/*
 * execute_fail_test
 * 		ASSERTS: Running execute on invalid executable file should fail
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: execute
 * 		FILES: syscalls.c
 * */
void execute_fail_test(void){
  TEST_HEADER;
  uint8_t file[]="thanOS.exe";
  int8_t ret;
  ret = execute((uint8_t*)file);
  if(ret==-1){
    TEST_OUTPUT("execute_fail_test", PASS);
  }
  else{
    TEST_OUTPUT("execute_fail_test", FAIL);
  }
}

/*
 * execute_fail_test_2
 * 		ASSERTS: Running execute on text file should fail
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: execute
 * 		FILES: syscalls.c
 * */
void execute_fail_test_2(void){
  TEST_HEADER;
  uint8_t file[]="frame0.txt";
  int8_t ret;
  ret = execute((uint8_t*)file);
  if(ret==-1){
    TEST_OUTPUT("execute_fail_test", PASS);
  }
  else{
    TEST_OUTPUT("execute_fail_test", FAIL);
  }
}

#define EIGHT_MB 0x800000

 /*
  * open_null_test
  * 		ASSERTS: Opening a NULL file should fail
  * 		INPUTS: None
	* 		OUTPUTS: PASS - if ret = -1
  							 FAIL - if ret != -1
  * 		SIDE EFFECTS: None
  * 		COVERAGE: open
  * 		FILES: syscalls.c
  * */
void open_null_test(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));

  int ret=open(NULL);

  if(ret==-1){
    TEST_OUTPUT("open_null_test",PASS);
  } else{
    TEST_OUTPUT("open_null_test",FAIL);
  }

}

/*
 * open_test_fail
 * 		ASSERTS: Opening a non existing file should lead to error
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
 							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: open
 * 		FILES: syscalls.c
 * */
void open_test_fail(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));

  int ret=open((uint8_t*)"PiedPiper.exe");

  if(ret==-1){
    TEST_OUTPUT("open_test_fail",PASS);
  } else{
    TEST_OUTPUT("open_test_fail",FAIL);
  }
}

/*
 * close_test_fail_1
 * 		ASSERTS: Cannot access fd < 0
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: close
 * 		FILES: syscalls.c
 * */
void close_test_fail_1(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));

  int ret=close(-1);

  if(ret==-1){
    TEST_OUTPUT("close_test_fail_1",PASS);
  } else{
    TEST_OUTPUT("close_test_fail_1",FAIL);
  }
}
/*
 * close_test_fail_2
 * 		ASSERTS: Cannot access fd > 7
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: close
 * 		FILES: syscalls.c
 * */
void close_test_fail_2(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;


  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));

  int ret=close(8);

  if(ret==-1){
    TEST_OUTPUT("close_test_fail_2",PASS);
  } else{
    TEST_OUTPUT("close_test_fail_2",FAIL);
  }
}

/*
 * read_test_fail_1
 * 		ASSERTS: Cannot access fd < 0
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: read
 * 		FILES: syscalls.c
 * */
void read_test_fail_1(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;


  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  unsigned char read_buf[128];
  int ret=read(-1,read_buf,128);

  if(ret==-1){
    TEST_OUTPUT("read_test_fail_1",PASS);
  } else{
    TEST_OUTPUT("read_test_fail_1",FAIL);
  }

}

/*
 * read_test_fail_2
 * 		ASSERTS: Cannot access fd > 7
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: read
 * 		FILES: syscalls.c
 * */
void read_test_fail_2(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;


  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  unsigned char read_buf[128];
  int ret=read(8,read_buf,128);

  if(ret==-1){
    TEST_OUTPUT("read_test_fail_2",PASS);
  } else{
    TEST_OUTPUT("read_test_fail_2",FAIL);
  }

}

/*
 * read_test_fail_3
 * 		ASSERTS: Cannot access read a NULL buffer
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: read
 * 		FILES: syscalls.c
 * */
void read_test_fail_3(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;


  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  int ret=read(0,NULL,128);

  if(ret==-1){
    TEST_OUTPUT("read_test_fail_3",PASS);
  } else{
    TEST_OUTPUT("read_test_fail_3",FAIL);
  }

}

/*
 * read_test_fail_4
 * 		ASSERTS: Cannot access read a soze of buffer < 0
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: read
 * 		FILES: syscalls.c
 * */
void read_test_fail_4(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  unsigned char read_buf[128];
  int ret=read(0,read_buf,-1);

  if(ret==-1){
    TEST_OUTPUT("read_test_fail_4",PASS);
  } else{
    TEST_OUTPUT("read_test_fail_4",FAIL);
  }

}

/*
 * read_test_fail_5
 * 		ASSERTS: Cannot read stout
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: read
 * 		FILES: syscalls.c
 * */
void read_test_fail_5(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  unsigned char read_buf[128];
  int ret=read(1,read_buf,128);

  if(ret==-1){
    TEST_OUTPUT("read_test_fail_5",PASS);
  } else{
    TEST_OUTPUT("read_test_fail_5",FAIL);
  }

}

/*
 * write_test_fail_1
 * 		ASSERTS: Cannot write when fd < 0
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: write
 * 		FILES: syscalls.c
 * */
void write_test_fail_1(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;


  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  unsigned char read_buf[128];
  int ret = write(-1,read_buf,128);

  if(ret==-1){
    TEST_OUTPUT("write_test_fail_1",PASS);
  } else{
    TEST_OUTPUT("write_test_fail_1",FAIL);
  }

}

/*
 * write_test_fail_2
 * 		ASSERTS: Cannot write when fd > 7
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: write
 * 		FILES: syscalls.c
 * */
void write_test_fail_2(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;


  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  unsigned char read_buf[128];
  int ret=write(8,read_buf,128);

  if(ret==-1){
    TEST_OUTPUT("write_test_fail_2",PASS);
  } else{
    TEST_OUTPUT("write_test_fail_2",FAIL);
  }

}

/*
 * write_test_fail_3
 * 		ASSERTS: Cannot write when buff is NULL
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: write
 * 		FILES: syscalls.c
 * */
void write_test_fail_3(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;


  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  //unsigned char read_buf[128];
  int ret=write(1,NULL,128);

  if(ret==-1){
    TEST_OUTPUT("write_test_fail_3",PASS);
  } else{
    TEST_OUTPUT("write_test_fail_3",FAIL);
  }

}

/*
 * write_test_fail_4
 * 		ASSERTS: Cannot write when size of buff < 0
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: write
 * 		FILES: syscalls.c
 * */
void write_test_fail_4(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  unsigned char write_buf[128];
  int ret=write(1,write_buf,-1);

  if(ret==-1){
    TEST_OUTPUT("write_test_fail_4",PASS);
  } else{
    TEST_OUTPUT("write_test_fail_4",FAIL);
  }

}

/*
 * write_test_fail_5
 * 		ASSERTS: Cannot write stdin
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: write
 * 		FILES: syscalls.c
 * */
void write_test_fail_5(void){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  unsigned char write_buf[128];
  int ret=write(0,write_buf,128);

  if(ret==-1){
    TEST_OUTPUT("write_test_fail_5",PASS);
  } else{
    TEST_OUTPUT("write_test_fail_5",FAIL);
  }

}

/*
 * write_test_fail_6
 * 		ASSERTS: Cannot write to files
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: write
 * 		FILES: syscalls.c
 * */
void write_test_fail_6(void){
	pcb_t pcb;
	pcb.fdt[0].jump_ptr= &stdin_table_1;
	pcb.fdt[1].jump_ptr= &stdout_table_1;
	pcb.fdt[2].jump_ptr=  &file_table_1;
	pcb.fdt[0].flags=1;
	pcb.fdt[1].flags=1;
	pcb.fdt[2].flags=1;
	

	memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
	open((uint8_t*)"pingpong");
	unsigned char write_buf[128];
        int ret=write(2,write_buf,128);

  if(ret==-1){
    TEST_OUTPUT("write_test_fail_6",PASS);
  } else{
    TEST_OUTPUT("write_test_fail_6",FAIL);
  }

}
/*
 * close_fail_1
 * 		ASSERTS: Cannot close when fd < 0
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: close
 * 		FILES: syscalls.c
 * */
void close_fail_1(){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  int ret=close(-1);

  if(ret==-1){
    TEST_OUTPUT("close_fail_1",PASS);
  } else{
    TEST_OUTPUT("close_fail_1",FAIL);
  }
}

/*
 * close_fail_2
 * 		ASSERTS: Cannot close if there is nothing open
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: close
 * 		FILES: syscalls.c
 * */
void close_fail_2(){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  int ret=close(3);

  if(ret==-1){
    TEST_OUTPUT("close_fail_2",PASS);
  } else{
    TEST_OUTPUT("close_fail_2",FAIL);
  }
}

/*
 * close_fail_3
 * 		ASSERTS: Cannot close when fd > 8
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: close
 * 		FILES: syscalls.c
 * */
void close_fail_3(){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  int ret=close(8);

  if(ret==-1){
    TEST_OUTPUT("close_fail_3",PASS);
  } else{
    TEST_OUTPUT("close_fail_3",FAIL);
  }
}

/*
 * close_fail_4
 * 		ASSERTS: Cannot close stdin
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: close
 * 		FILES: syscalls.c
 * */
void close_fail_4(){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  int ret=close(0);

  if(ret==-1){
    TEST_OUTPUT("close_fail_4",PASS);
  } else{
    TEST_OUTPUT("close_fail_4",FAIL);
  }
}

/*
 * close_fail_5
 * 		ASSERTS: Cannot close stdout
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: close
 * 		FILES: syscalls.c
 * */
void close_fail_5(){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  int ret=close(1);

  if(ret==-1){
    TEST_OUTPUT("close_fail_5",PASS);
  } else{
    TEST_OUTPUT("close_fail_5",FAIL);
  }
}

/*
 * read_write
 * 		ASSERTS: Reads stdin, writes stdout
 * 		INPUTS: None
 * 		OUTPUTS: PASS - if ret = -1
							 FAIL - if ret != -1
 * 		SIDE EFFECTS: None
 * 		COVERAGE: close
 * 		FILES: syscalls.c
 * */
void read_write(){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
  unsigned char read_buf[128];

 	read(0,read_buf,128);

  write(1,read_buf,128);
}

/*
 * fd_file_read_test
 * 		ASSERTS: Reads a file using file descriptor
 * 		INPUTS: None
 * 		OUTPUTS: prints contents of file
 * 		SIDE EFFECTS: None
 * 		COVERAGE: open,read,close
 * 		FILES: syscalls.c
 * */
void fd_file_read_test(){
  pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

	int i;
	for(i = 2; i < 8; i++){
    pcb.fdt[i].flags = -1;
  }
  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
	printf("Reading frame1.txt\n");
  unsigned char read_buf[1000];
	int fd = open((uint8_t*)"frame1.txt");
	read(fd,read_buf,1000);
	printf("%s\n",read_buf);
	close(fd);
}

/*
 * fd_dir_read_test
 *		ASSERTS: read from . directory by listing all files in directory
 *		INPUTS: None
 *    OUTPUTS: prints all files in the . directory
 *		SIDE EFFECTS: None
 *		COVERAGE: open,read,close
 *		FILES: syscalls.c
 */
void fd_dir_read_test(){
	pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

	int i;
	for(i = 2; i < 8; i++){
    pcb.fdt[i].flags = -1;
  }
  /* Place pcb in kernel memory */
  memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));
	uint8_t buf[33];
  int32_t cnt;
  uint8_t dirName[]={'.'};
  int fd =open((const uint8_t*)dirName);
  while (0 != (cnt = read (fd, (void*)buf, 32))) {
    if(cnt==-1)break;
    puts((int8_t*)buf);
    printf("\n");
  }
  close(fd);
}

/*
 * rtc_system_call_test
 *		ASSERTS: RTC jump tables work
 *		INPUTS: none
 *    OUTPUTS: PASS or FAIL
 *		SIDE EFFECTS: Sets the rtc rate
 *		COVERAGE: Opening an RTC file type
 *		FILES: syscalls.c
 */
void rtc_system_call_test(){
	TEST_HEADER;

	pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

	int i;
	for(i = 2; i < 8; i++){
    pcb.fdt[i].flags = -1;
  }
	memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));

	int fd;
	int32_t buf[1] = {64};
	int count;

	printf("Opening RTC\n");
	if(-1 == (fd = open((uint8_t*)"rtc"))){
		TEST_OUTPUT("rtc open failure", FAIL);
	}
	rtc_test_flag = 1;
	while(count < 1000000000){
		count++;
	}
	printf("\nWrite called: 64hz\n");
	if(-1 == write(fd, buf, 4)){
		TEST_OUTPUT("rtc write failure", FAIL);
	}
	count = 0;
	while(count < 1000000000){
		count++;
	}
	printf("\n");
	buf[0] = 1;
	if(-1 == write(fd, buf, 4)){
		TEST_OUTPUT("rtc write failure", FAIL);
	}
	rtc_test_flag = 0;
	rtc_read_test_flag = 1;
	if(-1 == read(fd, buf, 4)){
		TEST_OUTPUT("rtc read failure", FAIL);
	}
	rtc_read_test_flag = 0;
	if(-1 == close(fd)){
		TEST_OUTPUT("rtc close failure", FAIL);
	}
	TEST_OUTPUT("rtc_system_call_test", PASS);
}

/*
 * pcb_overflow
 *		ASSERTS: Can't add more than 6 file descriptors
 *		INPUTS: none
 *    OUTPUTS: PASS or FAIL
 *		SIDE EFFECTS: Sets file descriptors to in use
 *		COVERAGE: Adding file descriptors
 *		FILES: syscalls.c
 */
void pcb_overflow(){
	TEST_HEADER;

	pcb_t pcb;
  pcb.fdt[0].jump_ptr = &stdin_table_1;
  pcb.fdt[1].jump_ptr = &stdout_table_1;
  pcb.fdt[0].flags = 1;
  pcb.fdt[1].flags = 1;

	int i;
	for(i = 2; i < 8; i++){
    pcb.fdt[i].flags = -1;
  }
	memcpy((void *)(EIGHT_MB - 1*0x2000), &pcb, sizeof(pcb));

	if(-1 == open((uint8_t*)"rtc")){
		TEST_OUTPUT("Opening file 1", FAIL);
	} else{
		TEST_OUTPUT("Opening file 1", PASS);
	}
	if(-1 == open((uint8_t*)"frame1.txt")){
		TEST_OUTPUT("Opening file 2", FAIL);
	} else{
		TEST_OUTPUT("Opening file 2", PASS);
	}
	if(-1 == open((uint8_t*)"frame0.txt")){
		TEST_OUTPUT("Opening file 3", FAIL);
	} else{
		TEST_OUTPUT("Opening file 3", PASS);
	}
	if(-1 == open((uint8_t*)"hello")){
		TEST_OUTPUT("Opening file 4", FAIL);
	} else{
		TEST_OUTPUT("Opening file 4", PASS);
	}
	if(-1 == open((uint8_t*)"shell")){
		TEST_OUTPUT("Opening file 5", FAIL);
	} else{
		TEST_OUTPUT("Opening file 5", PASS);
	}
	if(-1 == open((uint8_t*)".")){
		TEST_OUTPUT("Opening file 6", FAIL);
	} else{
		TEST_OUTPUT("Opening file 6", PASS);
	}
	if(-1 == open((uint8_t*)"verylargetextwithverylongname.tx")){
		TEST_OUTPUT("Opening file 7", FAIL);
	} else{
		TEST_OUTPUT("Opening file 7", PASS);
	}
	if(-1 == close(3)){
		TEST_OUTPUT("Closing file 1", FAIL);
	} else{
		TEST_OUTPUT("Closing file 1", PASS);
	}
	if(-1 == open((uint8_t*)"verylargetextwithverylongname.txt")){
		TEST_OUTPUT("Opening file 7", FAIL);
	} else {
		TEST_OUTPUT("Opening file 7", PASS);
	}
}

/* Checkpoint 4 tests */
#define USER_PROG  0x8000000
#define FOUR_MB    0x400000
/*
* Description- Test whats happens with an address less than 128 MB for vidmap
* Inputs- None
* Outputs- None
* Return Value- None
* Side Effects- None
*/
void vidmap_test_1(void){
	int ret_val=vidmap((uint8_t**)USER_PROG-0x1000);
	if(ret_val==-1){
		TEST_OUTPUT("vidmap test 1",PASS);
	} else{
		TEST_OUTPUT("vidmap test 1",FAIL);
	}
}

/*
* Description- Tests what happens when the adress is greater than 124 MB (the program executable)
* Inputs- None
* Outputs- None
* Return Value- None
* Side Effects- None
*/
void vidmap_test_2(void){
	int ret_val= vidmap((uint8_t**)USER_PROG+FOUR_MB+1);
	if(ret_val==-1){
		TEST_OUTPUT("vidmap test 2",PASS);
	} else{
		TEST_OUTPUT("vidmap test 2",FAIL);
	}
}
/*
* Description- Tests what happens when getargs is passed a NULL pointer
* Inputs- None
* Outputs- None
* Return Value- None
* Side Effects- None
*/

void getargs_test_1(void){
	int ret_val=getargs(NULL,69);
	if(ret_val==-1){
		TEST_OUTPUT("getargs test 1",PASS);
	} else{
		TEST_OUTPUT("getargs test 1",FAIL);
	}
}


/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	// TEST_OUTPUT("idt_test", idt_test());
	// launch your tests here
  // TEST_OUTPUT("idt_test2", idt_test_2());
  // TEST_OUTPUT("idt_test3", idt_test_3());
	// page_fault_test0();
	// page_fault_test1();
	// page_fault_test2();
	// page_fault_test3();
	// page_fault_test4();
	// page_fault_test5();
	// page_fault_test6();
  // divide_zero_test();
  // TEST_OUTPUT("page_directory_test", page_directory_test());
  // TEST_OUTPUT("page_table_test", page_table_test());

	/* Checkpoint 2 tests */
	//buffer_write();
	// file_index_test(0);
	// file_index_test(1);
	// dir_read_test();
  //uint8_t name[] = "frame0.txt";
	//read_file_test(name);
	//uint8_t name[] = "frame1.txt";
	//read_file_test(name);
	//uint8_t name[] = "verylargetextwithverylongname.tx";
	//read_file_test(name);
	//uint8_t name[] = "cat";
	//read_file_test(name);
	//uint8_t name[] = "fish";
	//read_file_test(name);
	//uint8_t name[] = "counter";
	//read_file_test(name);
	//uint8_t name[] = "grep";
	//read_file_test(name);
	//uint8_t name[] = "hello";
	//read_file_test(name);
	//uint8_t name[] = "ls";
	//read_file_test(name);
	//uint8_t name[] = "pingpong";
	//read_file_test(name);
	//uint8_t name[] = "shell";
	//read_file_test(name);
	//uint8_t name[] = "sigtest";
	//read_file_test(name);
	//uint8_t name[] = "syserr";
	//read_file_test(name);
	//uint8_t name[] = "testprint";
	//read_file_test(name);
	//rtc_write_test();
	// rtc_read_test();
	//rtc_open_test();
	//fread_fail_test();
	//dread_fail_test();
	//read_test();

	/* checkpoint 3 tests */
	/*
	open_null_test();
	open_test_fail();
	execute_fail_test();
	execute_fail_test_2();
	read_test_fail_1();
	read_test_fail_2();
	read_test_fail_3();
	read_test_fail_4();
	read_test_fail_5();
	write_test_fail_1();
	write_test_fail_2();
	write_test_fail_2();
	write_test_fail_3();
	write_test_fail_4();
	write_test_fail_5();
	write_test_fail_6();
	close_fail_1();
	close_fail_2();
	close_fail_3();
	close_fail_4();
	close_fail_5(); */
	// fd_file_read_test();
	// fd_dir_read_test();
	// rtc_system_call_test();
	// pcb_overflow();

	vidmap_test_1();
	vidmap_test_2();
	getargs_test_1();
}

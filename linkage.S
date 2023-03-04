#define ASM 1
#include "kb.h"
#include "rtc.h"
#include "pit.h"
#include "linkage.h"
#include "syscalls.h"
#include "x86_desc.h"

.text
.globl keyboard_linkage, rtc_linkage, pit_linkage, system_call_handler, context_switch

# Switch to user space
context_switch:
    # Point ds to user stack segment
    movw $USER_DS, %ax
    movw %ax, %ds
    pushf
    popl %eax
    orl $0x200, %eax # Set IF to 1 for user space
    pushl %eax
    popf
    pushl %ds # Push user ds
    pushl $USER_ESP # Push stack pointer of user program
    pushf # Push EFLAGS
    pushl $USER_CS  # Push user cs
    pushl %ecx # User eip
    iret  # Switch to user space

# System call linkage
system_call_handler:
    sti # Enable interrupts in kernel space
    # Save registers
    pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx
    subl $1, %eax # System call numbers are from 1-10, map them to 0-9 for jump table
		cmpl $9, %eax # Check if sys call number is greater than 9, if so it's invalid
		ja SYSCALL_FAIL
		cmpl $0, %eax # Check if sys call number is less than 0, if so it's invalid
		jb SYSCALL_FAIL
		call *system_call_table(, %eax, 4)  # Call the appropriate system call
    jmp SYSCALL_DONE
SYSCALL_FAIL:
    movl $-1, %eax # System call number is invalid
SYSCALL_DONE:
    # Restore registers
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp
		iret  # switch back to user space

# Jump table for system call
system_call_table:
	.long halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn

# Linkage for the keyboard handler
keyboard_linkage:
		# Save all registers
		pushl %eax
	  pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx

    call keyboard_interrupt_handler

		# Restore the registers
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp
    popl %eax
		# Return from interrupt
	  iret

# Linkage for the RTC handler
rtc_linkage:
		# Save all registers
		pushl %eax
	  pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx

    call rtc_interrupt_handler

		# Restore the registers
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp
    popl %eax
		# Return from interrupt
    iret

# Linkage for the PIT handler
pit_linkage:
		# Save all registers
		pushl %eax
	  pushl %ebp
    pushl %edi
    pushl %esi
    pushl %edx
    pushl %ecx
    pushl %ebx

    call pit_interrupt_handler

		# Restore the registers
    popl %ebx
    popl %ecx
    popl %edx
    popl %esi
    popl %edi
    popl %ebp
    popl %eax
		# Return from interrupt
    iret

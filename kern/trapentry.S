/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>



###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
	TRAPHANDLER_NOEC(DE,  T_DIVIDE);
	TRAPHANDLER_NOEC(DB,  T_DEBUG);
	TRAPHANDLER_NOEC(NMI, T_NMI);
	TRAPHANDLER_NOEC(BP,  T_BRKPT);
	TRAPHANDLER_NOEC(OF,  T_OFLOW);
	TRAPHANDLER_NOEC(BR,  T_BOUND);
	TRAPHANDLER_NOEC(UD,  T_ILLOP);
	TRAPHANDLER_NOEC(NM,  T_DEVICE);
	TRAPHANDLER(DF,  T_DBLFLT);
	# trap 9 is reserved
	TRAPHANDLER(TS, T_TSS);
	TRAPHANDLER(NP, T_SEGNP);
	TRAPHANDLER(SS, T_STACK);
	TRAPHANDLER(GP, T_GPFLT);
	TRAPHANDLER(PF, T_PGFLT);
	# trap 15 is reserved
	TRAPHANDLER_NOEC(MF, T_FPERR);
	TRAPHANDLER(AC, T_ALIGN);
	TRAPHANDLER_NOEC(MC, T_MCHK);
	TRAPHANDLER_NOEC(XF, T_SIMDERR);

	# trap 48 for system call
	TRAPHANDLER_NOEC(SYS_CALL, T_SYSCALL);

/*
 * Lab 3: Your code here for _alltraps
 */
_alltraps:
	pushw $0;                       # padding
	pushw %ds;                      # push data segment
	pushw $0;                       # padding
	pushw %es;                      # push data segment
	pushal;                         # push all registers
	# cannot directly operate on segment register
	movw $GD_KD, %ax;
	movw %ax, %ds;
	movw %ax, %es;
	# pass parameter and call function
	pushl %esp;
	call trap;

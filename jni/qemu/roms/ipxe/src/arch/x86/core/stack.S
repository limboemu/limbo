FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL )

	.arch i386

#ifdef __x86_64__
#define STACK_SIZE 8192
#else
#define STACK_SIZE 4096
#endif

/****************************************************************************
 * Internal stack
 ****************************************************************************
 */
	.section ".stack", "aw", @nobits
	.align 8
	.globl _stack
_stack:
	.space STACK_SIZE
	.globl _estack
_estack:

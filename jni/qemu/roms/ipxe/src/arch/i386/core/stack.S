FILE_LICENCE ( GPL2_OR_LATER )

	.arch i386

/****************************************************************************
 * Internal stack
 ****************************************************************************
 */
	.section ".stack", "aw", @nobits
	.align 8
	.globl _stack
_stack:
	.space 4096
	.globl _estack
_estack:

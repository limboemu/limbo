FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL )

#include <librm.h>

	.text
	.code16
	.arch i386
	.section ".prefix", "ax", @progbits

/* UNDI loader
 *
 * Called by an external program to load our PXE stack.
 */
	.globl	undiloader
undiloader:
	/* Save registers */
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	pushw	%ds
	pushw	%es
	pushw	%bx

	/* ROM segment address to %ds */
	pushw	%cs
	popw	%ds

	/* UNDI loader parameter structure address into %es:%di */
	movw	%sp, %bx
	movw	%ss:22(%bx), %di
	movw	%ss:24(%bx), %es

	/* Install to specified real-mode addresses */
	pushw	%di
	movw	%es:12(%di), %bx
	movw	%es:14(%di), %ax
	movl	image_source, %esi
	movl	decompress_to, %edi
	orl	$0xffffffff, %ebp	/* Allow arbitrary relocation */
	call	install_prealloc
	popw	%di

	/* Jump to .text16 segment */
	pushw	%ax
	pushw	$1f
	lret
	.section ".text16", "ax", @progbits
1:
	/* Call UNDI loader C code */
	virtcall pxe_loader_call

1:	/* Restore registers and return */
	popw	%bx
	popw	%es
	popw	%ds
	popl	%ebp
	popl	%edi
	popl	%esi
	lret

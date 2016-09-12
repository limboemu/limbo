/*
 * Copyright (C) 2008 Daniel Verkamp <daniel@drv.nu>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

FILE_LICENCE ( GPL2_OR_LATER )

#include "librm.h"

	.text

	.code32
	.globl com32_farcall_wrapper
com32_farcall_wrapper:
	movl	$VIRTUAL(com32_farcall), %eax
	jmp	com32_wrapper

	.code32
	.globl com32_cfarcall_wrapper
com32_cfarcall_wrapper:
	movl	$VIRTUAL(com32_cfarcall), %eax
	jmp	com32_wrapper

	.code32
	.globl com32_intcall_wrapper
com32_intcall_wrapper:
	movl	$VIRTUAL(com32_intcall), %eax
	/* fall through */

	.code32
com32_wrapper:

	/* Disable interrupts */
	cli

	/* Switch to internal virtual address space */
	call	_phys_to_virt

#ifdef __x86_64__

	.code64

	/* Preserve registers which are callee-save for COM32 (i386 API) */
	pushq	%rdi
	pushq	%rsi
	pushq	%rbp

	/* Extract parameters from stack */
	movl	28(%rsp), %edi
	movl	32(%rsp), %esi
	movl	36(%rsp), %edx

	/* Align stack pointer */
	movq	%rsp, %rbp
	andq	$~0x07, %rsp

	/* Call helper function */
	movslq	%eax, %rax
	call	*%rax

	/* Restore stack pointer */
	movq	%rbp, %rsp

	/* Restore registers */
	popq	%rbp
	popq	%rsi
	popq	%rdi

#else /* _x86_64 */

	/* Call helper function */
	pushl	12(%esp)
	pushl	12(%esp)
	pushl	12(%esp)
	call	*%eax
	addl	$12, %esp

#endif /* _x86_64 */

	/* Switch to external flat physical address space */
	call	_virt_to_phys
	.code32

	/* Reenable interrupts and return */
	sti
	ret

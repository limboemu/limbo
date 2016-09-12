/*
 * Copyright (C) 2016 Michael Brown <mbrown@fensystems.co.uk>.
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
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/** @file
 *
 * GDB exception handlers
 *
 */

/* Size of a register */
#define SIZEOF_REG 8

/* POSIX signal numbers for reporting traps to GDB */
#define SIGILL 4
#define SIGTRAP 5
#define SIGFPE 8
#define SIGSTKFLT 16

	.section ".text.gdbmach_interrupt", "ax", @progbits
	.code64

	.struct 0
/* Register dump created for GDB stub */
regs:
regs_rax:	.space	SIZEOF_REG
regs_rbx:	.space	SIZEOF_REG
regs_rcx:	.space	SIZEOF_REG
regs_rdx:	.space	SIZEOF_REG
regs_rsi:	.space	SIZEOF_REG
regs_rdi:	.space	SIZEOF_REG
regs_rbp:	.space	SIZEOF_REG
regs_rsp:	.space	SIZEOF_REG
regs_r8:	.space	SIZEOF_REG
regs_r9:	.space	SIZEOF_REG
regs_r10:	.space	SIZEOF_REG
regs_r11:	.space	SIZEOF_REG
regs_r12:	.space	SIZEOF_REG
regs_r13:	.space	SIZEOF_REG
regs_r14:	.space	SIZEOF_REG
regs_r15:	.space	SIZEOF_REG
regs_rip:	.space	SIZEOF_REG
regs_rflags:	.space	SIZEOF_REG
regs_cs:	.space	SIZEOF_REG
regs_ss:	.space	SIZEOF_REG
regs_ds:	.space	SIZEOF_REG
regs_es:	.space	SIZEOF_REG
regs_fs:	.space	SIZEOF_REG
regs_gs:	.space	SIZEOF_REG
regs_end:
/* GDB signal code */
gdb:
gdb_code:	.space	SIZEOF_REG
gdb_end:
/* Long-mode exception frame */
frame:
frame_rip:	.space	SIZEOF_REG
frame_cs:	.space	SIZEOF_REG
frame_rflags:	.space	SIZEOF_REG
frame_rsp:	.space	SIZEOF_REG
frame_ss:	.space	SIZEOF_REG
frame_end:
	.previous

	.globl	gdbmach_sigfpe
gdbmach_sigfpe:
	push	$SIGFPE
	jmp	gdbmach_interrupt

	.globl	gdbmach_sigtrap
gdbmach_sigtrap:
	push	$SIGTRAP
	jmp	gdbmach_interrupt

	.globl	gdbmach_sigstkflt
gdbmach_sigstkflt:
	push	$SIGSTKFLT
	jmp	gdbmach_interrupt

	.globl	gdbmach_sigill
gdbmach_sigill:
	push	$SIGILL
	jmp	gdbmach_interrupt

gdbmach_interrupt:

	/* Create register dump */
	pushq	%gs
	pushq	%fs
	pushq	$0		/* %es unused in long mode */
	pushq	$0		/* %ds unused in long mode */
	pushq	( frame_ss	- regs_ss	- SIZEOF_REG )(%rsp)
	pushq	( frame_cs	- regs_cs	- SIZEOF_REG )(%rsp)
	pushq	( frame_rflags	- regs_rflags	- SIZEOF_REG )(%rsp)
	pushq	( frame_rip	- regs_rip	- SIZEOF_REG )(%rsp)
	pushq	%r15
	pushq	%r14
	pushq	%r13
	pushq	%r12
	pushq	%r11
	pushq	%r10
	pushq	%r9
	pushq	%r8
	pushq	( frame_rsp	- regs_rsp	- SIZEOF_REG )(%rsp)
	pushq	%rbp
	pushq	%rdi
	pushq	%rsi
	pushq	%rdx
	pushq	%rcx
	pushq	%rbx
	pushq	%rax

	/* Call GDB stub exception handler */
	movq	gdb_code(%rsp), %rdi
	movq	%rsp, %rsi
	call	gdbmach_handler

	/* Restore from register dump */
	popq	%rax
	popq	%rbx
	popq	%rcx
	popq	%rdx
	popq	%rsi
	popq	%rdi
	popq	%rbp
	popq	( frame_rsp	- regs_rsp	- SIZEOF_REG )(%rsp)
	popq	%r8
	popq	%r9
	popq	%r10
	popq	%r11
	popq	%r12
	popq	%r13
	popq	%r14
	popq	%r15
	popq	( frame_rip	- regs_rip	- SIZEOF_REG )(%rsp)
	popq	( frame_rflags	- regs_rflags	- SIZEOF_REG )(%rsp)
	popq	( frame_cs	- regs_cs	- SIZEOF_REG )(%rsp)
	popq	( frame_ss	- regs_ss	- SIZEOF_REG )(%rsp)
	addq	$( regs_fs - regs_ds ), %rsp	/* skip %ds, %es */
	popq	%fs
	popq	%gs

	/* Skip code */
	addq	$( gdb_end - gdb_code ), %rsp	/* skip code */

	/* Return */
	iretq

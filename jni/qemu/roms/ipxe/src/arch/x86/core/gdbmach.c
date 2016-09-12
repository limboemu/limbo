/*
 * Copyright (C) 2008 Stefan Hajnoczi <stefanha@gmail.com>.
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

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <ipxe/uaccess.h>
#include <ipxe/gdbstub.h>
#include <librm.h>
#include <gdbmach.h>

/** @file
 *
 * GDB architecture-specific bits for x86
 *
 */

/** Number of hardware breakpoints */
#define NUM_HWBP 4

/** Debug register 7: Global breakpoint enable */
#define DR7_G( bp ) ( 2 << ( 2 * (bp) ) )

/** Debug register 7: Global exact breakpoint enable */
#define DR7_GE ( 1 << 9 )

/** Debug register 7: Break on data writes */
#define DR7_RWLEN_WRITE 0x11110000

/** Debug register 7: Break on data access */
#define DR7_RWLEN_ACCESS 0x33330000

/** Debug register 7: One-byte length */
#define DR7_RWLEN_1 0x00000000

/** Debug register 7: Two-byte length */
#define DR7_RWLEN_2 0x44440000

/** Debug register 7: Four-byte length */
#define DR7_RWLEN_4 0xcccc0000

/** Debug register 7: Eight-byte length */
#define DR7_RWLEN_8 0x88880000

/** Debug register 7: Breakpoint R/W and length mask */
#define DR7_RWLEN_MASK( bp ) ( 0xf0000 << ( 4 * (bp) ) )

/** Hardware breakpoint addresses (debug registers 0-3) */
static unsigned long dr[NUM_HWBP];

/** Active value of debug register 7 */
static unsigned long dr7 = DR7_GE;

/**
 * Update debug registers
 *
 */
static void gdbmach_update ( void ) {

	/* Set debug registers */
	__asm__ __volatile__ ( "mov %0, %%dr0" : : "r" ( dr[0] ) );
	__asm__ __volatile__ ( "mov %0, %%dr1" : : "r" ( dr[1] ) );
	__asm__ __volatile__ ( "mov %0, %%dr2" : : "r" ( dr[2] ) );
	__asm__ __volatile__ ( "mov %0, %%dr3" : : "r" ( dr[3] ) );
	__asm__ __volatile__ ( "mov %0, %%dr7" : : "r" ( dr7 ) );
}

/**
 * Find reusable or available hardware breakpoint
 *
 * @v addr		Linear address
 * @v rwlen		Control bits
 * @ret bp		Hardware breakpoint, or negative error
 */
static int gdbmach_find ( unsigned long addr, unsigned int rwlen ) {
	unsigned int i;
	int bp = -ENOENT;

	/* Look for a reusable or available breakpoint */
	for ( i = 0 ; i < NUM_HWBP ; i++ ) {

		/* If breakpoint is not enabled, then it is available */
		if ( ! ( dr7 & DR7_G ( i ) ) ) {
			bp = i;
			continue;
		}

		/* If breakpoint is enabled and has the same address
		 * and control bits, then reuse it.
		 */
		if ( ( dr[i] == addr ) &&
		     ( ( ( dr7 ^ rwlen ) & DR7_RWLEN_MASK ( i ) ) == 0 ) ) {
			bp = i;
			break;
		}
	}

	return bp;
}

/**
 * Set hardware breakpoint
 *
 * @v type		GDB breakpoint type
 * @v addr		Virtual address
 * @v len		Length
 * @v enable		Enable (not disable) breakpoint
 * @ret rc		Return status code
 */
int gdbmach_set_breakpoint ( int type, unsigned long addr, size_t len,
			     int enable ) {
	unsigned int rwlen;
	unsigned long mask;
	int bp;

	/* Parse breakpoint type */
	switch ( type ) {
	case GDBMACH_WATCH:
		rwlen = DR7_RWLEN_WRITE;
		break;
	case GDBMACH_AWATCH:
		rwlen = DR7_RWLEN_ACCESS;
		break;
	default:
		return -ENOTSUP;
	}

	/* Parse breakpoint length */
	switch ( len ) {
	case 1:
		rwlen |= DR7_RWLEN_1;
		break;
	case 2:
		rwlen |= DR7_RWLEN_2;
		break;
	case 4:
		rwlen |= DR7_RWLEN_4;
		break;
	case 8:
		rwlen |= DR7_RWLEN_8;
		break;
	default:
		return -ENOTSUP;
	}

	/* Convert to linear address */
	if ( sizeof ( physaddr_t ) <= sizeof ( uint32_t ) )
		addr = virt_to_phys ( ( void * ) addr );

	/* Find reusable or available hardware breakpoint */
	bp = gdbmach_find ( addr, rwlen );
	if ( bp < 0 )
		return ( enable ? -ENOBUFS : 0 );

	/* Configure this breakpoint */
	DBGC ( &dr[0], "GDB bp %d at %p+%zx type %d (%sabled)\n",
	       bp, ( ( void * ) addr ), len, type, ( enable ? "en" : "dis" ) );
	dr[bp] = addr;
	mask = DR7_RWLEN_MASK ( bp );
	dr7 = ( ( dr7 & ~mask ) | ( rwlen & mask ) );
	mask = DR7_G ( bp );
	dr7 &= ~mask;
	if ( enable )
		dr7 |= mask;

	/* Update debug registers */
	gdbmach_update();

	return 0;
}

/**
 * Handle exception
 *
 * @v signo		GDB signal number
 * @v regs		Register dump
 */
__asmcall void gdbmach_handler ( int signo, gdbreg_t *regs ) {
	unsigned long dr7_disabled = DR7_GE;
	unsigned long dr6_clear = 0;

	/* Temporarily disable breakpoints */
	__asm__ __volatile__ ( "mov %0, %%dr7\n" : : "r" ( dr7_disabled ) );

	/* Handle exception */
	DBGC ( &dr[0], "GDB signal %d\n", signo );
	DBGC2_HDA ( &dr[0], 0, regs, ( GDBMACH_NREGS * sizeof ( *regs ) ) );
	gdbstub_handler ( signo, regs );
	DBGC ( &dr[0], "GDB signal %d returning\n", signo );
	DBGC2_HDA ( &dr[0], 0, regs, ( GDBMACH_NREGS * sizeof ( *regs ) ) );

	/* Clear breakpoint status register */
	__asm__ __volatile__ ( "mov %0, %%dr6\n" : : "r" ( dr6_clear ) );

	/* Re-enable breakpoints */
	__asm__ __volatile__ ( "mov %0, %%dr7\n" : : "r" ( dr7 ) );
}

/**
 * CPU exception vectors
 *
 * Note that we cannot intercept anything from INT8 (double fault)
 * upwards, since these overlap by default with IRQ0-7.
 */
static void * gdbmach_vectors[] = {
	gdbmach_sigfpe,		/* Divide by zero */
	gdbmach_sigtrap,	/* Debug trap */
	NULL,			/* Non-maskable interrupt */
	gdbmach_sigtrap,	/* Breakpoint */
	gdbmach_sigstkflt,	/* Overflow */
	gdbmach_sigstkflt,	/* Bound range exceeded */
	gdbmach_sigill,		/* Invalid opcode */
};

/**
 * Initialise GDB
 */
void gdbmach_init ( void ) {
	unsigned int i;

	/* Hook CPU exception vectors */
	for ( i = 0 ; i < ( sizeof ( gdbmach_vectors ) /
			    sizeof ( gdbmach_vectors[0] ) ) ; i++ ) {
		if ( gdbmach_vectors[i] )
			set_interrupt_vector ( i, gdbmach_vectors[i] );
	}
}

/*
 * Copyright (C) 2016 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
 * TCP/IP checksum
 *
 */

#include <strings.h>
#include <ipxe/tcpip.h>

/** Alignment used by main checksumming loop */
#define TCPIP_CHKSUM_ALIGN 16

/** Number of steps in each iteration of the unrolled main checksumming loop */
#define TCPIP_CHKSUM_UNROLL 4

/**
 * Calculate continued TCP/IP checkum
 *
 * @v sum		Checksum of already-summed data, in network byte order
 * @v data		Data buffer
 * @v len		Length of data buffer
 * @ret sum		Updated checksum, in network byte order
 */
uint16_t tcpip_continue_chksum ( uint16_t sum, const void *data,
				 size_t len ) {
	intptr_t start;
	intptr_t end;
	intptr_t mid;
	unsigned int pre;
	unsigned int post;
	unsigned int first;
	uint64_t discard_low;
	uint64_t discard_high;

	/* Avoid potentially undefined shift operation */
	if ( len == 0 )
		return sum;

	/* Find maximally-aligned midpoint.  For short blocks of data,
	 * this may be aligned to fewer than 16 bytes.
	 */
	start = ( ( intptr_t ) data );
	end = ( start + len );
	mid = ( end &
		~( ( ~( 1UL << 63 ) ) >> ( 64 - flsl ( start ^ end ) ) ) );

	/* Calculate pre- and post-alignment lengths */
	pre = ( ( mid - start ) & ( TCPIP_CHKSUM_ALIGN - 1 ) );
	post = ( ( end - mid ) & ( TCPIP_CHKSUM_ALIGN - 1 ) );

	/* Calculate number of steps in first iteration of unrolled loop */
	first = ( ( ( len - pre - post ) / TCPIP_CHKSUM_ALIGN ) &
		  ( TCPIP_CHKSUM_UNROLL - 1 ) );

	/* Calculate checksum */
	__asm__ ( /* Invert sum */
		  "eor %w0, %w0, #0xffff\n\t"
		  /* Clear carry flag */
		  "cmn xzr, xzr\n\t"
		  /* Byteswap and sum pre-alignment byte, if applicable */
		  "tbz %w4, #0, 1f\n\t"
		  "ldrb %w2, [%1], #1\n\t"
		  "rev16 %w0, %w0\n\t"
		  "rev16 %w2, %w2\n\t"
		  "adcs %0, %0, %2\n\t"
		  "\n1:\n\t"
		  /* Sum pre-alignment halfword, if applicable */
		  "tbz %w4, #1, 1f\n\t"
		  "ldrh %w2, [%1], #2\n\t"
		  "adcs %0, %0, %2\n\t"
		  "\n1:\n\t"
		  /* Sum pre-alignment word, if applicable */
		  "tbz %w4, #2, 1f\n\t"
		  "ldr %w2, [%1], #4\n\t"
		  "adcs %0, %0, %2\n\t"
		  "\n1:\n\t"
		  /* Sum pre-alignment doubleword, if applicable */
		  "tbz %w4, #3, 1f\n\t"
		  "ldr %2, [%1], #8\n\t"
		  "adcs %0, %0, %2\n\t"
		  "\n1:\n\t"
		  /* Jump into unrolled (x4) main loop */
		  "adr %2, 2f\n\t"
		  "sub %2, %2, %5, lsl #3\n\t"
		  "sub %2, %2, %5, lsl #2\n\t"
		  "br %2\n\t"
		  "\n1:\n\t"
		  "ldp %2, %3, [%1], #16\n\t"
		  "adcs %0, %0, %2\n\t"
		  "adcs %0, %0, %3\n\t"
		  "ldp %2, %3, [%1], #16\n\t"
		  "adcs %0, %0, %2\n\t"
		  "adcs %0, %0, %3\n\t"
		  "ldp %2, %3, [%1], #16\n\t"
		  "adcs %0, %0, %2\n\t"
		  "adcs %0, %0, %3\n\t"
		  "ldp %2, %3, [%1], #16\n\t"
		  "adcs %0, %0, %2\n\t"
		  "adcs %0, %0, %3\n\t"
		  "\n2:\n\t"
		  "sub %2, %1, %6\n\t"
		  "cbnz %2, 1b\n\t"
		  /* Sum post-alignment doubleword, if applicable */
		  "tbz %w7, #3, 1f\n\t"
		  "ldr %2, [%1], #8\n\t"
		  "adcs %0, %0, %2\n\t"
		  "\n1:\n\t"
		  /* Sum post-alignment word, if applicable */
		  "tbz %w7, #2, 1f\n\t"
		  "ldr %w2, [%1], #4\n\t"
		  "adcs %0, %0, %2\n\t"
		  "\n1:\n\t"
		  /* Sum post-alignment halfword, if applicable */
		  "tbz %w7, #1, 1f\n\t"
		  "ldrh %w2, [%1], #2\n\t"
		  "adcs %0, %0, %2\n\t"
		  "\n1:\n\t"
		  /* Sum post-alignment byte, if applicable */
		  "tbz %w7, #0, 1f\n\t"
		  "ldrb %w2, [%1], #1\n\t"
		  "adcs %0, %0, %2\n\t"
		  "\n1:\n\t"
		  /* Fold down to a uint32_t plus carry flag */
		  "lsr %2, %0, #32\n\t"
		  "adcs %w0, %w0, %w2\n\t"
		  /* Fold down to a uint16_t plus carry in bit 16 */
		  "ubfm %2, %0, #0, #15\n\t"
		  "ubfm %3, %0, #16, #31\n\t"
		  "adc %w0, %w2, %w3\n\t"
		  /* Fold down to a uint16_t */
		  "tbz %w0, #16, 1f\n\t"
		  "mov %w2, #0xffff\n\t"
		  "sub %w0, %w0, %w2\n\t"
		  "tbz %w0, #16, 1f\n\t"
		  "sub %w0, %w0, %w2\n\t"
		  "\n1:\n\t"
		  /* Byteswap back, if applicable */
		  "tbz %w4, #0, 1f\n\t"
		  "rev16 %w0, %w0\n\t"
		  "\n1:\n\t"
		  /* Invert sum */
		  "eor %w0, %w0, #0xffff\n\t"
		  : "+r" ( sum ), "+r" ( data ), "=&r" ( discard_low ),
		    "=&r" ( discard_high )
		  : "r" ( pre ), "r" ( first ), "r" ( end - post ),
		    "r" ( post )
		  : "cc" );

	return sum;
}

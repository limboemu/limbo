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

/** @file
 *
 * Optimised string operations
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <string.h>

/**
 * Copy memory area
 *
 * @v dest		Destination address
 * @v src		Source address
 * @v len		Length
 * @ret dest		Destination address
 */
void arm64_memcpy ( void *dest, const void *src, size_t len ) {
	void *discard_dest;
	void *discard_end;
	const void *discard_src;
	size_t discard_offset;
	unsigned long discard_data;
	unsigned long discard_low;
	unsigned long discard_high;

	/* If length is too short for an "ldp"/"stp" instruction pair,
	 * then just copy individual bytes.
	 */
	if ( len < 16 ) {
		__asm__ __volatile__ ( "cbz %0, 2f\n\t"
				       "\n1:\n\t"
				       "sub %0, %0, #1\n\t"
				       "ldrb %w1, [%3, %0]\n\t"
				       "strb %w1, [%2, %0]\n\t"
				       "cbnz %0, 1b\n\t"
				       "\n2:\n\t"
				       : "=&r" ( discard_offset ),
					 "=&r" ( discard_data )
				       : "r" ( dest ), "r" ( src ), "0" ( len )
				       : "memory" );
		return;
	}

	/* Use "ldp"/"stp" to copy 16 bytes at a time: one initial
	 * potentially unaligned access, multiple destination-aligned
	 * accesses, one final potentially unaligned access.
	 */
	__asm__ __volatile__ ( "ldp %3, %4, [%1], #16\n\t"
			       "stp %3, %4, [%0], #16\n\t"
			       "and %3, %0, #15\n\t"
			       "sub %0, %0, %3\n\t"
			       "sub %1, %1, %3\n\t"
			       "bic %2, %5, #15\n\t"
			       "b 2f\n\t"
			       "\n1:\n\t"
			       "ldp %3, %4, [%1], #16\n\t"
			       "stp %3, %4, [%0], #16\n\t"
			       "\n2:\n\t"
			       "cmp %0, %2\n\t"
			       "bne 1b\n\t"
			       "ldp %3, %4, [%6, #-16]\n\t"
			       "stp %3, %4, [%5, #-16]\n\t"
			       : "=&r" ( discard_dest ),
				 "=&r" ( discard_src ),
				 "=&r" ( discard_end ),
				 "=&r" ( discard_low ),
				 "=&r" ( discard_high )
			       : "r" ( dest + len ), "r" ( src + len ),
				 "0" ( dest ), "1" ( src )
			       : "memory", "cc" );
}

/**
 * Zero memory region
 *
 * @v dest		Destination region
 * @v len		Length
 */
void arm64_bzero ( void *dest, size_t len ) {
	size_t discard_offset;
	void *discard_dest;
	void *discard_end;

	/* If length is too short for an "stp" instruction, then just
	 * zero individual bytes.
	 */
	if ( len < 16 ) {
		__asm__ __volatile__ ( "cbz %0, 2f\n\t"
				       "\n1:\n\t"
				       "sub %0, %0, #1\n\t"
				       "strb wzr, [%1, %0]\n\t"
				       "cbnz %0, 1b\n\t"
				       "\n2:\n\t"
				       : "=&r" ( discard_offset )
				       : "r" ( dest ), "0" ( len )
				       : "memory" );
		return;
	}

	/* Use "stp" to zero 16 bytes at a time: one initial
	 * potentially unaligned access, multiple aligned accesses,
	 * one final potentially unaligned access.
	 */
	__asm__ __volatile__ ( "stp xzr, xzr, [%0], #16\n\t"
			       "bic %0, %0, #15\n\t"
			       "bic %1, %2, #15\n\t"
			       "b 2f\n\t"
			       "\n1:\n\t"
			       "stp xzr, xzr, [%0], #16\n\t"
			       "\n2:\n\t"
			       "cmp %0, %1\n\t"
			       "bne 1b\n\t"
			       "stp xzr, xzr, [%2, #-16]\n\t"
			       : "=&r" ( discard_dest ),
				 "=&r" ( discard_end )
			       : "r" ( dest + len ), "0" ( dest )
			       : "memory", "cc" );
}

/**
 * Fill memory region
 *
 * @v dest		Destination region
 * @v len		Length
 * @v character		Fill character
 *
 * The unusual parameter order is to allow for more efficient
 * tail-calling to arm64_memset() when zeroing a region.
 */
void arm64_memset ( void *dest, size_t len, int character ) {
	size_t discard_offset;

	/* Use optimised zeroing code if applicable */
	if ( character == 0 ) {
		arm64_bzero ( dest, len );
		return;
	}

	/* Fill one byte at a time.  Calling memset() with a non-zero
	 * value is relatively rare and unlikely to be
	 * performance-critical.
	 */
	__asm__ __volatile__ ( "cbz %0, 2f\n\t"
			       "\n1:\n\t"
			       "sub %0, %0, #1\n\t"
			       "strb %w2, [%1, %0]\n\t"
			       "cbnz %0, 1b\n\t"
			       "\n2:\n\t"
			       : "=&r" ( discard_offset )
			       : "r" ( dest ), "r" ( character ), "0" ( len )
			       : "memory" );
}

/**
 * Copy (possibly overlapping) memory region forwards
 *
 * @v dest		Destination region
 * @v src		Source region
 * @v len		Length
 */
void arm64_memmove_forwards ( void *dest, const void *src, size_t len ) {
	void *discard_dest;
	const void *discard_src;
	unsigned long discard_data;

	/* Assume memmove() is not performance-critical, and perform a
	 * bytewise copy for simplicity.
	 */
	__asm__ __volatile__ ( "b 2f\n\t"
			       "\n1:\n\t"
			       "ldrb %w2, [%1], #1\n\t"
			       "strb %w2, [%0], #1\n\t"
			       "\n2:\n\t"
			       "cmp %0, %3\n\t"
			       "bne 1b\n\t"
			       : "=&r" ( discard_dest ),
				 "=&r" ( discard_src ),
				 "=&r" ( discard_data )
			       : "r" ( dest + len ), "0" ( dest ), "1" ( src )
			       : "memory" );
}

/**
 * Copy (possibly overlapping) memory region backwards
 *
 * @v dest		Destination region
 * @v src		Source region
 * @v len		Length
 */
void arm64_memmove_backwards ( void *dest, const void *src, size_t len ) {
	size_t discard_offset;
	unsigned long discard_data;

	/* Assume memmove() is not performance-critical, and perform a
	 * bytewise copy for simplicity.
	 */
	__asm__ __volatile__ ( "cbz %0, 2f\n\t"
			       "\n1:\n\t"
			       "sub %0, %0, #1\n\t"
			       "ldrb %w1, [%3, %0]\n\t"
			       "strb %w1, [%2, %0]\n\t"
			       "cbnz %0, 1b\n\t"
			       "\n2:\n\t"
			       : "=&r" ( discard_offset ),
				 "=&r" ( discard_data )
			       : "r" ( dest ), "r" ( src ), "0" ( len )
			       : "memory" );
}

/**
 * Copy (possibly overlapping) memory region
 *
 * @v dest		Destination region
 * @v src		Source region
 * @v len		Length
 */
void arm64_memmove ( void *dest, const void *src, size_t len ) {

	if ( dest <= src ) {
		arm64_memmove_forwards ( dest, src, len );
	} else {
		arm64_memmove_backwards ( dest, src, len );
	}
}

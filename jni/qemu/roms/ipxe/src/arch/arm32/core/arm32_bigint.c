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

#include <stdint.h>
#include <string.h>
#include <ipxe/bigint.h>

/** @file
 *
 * Big integer support
 */

/**
 * Multiply big integers
 *
 * @v multiplicand0	Element 0 of big integer to be multiplied
 * @v multiplier0	Element 0 of big integer to be multiplied
 * @v result0		Element 0 of big integer to hold result
 * @v size		Number of elements
 */
void bigint_multiply_raw ( const uint32_t *multiplicand0,
			   const uint32_t *multiplier0,
			   uint32_t *result0, unsigned int size ) {
	const bigint_t ( size ) __attribute__ (( may_alias )) *multiplicand =
		( ( const void * ) multiplicand0 );
	const bigint_t ( size ) __attribute__ (( may_alias )) *multiplier =
		( ( const void * ) multiplier0 );
	bigint_t ( size * 2 ) __attribute__ (( may_alias )) *result =
		( ( void * ) result0 );
	unsigned int i;
	unsigned int j;
	uint32_t multiplicand_element;
	uint32_t multiplier_element;
	uint32_t *result_elements;
	uint32_t discard_low;
	uint32_t discard_high;
	uint32_t discard_temp;

	/* Zero result */
	memset ( result, 0, sizeof ( *result ) );

	/* Multiply integers one element at a time */
	for ( i = 0 ; i < size ; i++ ) {
		multiplicand_element = multiplicand->element[i];
		for ( j = 0 ; j < size ; j++ ) {
			multiplier_element = multiplier->element[j];
			result_elements = &result->element[ i + j ];
			/* Perform a single multiply, and add the
			 * resulting double-element into the result,
			 * carrying as necessary.  The carry can
			 * never overflow beyond the end of the
			 * result, since:
			 *
			 *     a < 2^{n}, b < 2^{n} => ab < 2^{2n}
			 */
			__asm__ __volatile__ ( "umull %1, %2, %5, %6\n\t"
					       "ldr %3, [%0]\n\t"
					       "adds %3, %1\n\t"
					       "stmia %0!, {%3}\n\t"
					       "ldr %3, [%0]\n\t"
					       "adcs %3, %2\n\t"
					       "stmia %0!, {%3}\n\t"
					       "bcc 2f\n\t"
					       "\n1:\n\t"
					       "ldr %3, [%0]\n\t"
					       "adcs %3, #0\n\t"
					       "stmia %0!, {%3}\n\t"
					       "bcs 1b\n\t"
					       "\n2:\n\t"
					       : "+l" ( result_elements ),
						 "=l" ( discard_low ),
						 "=l" ( discard_high ),
						 "=l" ( discard_temp ),
						 "+m" ( *result )
					       : "l" ( multiplicand_element ),
						 "l" ( multiplier_element )
					       : "cc" );
		}
	}
}

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
 * Bit operations self-tests
 *
 */

/* Forcibly enable assertions */
#undef NDEBUG

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ipxe/bitops.h>
#include <ipxe/test.h>

/**
 * Perform bit operations self-tests
 *
 */
static void bitops_test_exec ( void ) {
	uint8_t bits[32];

	/* Initialise bits */
	memset ( bits, 0, sizeof ( bits ) );

	/* Test set_bit() */
	set_bit ( 0, bits );
	ok ( bits[0] == 0x01 );
	set_bit ( 17, bits );
	ok ( bits[2] == 0x02 );
	set_bit ( 22, bits );
	ok ( bits[2] == 0x42 );
	set_bit ( 22, bits );
	ok ( bits[2] == 0x42 );

	/* Test clear_bit() */
	clear_bit ( 0, bits );
	ok ( bits[0] == 0x00 );
	bits[5] = 0xff;
	clear_bit ( 42, bits );
	ok ( bits[5] == 0xfb );
	clear_bit ( 42, bits );
	ok ( bits[5] == 0xfb );
	clear_bit ( 44, bits );
	ok ( bits[5] == 0xeb );

	/* Test test_and_set_bit() */
	ok ( test_and_set_bit ( 0, bits ) == 0 );
	ok ( bits[0] == 0x01 );
	ok ( test_and_set_bit ( 0, bits ) != 0 );
	ok ( bits[0] == 0x01 );
	ok ( test_and_set_bit ( 69, bits ) == 0 );
	ok ( bits[8] == 0x20 );
	ok ( test_and_set_bit ( 69, bits ) != 0 );
	ok ( bits[8] == 0x20 );
	ok ( test_and_set_bit ( 69, bits ) != 0 );
	ok ( bits[8] == 0x20 );

	/* Test test_and_clear_bit() */
	ok ( test_and_clear_bit ( 0, bits ) != 0 );
	ok ( bits[0] == 0x00 );
	ok ( test_and_clear_bit ( 0, bits ) == 0 );
	ok ( bits[0] == 0x00 );
	bits[31] = 0xeb;
	ok ( test_and_clear_bit ( 255, bits ) != 0 );
	ok ( bits[31] == 0x6b );
	ok ( test_and_clear_bit ( 255, bits ) == 0 );
	ok ( bits[31] == 0x6b );
	ok ( test_and_clear_bit ( 255, bits ) == 0 );
	ok ( bits[31] == 0x6b );
}

/** Bit operations self-test */
struct self_test bitops_test __self_test = {
	.name = "bitops",
	.exec = bitops_test_exec,
};

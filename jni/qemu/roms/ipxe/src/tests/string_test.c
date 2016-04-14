/*
 * Copyright (C) 2012 Michael Brown <mbrown@fensystems.co.uk>.
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
 */

FILE_LICENCE ( GPL2_OR_LATER );

/** @file
 *
 * String self-tests
 *
 * memcpy() tests are handled separately
 */

/* Forcibly enable assertions */
#undef NDEBUG

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ipxe/test.h>

/**
 * Perform string self-tests
 *
 */
static void string_test_exec ( void ) {

	/* Test strlen() */
	ok ( strlen ( "" ) == 0 );
	ok ( strlen ( "Hello" ) == 5 );
	ok ( strlen ( "Hello world!" ) == 12 );
	ok ( strlen ( "Hello\0world!" ) == 5 );

	/* Test strnlen() */
	ok ( strnlen ( "", 0 ) == 0 );
	ok ( strnlen ( "", 10 ) == 0 );
	ok ( strnlen ( "Hello", 0 ) == 0 );
	ok ( strnlen ( "Hello", 3 ) == 3 );
	ok ( strnlen ( "Hello", 5 ) == 5 );
	ok ( strnlen ( "Hello", 16 ) == 5 );
	ok ( strnlen ( "Hello world!", 5 ) == 5 );
	ok ( strnlen ( "Hello world!", 11 ) == 11 );
	ok ( strnlen ( "Hello world!", 16 ) == 12 );

	/* Test strchr() */
	ok ( strchr ( "", 'a' ) == NULL );
	ok ( *(strchr ( "Testing", 'e' )) == 'e' );
	ok ( *(strchr ( "Testing", 'g' )) == 'g' );
	ok ( strchr ( "Testing", 'x' ) == NULL );

	/* Test strcmp() */
	ok ( strcmp ( "", "" ) == 0 );
	ok ( strcmp ( "Hello", "Hello" ) == 0 );
	ok ( strcmp ( "Hello", "hello" ) != 0 );
	ok ( strcmp ( "Hello", "Hello world!" ) != 0 );
	ok ( strcmp ( "Hello world!", "Hello" ) != 0 );

	/* Test strncmp() */
	ok ( strncmp ( "", "", 0 ) == 0 );
	ok ( strncmp ( "", "", 15 ) == 0 );
	ok ( strncmp ( "Goodbye", "Goodbye", 16 ) == 0 );
	ok ( strncmp ( "Goodbye", "Hello", 16 ) != 0 );
	ok ( strncmp ( "Goodbye", "Goodbye world", 32 ) != 0 );
	ok ( strncmp ( "Goodbye", "Goodbye world", 7 ) == 0 );

	/* Test memcmp() */
	ok ( memcmp ( "", "", 0 ) == 0 );
	ok ( memcmp ( "Foo", "Foo", 3 ) == 0 );
	ok ( memcmp ( "Foo", "Bar", 3 ) != 0 );

	/* Test memset() */
	{
		static uint8_t test[7] = { '>', 1, 1, 1, 1, 1, '<' };
		static const uint8_t expected[7] = { '>', 0, 0, 0, 0, 0, '<' };
		memset ( ( test + 1 ), 0, ( sizeof ( test ) - 2 ) );
		ok ( memcmp ( test, expected, sizeof ( test ) ) == 0 );
	}
	{
		static uint8_t test[4] = { '>', 0, 0, '<' };
		static const uint8_t expected[4] = { '>', 0xeb, 0xeb, '<' };
		memset ( ( test + 1 ), 0xeb, ( sizeof ( test ) - 2 ) );
		ok ( memcmp ( test, expected, sizeof ( test ) ) == 0 );
	}

	/* Test memmove() */
	{
		static uint8_t test[11] =
			{ '>', 1, 2, 3, 4, 5, 6, 7, 8, 9, '<' };
		static const uint8_t expected[11] =
			{ '>', 3, 4, 5, 6, 7, 8, 7, 8, 9, '<' };
		memmove ( ( test + 1 ), ( test + 3 ), 6 );
		ok ( memcmp ( test, expected, sizeof ( test ) ) == 0 );
	}
	{
		static uint8_t test[12] =
			{ '>', 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, '<' };
		static const uint8_t expected[12] =
			{ '>', 1, 2, 3, 4, 5, 1, 2, 3, 4, 5, '<' };
		memmove ( ( test + 6 ), ( test + 1 ), 5 );
		ok ( memcmp ( test, expected, sizeof ( test ) ) == 0 );
	}

	/* Test memswap() */
	{
		static uint8_t test[8] =
			{ '>', 1, 2, 3, 7, 8, 9, '<' };
		static const uint8_t expected[8] =
			{ '>', 7, 8, 9, 1, 2, 3, '<' };
		memswap ( ( test + 1 ), ( test + 4 ), 3 );
		ok ( memcmp ( test, expected, sizeof ( test ) ) == 0 );
	}

	/* Test strdup() */
	{
		const char *orig = "testing testing";
		char *dup = strdup ( orig );
		ok ( dup != NULL );
		ok ( dup != orig );
		ok ( strcmp ( dup, orig ) == 0 );
		free ( dup );
	}

	/* Test strndup() */
	{
		const char *normal = "testing testing";
		const char unterminated[6] = { 'h', 'e', 'l', 'l', 'o', '!' };
		char *dup;
		dup = strndup ( normal, 32 );
		ok ( dup != NULL );
		ok ( dup != normal );
		ok ( strcmp ( dup, normal ) == 0 );
		free ( dup );
		dup = strndup ( normal, 4 );
		ok ( dup != NULL );
		ok ( strcmp ( dup, "test" ) == 0 );
		free ( dup );
		dup = strndup ( unterminated, 5 );
		ok ( dup != NULL );
		ok ( strcmp ( dup, "hello" ) == 0 );
		free ( dup );
	}
}

/** String self-test */
struct self_test string_test __self_test = {
	.name = "string",
	.exec = string_test_exec,
};

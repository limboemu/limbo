/*
 * Copyright (C) 2010 Michael Brown <mbrown@fensystems.co.uk>.
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

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <ipxe/base16.h>

/** @file
 *
 * Base16 encoding
 *
 */

/**
 * Base16-encode data
 *
 * @v raw		Raw data
 * @v len		Length of raw data
 * @v encoded		Buffer for encoded string
 *
 * The buffer must be the correct length for the encoded string.  Use
 * something like
 *
 *     char buf[ base16_encoded_len ( len ) + 1 ];
 *
 * (the +1 is for the terminating NUL) to provide a buffer of the
 * correct size.
 */
void base16_encode ( const uint8_t *raw, size_t len, char *encoded ) {
	const uint8_t *raw_bytes = raw;
	char *encoded_bytes = encoded;
	size_t remaining = len;

	/* Encode each byte */
	for ( ; remaining-- ; encoded_bytes += 2 ) {
		sprintf ( encoded_bytes, "%02x", *(raw_bytes++) );
	}

	/* Ensure terminating NUL exists even if length was zero */
	*encoded_bytes = '\0';

	DBG ( "Base16-encoded to \"%s\":\n", encoded );
	DBG_HDA ( 0, raw, len );
	assert ( strlen ( encoded ) == base16_encoded_len ( len ) );
}

/**
 * Decode hexadecimal string
 *
 * @v encoded		Encoded string
 * @v separator		Byte separator character, or 0 for no separator
 * @v data		Buffer
 * @v len		Length of buffer
 * @ret len		Length of data, or negative error
 */
int hex_decode ( const char *encoded, char separator, void *data, size_t len ) {
	uint8_t *out = data;
	unsigned int count = 0;
	unsigned int sixteens;
	unsigned int units;

	while ( *encoded ) {

		/* Check separator, if applicable */
		if ( count && separator && ( ( *(encoded++) != separator ) ) )
			return -EINVAL;

		/* Extract digits.  Note that either digit may be NUL,
		 * which would be interpreted as an invalid value by
		 * strtoul_charval(); there is therefore no need for an
		 * explicit end-of-string check.
		 */
		sixteens = strtoul_charval ( *(encoded++) );
		if ( sixteens >= 16 )
			return -EINVAL;
		units = strtoul_charval ( *(encoded++) );
		if ( units >= 16 )
			return -EINVAL;

		/* Store result */
		if ( count < len )
			out[count] = ( ( sixteens << 4 ) | units );
		count++;

	}
	return count;
}

/**
 * Base16-decode data
 *
 * @v encoded		Encoded string
 * @v raw		Raw data
 * @ret len		Length of raw data, or negative error
 *
 * The buffer must be large enough to contain the decoded data.  Use
 * something like
 *
 *     char buf[ base16_decoded_max_len ( encoded ) ];
 *
 * to provide a buffer of the correct size.
 */
int base16_decode ( const char *encoded, uint8_t *raw ) {
	int len;

	len = hex_decode ( encoded, 0, raw, -1UL );
	if ( len < 0 )
		return len;

	DBG ( "Base16-decoded \"%s\" to:\n", encoded );
	DBG_HDA ( 0, raw, len );
	assert ( len <= ( int ) base16_decoded_max_len ( encoded ) );

	return len;
}

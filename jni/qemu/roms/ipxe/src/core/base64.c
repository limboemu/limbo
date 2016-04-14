/*
 * Copyright (C) 2009 Michael Brown <mbrown@fensystems.co.uk>.
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
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <assert.h>
#include <ipxe/base64.h>

/** @file
 *
 * Base64 encoding
 *
 */

static const char base64[64] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * Base64-encode data
 *
 * @v raw		Raw data
 * @v len		Length of raw data
 * @v encoded		Buffer for encoded string
 *
 * The buffer must be the correct length for the encoded string.  Use
 * something like
 *
 *     char buf[ base64_encoded_len ( len ) + 1 ];
 *
 * (the +1 is for the terminating NUL) to provide a buffer of the
 * correct size.
 */
void base64_encode ( const uint8_t *raw, size_t len, char *encoded ) {
	const uint8_t *raw_bytes = ( ( const uint8_t * ) raw );
	uint8_t *encoded_bytes = ( ( uint8_t * ) encoded );
	size_t raw_bit_len = ( 8 * len );
	unsigned int bit;
	unsigned int byte;
	unsigned int shift;
	unsigned int tmp;

	for ( bit = 0 ; bit < raw_bit_len ; bit += 6 ) {
		byte = ( bit / 8 );
		shift = ( bit % 8 );
		tmp = ( raw_bytes[byte] << shift );
		if ( ( byte + 1 ) < len )
			tmp |= ( raw_bytes[ byte + 1 ] >> ( 8 - shift ) );
		tmp = ( ( tmp >> 2 ) & 0x3f );
		*(encoded_bytes++) = base64[tmp];
	}
	for ( ; ( bit % 8 ) != 0 ; bit += 6 )
		*(encoded_bytes++) = '=';
	*(encoded_bytes++) = '\0';

	DBG ( "Base64-encoded to \"%s\":\n", encoded );
	DBG_HDA ( 0, raw, len );
	assert ( strlen ( encoded ) == base64_encoded_len ( len ) );
}

/**
 * Base64-decode string
 *
 * @v encoded		Encoded string
 * @v raw		Raw data
 * @ret len		Length of raw data, or negative error
 *
 * The buffer must be large enough to contain the decoded data.  Use
 * something like
 *
 *     char buf[ base64_decoded_max_len ( encoded ) ];
 *
 * to provide a buffer of the correct size.
 */
int base64_decode ( const char *encoded, uint8_t *raw ) {
	const uint8_t *encoded_bytes = ( ( const uint8_t * ) encoded );
	uint8_t *raw_bytes = ( ( uint8_t * ) raw );
	uint8_t encoded_byte;
	char *match;
	int decoded;
	unsigned int bit = 0;
	unsigned int pad_count = 0;
	size_t len;

	/* Zero the raw data */
	memset ( raw, 0, base64_decoded_max_len ( encoded ) );

	/* Decode string */
	while ( ( encoded_byte = *(encoded_bytes++) ) ) {

		/* Ignore whitespace characters */
		if ( isspace ( encoded_byte ) )
			continue;

		/* Process pad characters */
		if ( encoded_byte == '=' ) {
			if ( pad_count >= 2 ) {
				DBG ( "Base64-encoded string \"%s\" has too "
				      "many pad characters\n", encoded );
				return -EINVAL;
			}
			pad_count++;
			bit -= 2; /* unused_bits = ( 2 * pad_count ) */
			continue;
		}
		if ( pad_count ) {
			DBG ( "Base64-encoded string \"%s\" has invalid pad "
			      "sequence\n", encoded );
			return -EINVAL;
		}

		/* Process normal characters */
		match = strchr ( base64, encoded_byte );
		if ( ! match ) {
			DBG ( "Base64-encoded string \"%s\" contains invalid "
			      "character '%c'\n", encoded, encoded_byte );
			return -EINVAL;
		}
		decoded = ( match - base64 );

		/* Add to raw data */
		decoded <<= 2;
		raw_bytes[ bit / 8 ] |= ( decoded >> ( bit % 8 ) );
		raw_bytes[ bit / 8 + 1 ] |= ( decoded << ( 8 - ( bit % 8 ) ) );
		bit += 6;
	}

	/* Check that we decoded a whole number of bytes */
	if ( ( bit % 8 ) != 0 ) {
		DBG ( "Base64-encoded string \"%s\" has invalid bit length "
		      "%d\n", encoded, bit );
		return -EINVAL;
	}
	len = ( bit / 8 );

	DBG ( "Base64-decoded \"%s\" to:\n", encoded );
	DBG_HDA ( 0, raw, len );
	assert ( len <= base64_decoded_max_len ( encoded ) );

	/* Return length in bytes */
	return ( len );
}

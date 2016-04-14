#ifndef _IPXE_BASE64_H
#define _IPXE_BASE64_H

/** @file
 *
 * Base64 encoding
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <string.h>

/**
 * Calculate length of base64-encoded data
 *
 * @v raw_len		Raw data length
 * @ret encoded_len	Encoded string length (excluding NUL)
 */
static inline size_t base64_encoded_len ( size_t raw_len ) {
	return ( ( ( raw_len + 3 - 1 ) / 3 ) * 4 );
}

/**
 * Calculate maximum length of base64-decoded string
 *
 * @v encoded		Encoded string
 * @v max_raw_len	Maximum length of raw data
 *
 * Note that the exact length of the raw data cannot be known until
 * the string is decoded.
 */
static inline size_t base64_decoded_max_len ( const char *encoded ) {
	return ( ( ( strlen ( encoded ) + 4 - 1 ) / 4 ) * 3 );
}

extern void base64_encode ( const uint8_t *raw, size_t len, char *encoded );
extern int base64_decode ( const char *encoded, uint8_t *raw );

#endif /* _IPXE_BASE64_H */

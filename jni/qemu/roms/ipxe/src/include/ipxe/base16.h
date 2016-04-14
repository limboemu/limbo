#ifndef _IPXE_BASE16_H
#define _IPXE_BASE16_H

/** @file
 *
 * Base16 encoding
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <string.h>

/**
 * Calculate length of base16-encoded data
 *
 * @v raw_len		Raw data length
 * @ret encoded_len	Encoded string length (excluding NUL)
 */
static inline size_t base16_encoded_len ( size_t raw_len ) {
	return ( 2 * raw_len );
}

/**
 * Calculate maximum length of base16-decoded string
 *
 * @v encoded		Encoded string
 * @v max_raw_len	Maximum length of raw data
 */
static inline size_t base16_decoded_max_len ( const char *encoded ) {
	return ( ( strlen ( encoded ) + 1 ) / 2 );
}

extern void base16_encode ( const uint8_t *raw, size_t len, char *encoded );
extern int hex_decode ( const char *string, char separator, void *data,
			size_t len );
extern int base16_decode ( const char *encoded, uint8_t *raw );

#endif /* _IPXE_BASE16_H */

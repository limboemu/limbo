#ifndef _BITS_BYTESWAP_H
#define _BITS_BYTESWAP_H

/** @file
 *
 * Byte-order swapping functions
 *
 */

#include <stdint.h>

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

static inline __attribute__ (( always_inline, const )) uint16_t
__bswap_variable_16 ( uint16_t x ) {
	__asm__ ( "rev16 %0, %1" : "=l" ( x ) : "l" ( x ) );
	return x;
}

static inline __attribute__ (( always_inline )) void
__bswap_16s ( uint16_t *x ) {
	*x = __bswap_variable_16 ( *x );
}

static inline __attribute__ (( always_inline, const )) uint32_t
__bswap_variable_32 ( uint32_t x ) {
	__asm__ ( "rev %0, %1" : "=l" ( x ) : "l" ( x ) );
	return x;
}

static inline __attribute__ (( always_inline )) void
__bswap_32s ( uint32_t *x ) {
	*x = __bswap_variable_32 ( *x );
}

static inline __attribute__ (( always_inline, const )) uint64_t
__bswap_variable_64 ( uint64_t x ) {
	uint32_t in_high = ( x >> 32 );
	uint32_t in_low = ( x & 0xffffffffUL );
	uint32_t out_high = __bswap_variable_32 ( in_low );
	uint32_t out_low = __bswap_variable_32 ( in_high );

	return ( ( ( ( uint64_t ) out_high ) << 32 ) |
		 ( ( uint64_t ) out_low ) );
}

static inline __attribute__ (( always_inline )) void
__bswap_64s ( uint64_t *x ) {
	*x = __bswap_variable_64 ( *x );
}

#endif

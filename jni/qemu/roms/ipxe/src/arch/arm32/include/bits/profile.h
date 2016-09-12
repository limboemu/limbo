#ifndef _BITS_PROFILE_H
#define _BITS_PROFILE_H

/** @file
 *
 * Profiling
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>

/**
 * Get profiling timestamp
 *
 * @ret timestamp	Timestamp
 */
static inline __attribute__ (( always_inline )) uint64_t
profile_timestamp ( void ) {
	uint32_t cycles;

	/* Read cycle counter */
	__asm__ __volatile__ ( "mcr p15, 0, %1, c9, c12, 0\n\t"
			       "mrc p15, 0, %0, c9, c13, 0\n\t"
			       : "=r" ( cycles ) : "r" ( 1 ) );
	return cycles;
}

#endif /* _BITS_PROFILE_H */

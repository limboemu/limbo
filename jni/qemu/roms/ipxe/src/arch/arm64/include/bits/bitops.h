#ifndef _BITS_BITOPS_H
#define _BITS_BITOPS_H

/** @file
 *
 * ARM bit operations
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>

/**
 * Test and set bit atomically
 *
 * @v bit		Bit to set
 * @v bits		Bit field
 * @ret old		Old value of bit (zero or non-zero)
 */
static inline __attribute__ (( always_inline )) int
test_and_set_bit ( unsigned int bit, volatile void *bits ) {
	unsigned int index = ( bit / 64 );
	unsigned int offset = ( bit % 64 );
	volatile uint64_t *qword = ( ( ( volatile uint64_t * ) bits ) + index );
	uint64_t mask = ( 1UL << offset );
	uint64_t old;
	uint64_t new;
	uint32_t flag;

	__asm__ __volatile__ ( "\n1:\n\t"
			       "ldxr %0, %3\n\t"
			       "orr %1, %0, %4\n\t"
			       "stxr %w2, %1, %3\n\t"
			       "tst %w2, %w2\n\t"
			       "bne 1b\n\t"
			       : "=&r" ( old ), "=&r" ( new ), "=&r" ( flag ),
				 "+Q" ( *qword )
			       : "r" ( mask )
			       : "cc" );

	return ( !! ( old & mask ) );
}

/**
 * Test and clear bit atomically
 *
 * @v bit		Bit to set
 * @v bits		Bit field
 * @ret old		Old value of bit (zero or non-zero)
 */
static inline __attribute__ (( always_inline )) int
test_and_clear_bit ( unsigned int bit, volatile void *bits ) {
	unsigned int index = ( bit / 64 );
	unsigned int offset = ( bit % 64 );
	volatile uint64_t *qword = ( ( ( volatile uint64_t * ) bits ) + index );
	uint64_t mask = ( 1UL << offset );
	uint64_t old;
	uint64_t new;
	uint32_t flag;

	__asm__ __volatile__ ( "\n1:\n\t"
			       "ldxr %0, %3\n\t"
			       "bic %1, %0, %4\n\t"
			       "stxr %w2, %1, %3\n\t"
			       "tst %w2, %w2\n\t"
			       "bne 1b\n\t"
			       : "=&r" ( old ), "=&r" ( new ), "=&r" ( flag ),
				 "+Q" ( *qword )
			       : "r" ( mask )
			       : "cc" );

	return ( !! ( old & mask ) );
}

/**
 * Set bit atomically
 *
 * @v bit		Bit to set
 * @v bits		Bit field
 */
static inline __attribute__ (( always_inline )) void
set_bit ( unsigned int bit, volatile void *bits ) {

	test_and_set_bit ( bit, bits );
}

/**
 * Clear bit atomically
 *
 * @v bit		Bit to set
 * @v bits		Bit field
 */
static inline __attribute__ (( always_inline )) void
clear_bit ( unsigned int bit, volatile void *bits ) {

	test_and_clear_bit ( bit, bits );
}

#endif /* _BITS_BITOPS_H */

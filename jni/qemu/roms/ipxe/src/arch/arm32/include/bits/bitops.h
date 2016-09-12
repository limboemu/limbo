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
	unsigned int index = ( bit / 32 );
	unsigned int offset = ( bit % 32 );
	volatile uint32_t *dword = ( ( ( volatile uint32_t * ) bits ) + index );
	uint32_t mask = ( 1UL << offset );
	uint32_t old;
	uint32_t new;
	uint32_t flag;

	__asm__ __volatile__ ( "\n1:\n\t"
			       "ldrex %0, %3\n\t"
			       "orr %1, %0, %4\n\t"
			       "strex %2, %1, %3\n\t"
			       "tst %2, %2\n\t"
			       "bne 1b\n\t"
			       : "=&r" ( old ), "=&r" ( new ), "=&l" ( flag ),
				 "+Q" ( *dword )
			       : "r" ( mask )
			       : "cc" );

	return ( old & mask );
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
	unsigned int index = ( bit / 32 );
	unsigned int offset = ( bit % 32 );
	volatile uint32_t *dword = ( ( ( volatile uint32_t * ) bits ) + index );
	uint32_t mask = ( 1UL << offset );
	uint32_t old;
	uint32_t new;
	uint32_t flag;

	__asm__ __volatile__ ( "\n1:\n\t"
			       "ldrex %0, %3\n\t"
			       "bic %1, %0, %4\n\t"
			       "strex %2, %1, %3\n\t"
			       "tst %2, %2\n\t"
			       "bne 1b\n\t"
			       : "=&r" ( old ), "=&r" ( new ), "=&l" ( flag ),
				 "+Q" ( *dword )
			       : "r" ( mask )
			       : "cc" );

	return ( old & mask );
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

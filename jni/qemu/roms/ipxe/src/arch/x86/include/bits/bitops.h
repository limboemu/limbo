#ifndef _BITS_BITOPS_H
#define _BITS_BITOPS_H

/** @file
 *
 * x86 bit operations
 *
 * We perform atomic bit set and bit clear operations using "lock bts"
 * and "lock btr".  We use the output constraint to inform the
 * compiler that any memory from the start of the bit field up to and
 * including the byte containing the bit may be modified.  (This is
 * overkill but shouldn't matter in practice since we're unlikely to
 * subsequently read other bits from the same bit field.)
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>

/**
 * Set bit atomically
 *
 * @v bit		Bit to set
 * @v bits		Bit field
 */
static inline __attribute__ (( always_inline )) void
set_bit ( unsigned int bit, volatile void *bits ) {
	volatile struct {
		uint8_t byte[ ( bit / 8 ) + 1 ];
	} *bytes = bits;

	__asm__ __volatile__ ( "lock bts %1, %0"
			       : "+m" ( *bytes ) : "Ir" ( bit ) );
}

/**
 * Clear bit atomically
 *
 * @v bit		Bit to set
 * @v bits		Bit field
 */
static inline __attribute__ (( always_inline )) void
clear_bit ( unsigned int bit, volatile void *bits ) {
	volatile struct {
		uint8_t byte[ ( bit / 8 ) + 1 ];
	} *bytes = bits;

	__asm__ __volatile__ ( "lock btr %1, %0"
			       : "+m" ( *bytes ) : "Ir" ( bit ) );
}

/**
 * Test and set bit atomically
 *
 * @v bit		Bit to set
 * @v bits		Bit field
 * @ret old		Old value of bit (zero or non-zero)
 */
static inline __attribute__ (( always_inline )) int
test_and_set_bit ( unsigned int bit, volatile void *bits ) {
	volatile struct {
		uint8_t byte[ ( bit / 8 ) + 1 ];
	} *bytes = bits;
	int old;

	__asm__ __volatile__ ( "lock bts %2, %0\n\t"
			       "sbb %1, %1\n\t"
			       : "+m" ( *bytes ), "=r"  ( old )
			       : "Ir" ( bit ) );
	return old;
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
	volatile struct {
		uint8_t byte[ ( bit / 8 ) + 1 ];
	} *bytes = bits;
	int old;

	__asm__ __volatile__ ( "lock btr %2, %0\n\t"
			       "sbb %1, %1\n\t"
			       : "+m" ( *bytes ), "=r"  ( old )
			       : "Ir" ( bit ) );
	return old;
}

#endif /* _BITS_BITOPS_H */

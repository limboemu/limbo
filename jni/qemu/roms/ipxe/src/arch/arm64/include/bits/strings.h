#ifndef _BITS_STRINGS_H
#define _BITS_STRINGS_H

/** @file
 *
 * String functions
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/**
 * Find first (i.e. least significant) set bit
 *
 * @v value		Value
 * @ret lsb		Least significant bit set in value (LSB=1), or zero
 */
static inline __attribute__ (( always_inline )) int __ffsll ( long long value ){
	unsigned long long bits = value;
	unsigned long long lsb;
	unsigned int lz;

	/* Extract least significant set bit */
	lsb = ( bits & -bits );

	/* Count number of leading zeroes before LSB */
	__asm__ ( "clz %0, %1" : "=r" ( lz ) : "r" ( lsb ) );

	return ( 64 - lz );
}

/**
 * Find first (i.e. least significant) set bit
 *
 * @v value		Value
 * @ret lsb		Least significant bit set in value (LSB=1), or zero
 */
static inline __attribute__ (( always_inline )) int __ffsl ( long value ) {

	return __ffsll ( value );
}

/**
 * Find last (i.e. most significant) set bit
 *
 * @v value		Value
 * @ret msb		Most significant bit set in value (LSB=1), or zero
 */
static inline __attribute__ (( always_inline )) int __flsll ( long long value ){
	unsigned int lz;

	/* Count number of leading zeroes */
	__asm__ ( "clz %0, %1" : "=r" ( lz ) : "r" ( value ) );

	return ( 64 - lz );
}

/**
 * Find last (i.e. most significant) set bit
 *
 * @v value		Value
 * @ret msb		Most significant bit set in value (LSB=1), or zero
 */
static inline __attribute__ (( always_inline )) int __flsl ( long value ) {

	return __flsll ( value );
}

#endif /* _BITS_STRINGS_H */

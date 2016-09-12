#ifndef BITS_STRING_H
#define BITS_STRING_H

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/** @file
 *
 * String functions
 *
 */

extern void arm64_bzero ( void *dest, size_t len );
extern void arm64_memset ( void *dest, size_t len, int character );
extern void arm64_memcpy ( void *dest, const void *src, size_t len );
extern void arm64_memmove_forwards ( void *dest, const void *src, size_t len );
extern void arm64_memmove_backwards ( void *dest, const void *src, size_t len );
extern void arm64_memmove ( void *dest, const void *src, size_t len );

/**
 * Fill memory region
 *
 * @v dest		Destination region
 * @v character		Fill character
 * @v len		Length
 * @ret dest		Destination region
 */
static inline __attribute__ (( always_inline )) void *
memset ( void *dest, int character, size_t len ) {

	/* Allow gcc to generate inline "stX xzr" instructions for
	 * small, constant lengths.
	 */
	if ( __builtin_constant_p ( character ) && ( character == 0 ) &&
	     __builtin_constant_p ( len ) && ( len <= 64 ) ) {
		__builtin_memset ( dest, 0, len );
		return dest;
	}

	/* For zeroing larger or non-constant lengths, use the
	 * optimised variable-length zeroing code.
	 */
	if ( __builtin_constant_p ( character ) && ( character == 0 ) ) {
		arm64_bzero ( dest, len );
		return dest;
	}

	/* Not necessarily zeroing: use basic variable-length code */
	arm64_memset ( dest, len, character );
	return dest;
}

/**
 * Copy memory region
 *
 * @v dest		Destination region
 * @v src		Source region
 * @v len		Length
 * @ret dest		Destination region
 */
static inline __attribute__ (( always_inline )) void *
memcpy ( void *dest, const void *src, size_t len ) {

	/* Allow gcc to generate inline "ldX"/"stX" instructions for
	 * small, constant lengths.
	 */
	if ( __builtin_constant_p ( len ) && ( len <= 64 ) ) {
		__builtin_memcpy ( dest, src, len );
		return dest;
	}

	/* Otherwise, use variable-length code */
	arm64_memcpy ( dest, src, len );
	return dest;
}

/**
 * Copy (possibly overlapping) memory region
 *
 * @v dest		Destination region
 * @v src		Source region
 * @v len		Length
 * @ret dest		Destination region
 */
static inline __attribute__ (( always_inline )) void *
memmove ( void *dest, const void *src, size_t len ) {
	ssize_t offset = ( dest - src );

	/* If required direction of copy is known at build time, then
	 * use the appropriate forwards/backwards copy directly.
	 */
	if ( __builtin_constant_p ( offset ) ) {
		if ( offset <= 0 ) {
			arm64_memmove_forwards ( dest, src, len );
			return dest;
		} else {
			arm64_memmove_backwards ( dest, src, len );
			return dest;
		}
	}

	/* Otherwise, use ambidirectional copy */
	arm64_memmove ( dest, src, len );
	return dest;
}

#endif /* BITS_STRING_H */

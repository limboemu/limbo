/*
 * Copyright (C) 2007 Michael Brown <mbrown@fensystems.co.uk>.
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

/** @file
 *
 * Optimised string operations
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <string.h>

/**
 * Copy memory area
 *
 * @v dest		Destination address
 * @v src		Source address
 * @v len		Length
 * @ret dest		Destination address
 */
void * __attribute__ (( noinline )) __memcpy ( void *dest, const void *src,
					       size_t len ) {
	void *edi = dest;
	const void *esi = src;
	int discard_ecx;

	/* We often do large dword-aligned and dword-length block
	 * moves.  Using movsl rather than movsb speeds these up by
	 * around 32%.
	 */
	__asm__ __volatile__ ( "rep movsl"
			       : "=&D" ( edi ), "=&S" ( esi ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( esi ), "2" ( len >> 2 )
			       : "memory" );
	__asm__ __volatile__ ( "rep movsb"
			       : "=&D" ( edi ), "=&S" ( esi ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( esi ), "2" ( len & 3 )
			       : "memory" );
	return dest;
}

/**
 * Copy memory area backwards
 *
 * @v dest		Destination address
 * @v src		Source address
 * @v len		Length
 * @ret dest		Destination address
 */
void * __attribute__ (( noinline )) __memcpy_reverse ( void *dest,
						       const void *src,
						       size_t len ) {
	void *edi = ( dest + len - 1 );
	const void *esi = ( src + len - 1 );
	int discard_ecx;

	/* Assume memmove() is not performance-critical, and perform a
	 * bytewise copy for simplicity.
	 */
	__asm__ __volatile__ ( "std\n\t"
			       "rep movsb\n\t"
			       "cld\n\t"
			       : "=&D" ( edi ), "=&S" ( esi ),
				 "=&c" ( discard_ecx )
			       : "0" ( edi ), "1" ( esi ),
				 "2" ( len )
			       : "memory" );
	return dest;
}


/**
 * Copy (possibly overlapping) memory area
 *
 * @v dest		Destination address
 * @v src		Source address
 * @v len		Length
 * @ret dest		Destination address
 */
void * __memmove ( void *dest, const void *src, size_t len ) {

	if ( dest <= src ) {
		return __memcpy ( dest, src, len );
	} else {
		return __memcpy_reverse ( dest, src, len );
	}
}

/**
 * Swap memory areas
 *
 * @v dest		Destination address
 * @v src		Source address
 * @v len		Length
 * @ret dest		Destination address
 */
void * memswap ( void *dest, void *src, size_t len ) {
	size_t discard_c;
	int discard;

	__asm__ __volatile__ ( "\n1:\n\t"
			       "dec %2\n\t"
			       "js 2f\n\t"
			       "movb (%0,%2), %b3\n\t"
			       "xchgb (%1,%2), %b3\n\t"
			       "movb %b3, (%0,%2)\n\t"
			       "jmp 1b\n\t"
			       "2:\n\t"
			       : "=r" ( src ), "=r" ( dest ),
				 "=&c" ( discard_c ), "=&q" ( discard )
			       : "0" ( src ), "1" ( dest ), "2" ( len )
			       : "memory" );

	return dest;
}

/**
 * Calculate length of string
 *
 * @v string		String
 * @ret len		Length (excluding NUL)
 */
size_t strlen ( const char *string ) {
	const char *discard_D;
	size_t len_plus_one;

	__asm__ __volatile__ ( "repne scasb\n\t"
			       "not %1\n\t"
			       : "=&D" ( discard_D ), "=&c" ( len_plus_one )
			       : "0" ( string ), "1" ( -1UL ), "a" ( 0 ) );

	return ( len_plus_one - 1 );
}

/**
 * Compare strings (up to a specified length)
 *
 * @v str1		First string
 * @v str2		Second string
 * @v len		Maximum length
 * @ret diff		Difference
 */
int strncmp ( const char *str1, const char *str2, size_t len ) {
	const void *discard_S;
	const void *discard_D;
	size_t discard_c;
	int diff;

	__asm__ __volatile__ ( "\n1:\n\t"
			       "dec %2\n\t"
			       "js 2f\n\t"
			       "lodsb\n\t"
			       "scasb\n\t"
			       "jne 3f\n\t"
			       "testb %b3, %b3\n\t"
			       "jnz 1b\n\t"
			       /* Equal */
			       "\n2:\n\t"
			       "xor %3, %3\n\t"
			       "jmp 4f\n\t"
			       /* Not equal; CF indicates difference */
			       "\n3:\n\t"
			       "sbb %3, %3\n\t"
			       "orb $1, %b3\n\t"
			       "\n4:\n\t"
			       : "=&S" ( discard_S ), "=&D" ( discard_D ),
				 "=&c" ( discard_c ), "=&a" ( diff )
			       : "0" ( str1 ), "1" ( str2 ), "2" ( len ) );

	return diff;
}

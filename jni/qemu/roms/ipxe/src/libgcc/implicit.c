/** @file
 *
 * gcc sometimes likes to insert implicit calls to memcpy() and
 * memset().  Unfortunately, there doesn't seem to be any way to
 * prevent it from doing this, or to force it to use the optimised
 * versions as seen by C code; it insists on inserting symbol
 * references to "memcpy" and "memset".  We therefore include wrapper
 * functions just to keep gcc happy.
 *
 */

#include <string.h>

void * gcc_implicit_memcpy ( void *dest, const void *src,
			     size_t len ) asm ( "memcpy" );

void * gcc_implicit_memcpy ( void *dest, const void *src, size_t len ) {
	return memcpy ( dest, src, len );
}

void * gcc_implicit_memset ( void *dest, int character,
			     size_t len ) asm ( "memset" );

void * gcc_implicit_memset ( void *dest, int character, size_t len ) {
	return memset ( dest, character, len );
}

#ifndef _IPXE_NULL_SANBOOT_H
#define _IPXE_NULL_SANBOOT_H

/** @file
 *
 * Standard do-nothing sanboot interface
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#ifdef SANBOOT_NULL
#define SANBOOT_PREFIX_null
#else
#define SANBOOT_PREFIX_null __null_
#endif

static inline __always_inline unsigned int
SANBOOT_INLINE ( null, san_default_drive ) ( void ) {
	return 0;
}

#endif /* _IPXE_NULL_SANBOOT_H */

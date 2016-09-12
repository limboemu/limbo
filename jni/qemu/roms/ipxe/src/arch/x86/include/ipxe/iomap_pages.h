#ifndef _IPXE_IOMAP_PAGES_H
#define _IPXE_IOMAP_PAGES_H

/** @file
 *
 * I/O mapping API using page tables
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#ifdef IOMAP_PAGES
#define IOMAP_PREFIX_pages
#else
#define IOMAP_PREFIX_pages __pages_
#endif

static inline __always_inline unsigned long
IOMAP_INLINE ( pages, io_to_bus ) ( volatile const void *io_addr ) {
	/* Not easy to do; just return the CPU address for debugging purposes */
	return ( ( intptr_t ) io_addr );
}

#endif /* _IPXE_IOMAP_PAGES_H */

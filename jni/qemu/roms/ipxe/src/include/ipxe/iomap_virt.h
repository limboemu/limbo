#ifndef _IPXE_IOMAP_VIRT_H
#define _IPXE_IOMAP_VIRT_H

/** @file
 *
 * iPXE I/O mapping API using phys_to_virt()
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#ifdef IOMAP_VIRT
#define IOMAP_PREFIX_virt
#else
#define IOMAP_PREFIX_virt __virt_
#endif

static inline __always_inline void *
IOMAP_INLINE ( virt, ioremap ) ( unsigned long bus_addr, size_t len __unused ) {
	return ( bus_addr ? phys_to_virt ( bus_addr ) : NULL );
}

static inline __always_inline void
IOMAP_INLINE ( virt, iounmap ) ( volatile const void *io_addr __unused ) {
	/* Nothing to do */
}

static inline __always_inline unsigned long
IOMAP_INLINE ( virt, io_to_bus ) ( volatile const void *io_addr ) {
	return virt_to_phys ( io_addr );
}

#endif /* _IPXE_IOMAP_VIRT_H */

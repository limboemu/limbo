#ifndef _IPXE_IOMAP_H
#define _IPXE_IOMAP_H

/** @file
 *
 * iPXE I/O mapping API
 *
 * The I/O mapping API provides methods for mapping and unmapping I/O
 * devices.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <ipxe/api.h>
#include <config/ioapi.h>
#include <ipxe/uaccess.h>

/**
 * Calculate static inline I/O mapping API function name
 *
 * @v _prefix		Subsystem prefix
 * @v _api_func		API function
 * @ret _subsys_func	Subsystem API function
 */
#define IOMAP_INLINE( _subsys, _api_func ) \
	SINGLE_API_INLINE ( IOMAP_PREFIX_ ## _subsys, _api_func )

/**
 * Provide an I/O mapping API implementation
 *
 * @v _prefix		Subsystem prefix
 * @v _api_func		API function
 * @v _func		Implementing function
 */
#define PROVIDE_IOMAP( _subsys, _api_func, _func ) \
	PROVIDE_SINGLE_API ( IOMAP_PREFIX_ ## _subsys, _api_func, _func )

/**
 * Provide a static inline I/O mapping API implementation
 *
 * @v _prefix		Subsystem prefix
 * @v _api_func		API function
 */
#define PROVIDE_IOMAP_INLINE( _subsys, _api_func ) \
	PROVIDE_SINGLE_API_INLINE ( IOMAP_PREFIX_ ## _subsys, _api_func )

/* Include all architecture-independent I/O API headers */
#include <ipxe/iomap_virt.h>

/* Include all architecture-dependent I/O API headers */
#include <bits/iomap.h>

/**
 * Map bus address as an I/O address
 *
 * @v bus_addr		Bus address
 * @v len		Length of region
 * @ret io_addr		I/O address
 */
void * ioremap ( unsigned long bus_addr, size_t len );

/**
 * Unmap I/O address
 *
 * @v io_addr		I/O address
 */
void iounmap ( volatile const void *io_addr );

/**
 * Convert I/O address to bus address (for debug only)
 *
 * @v io_addr		I/O address
 * @ret bus_addr	Bus address
 */
unsigned long io_to_bus ( volatile const void *io_addr );

#endif /* _IPXE_IOMAP_H */

/*
 * Copyright (C) 2016 Michael Brown <mbrown@fensystems.co.uk>.
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
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/io.h>
#include <ipxe/arm_io.h>

/** @file
 *
 * iPXE I/O API for ARM
 *
 */

/** An ARM I/O qword */
union arm32_io_qword {
	uint64_t qword;
	uint32_t dword[2];
};

/**
 * Read 64-bit qword from memory-mapped device
 *
 * @v io_addr		I/O address
 * @ret data		Value read
 *
 * This is not atomic for ARM32.
 */
static uint64_t arm32_readq ( volatile uint64_t *io_addr ) {
	volatile union arm32_io_qword *ptr =
		container_of ( io_addr, union arm32_io_qword, qword );
	union arm32_io_qword tmp;

	tmp.dword[0] = readl ( &ptr->dword[0] );
	tmp.dword[1] = readl ( &ptr->dword[1] );
	return tmp.qword;
}

/**
 * Write 64-bit qword to memory-mapped device
 *
 * @v data		Value to write
 * @v io_addr		I/O address
 *
 * This is not atomic for ARM32.
 */
static void arm32_writeq ( uint64_t data, volatile uint64_t *io_addr ) {
	volatile union arm32_io_qword *ptr =
		container_of ( io_addr, union arm32_io_qword, qword );
	union arm32_io_qword tmp;

	tmp.qword = data;
	writel ( tmp.dword[0], &ptr->dword[0] );
	writel ( tmp.dword[1], &ptr->dword[1] );
}

PROVIDE_IOAPI_INLINE ( arm, phys_to_bus );
PROVIDE_IOAPI_INLINE ( arm, bus_to_phys );
PROVIDE_IOAPI_INLINE ( arm, readb );
PROVIDE_IOAPI_INLINE ( arm, readw );
PROVIDE_IOAPI_INLINE ( arm, readl );
PROVIDE_IOAPI_INLINE ( arm, writeb );
PROVIDE_IOAPI_INLINE ( arm, writew );
PROVIDE_IOAPI_INLINE ( arm, writel );
PROVIDE_IOAPI_INLINE ( arm, iodelay );
PROVIDE_IOAPI_INLINE ( arm, mb );
#ifdef __aarch64__
PROVIDE_IOAPI_INLINE ( arm, readq );
PROVIDE_IOAPI_INLINE ( arm, writeq );
#else
PROVIDE_IOAPI ( arm, readq, arm32_readq );
PROVIDE_IOAPI ( arm, writeq, arm32_writeq );
#endif

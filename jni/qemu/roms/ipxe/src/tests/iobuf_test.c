/*
 * Copyright (C) 2016 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

/** @file
 *
 * I/O buffer tests
 *
 */

/* Forcibly enable assertions */
#undef NDEBUG

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <ipxe/iobuf.h>
#include <ipxe/io.h>
#include <ipxe/test.h>

/* Forward declaration */
struct self_test iobuf_test __self_test;

/**
 * Report I/O buffer allocation test result
 *
 * @v len		Required length of buffer
 * @v align		Physical alignment
 * @v offset		Offset from physical alignment
 * @v file		Test code file
 * @v line		Test code line
 */
static inline void alloc_iob_okx ( size_t len, size_t align, size_t offset,
				   const char *file, unsigned int line ) {
	struct io_buffer *iobuf;

	/* Allocate I/O buffer */
	iobuf = alloc_iob_raw ( len, align, offset );
	okx ( iobuf != NULL, file, line );
	DBGC ( &iobuf_test, "IOBUF %p (%#08lx+%#zx) for %#zx align %#zx "
	       "offset %#zx\n", iobuf, virt_to_phys ( iobuf->data ),
	       iob_tailroom ( iobuf ), len, align, offset );

	/* Validate requested length and alignment */
	okx ( ( ( ( intptr_t ) iobuf ) & ( __alignof__ ( *iobuf ) - 1 ) ) == 0,
		file, line );
	okx ( iob_tailroom ( iobuf ) >= len, file, line );
	okx ( ( ( align == 0 ) ||
		( ( virt_to_phys ( iobuf->data ) & ( align - 1 ) ) ==
		  ( offset & ( align - 1 ) ) ) ), file, line );

	/* Overwrite entire content of I/O buffer (for Valgrind) */
	memset ( iob_put ( iobuf, len ), 0x55, len );

	/* Free I/O buffer */
	free_iob ( iobuf );
}
#define alloc_iob_ok( len, align, offset ) \
	alloc_iob_okx ( len, align, offset, __FILE__, __LINE__ )

/**
 * Report I/O buffer allocation failure test result
 *
 * @v len		Required length of buffer
 * @v align		Physical alignment
 * @v offset		Offset from physical alignment
 * @v file		Test code file
 * @v line		Test code line
 */
static inline void alloc_iob_fail_okx ( size_t len, size_t align, size_t offset,
					const char *file, unsigned int line ) {
	struct io_buffer *iobuf;

	/* Allocate I/O buffer */
	iobuf = alloc_iob_raw ( len, align, offset );
	okx ( iobuf == NULL, file, line );
}
#define alloc_iob_fail_ok( len, align, offset ) \
	alloc_iob_fail_okx ( len, align, offset, __FILE__, __LINE__ )

/**
 * Perform I/O buffer self-tests
 *
 */
static void iobuf_test_exec ( void ) {

	/* Check zero-length allocations */
	alloc_iob_ok ( 0, 0, 0 );
	alloc_iob_ok ( 0, 0, 1 );
	alloc_iob_ok ( 0, 1, 0 );
	alloc_iob_ok ( 0, 1024, 0 );
	alloc_iob_ok ( 0, 139, -17 );

	/* Check various sensible allocations */
	alloc_iob_ok ( 1, 0, 0 );
	alloc_iob_ok ( 16, 16, 0 );
	alloc_iob_ok ( 64, 0, 0 );
	alloc_iob_ok ( 65, 0, 0 );
	alloc_iob_ok ( 65, 1024, 19 );
	alloc_iob_ok ( 1536, 1536, 0 );
	alloc_iob_ok ( 2048, 2048, 0 );
	alloc_iob_ok ( 2048, 2048, -10 );

	/* Excessively large or excessively aligned allocations should fail */
	alloc_iob_fail_ok ( -1UL, 0, 0 );
	alloc_iob_fail_ok ( -1UL, 1024, 0 );
	alloc_iob_fail_ok ( 0, -1UL, 0 );
	alloc_iob_fail_ok ( 1024, -1UL, 0 );
}

/** I/O buffer self-test */
struct self_test iobuf_test __self_test = {
	.name = "iobuf",
	.exec = iobuf_test_exec,
};

/*
 * Copyright (C) 2012 Michael Brown <mbrown@fensystems.co.uk>.
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
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ipxe/xfer.h>
#include <ipxe/iobuf.h>
#include <ipxe/xferbuf.h>

/** @file
 *
 * Data transfer buffer
 *
 */

/**
 * Finish using data transfer buffer
 *
 * @v xferbuf		Data transfer buffer
 */
void xferbuf_done ( struct xfer_buffer *xferbuf ) {
	free ( xferbuf->data );
	xferbuf->data = NULL;
	xferbuf->len = 0;
	xferbuf->pos = 0;
}

/**
 * Ensure that data transfer buffer is large enough for the specified size
 *
 * @v xferbuf		Data transfer buffer
 * @v len		Required minimum size
 * @ret rc		Return status code
 */
static int xferbuf_ensure_size ( struct xfer_buffer *xferbuf, size_t len ) {
	void *new_data;

	/* If buffer is already large enough, do nothing */
	if ( len <= xferbuf->len )
		return 0;

	/* Extend buffer */
	new_data = realloc ( xferbuf->data, len );
	if ( ! new_data ) {
		DBGC ( xferbuf, "XFERBUF %p could not extend buffer to "
		       "%zd bytes\n", xferbuf, len );
		return -ENOSPC;
	}
	xferbuf->data = new_data;
	xferbuf->len = len;

	return 0;
}

/**
 * Add received data to data transfer buffer
 *
 * @v xferbuf		Data transfer buffer
 * @v iobuf		I/O buffer
 * @v meta		Data transfer metadata
 * @ret rc		Return status code
 */
int xferbuf_deliver ( struct xfer_buffer *xferbuf, struct io_buffer *iobuf,
		      struct xfer_metadata *meta ) {
	size_t len;
	size_t max;
	int rc;

	/* Calculate new buffer position */
	if ( meta->flags & XFER_FL_ABS_OFFSET )
		xferbuf->pos = 0;
	xferbuf->pos += meta->offset;

	/* Ensure that we have enough buffer space for this data */
	len = iob_len ( iobuf );
	max = ( xferbuf->pos + len );
	if ( ( rc = xferbuf_ensure_size ( xferbuf, max ) ) != 0 )
		goto done;

	/* Copy data to buffer */
	memcpy ( ( xferbuf->data + xferbuf->pos ), iobuf->data, len );

	/* Update current buffer position */
	xferbuf->pos += len;

 done:
	free_iob ( iobuf );
	return rc;
}

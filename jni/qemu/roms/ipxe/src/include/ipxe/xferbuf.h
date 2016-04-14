#ifndef _IPXE_XFERBUF_H
#define _IPXE_XFERBUF_H

/** @file
 *
 * Data transfer buffer
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdint.h>
#include <ipxe/iobuf.h>
#include <ipxe/xfer.h>

/** A data transfer buffer */
struct xfer_buffer {
	/** Data */
	void *data;
	/** Size of data */
	size_t len;
	/** Current offset within data */
	size_t pos;
};

extern void xferbuf_done ( struct xfer_buffer *xferbuf );
extern int xferbuf_deliver ( struct xfer_buffer *xferbuf,
			     struct io_buffer *iobuf,
			     struct xfer_metadata *meta );

#endif /* _IPXE_XFERBUF_H */

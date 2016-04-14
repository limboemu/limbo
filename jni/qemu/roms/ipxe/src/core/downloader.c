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

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdlib.h>
#include <errno.h>
#include <syslog.h>
#include <ipxe/iobuf.h>
#include <ipxe/xfer.h>
#include <ipxe/open.h>
#include <ipxe/job.h>
#include <ipxe/uaccess.h>
#include <ipxe/umalloc.h>
#include <ipxe/image.h>
#include <ipxe/profile.h>
#include <ipxe/downloader.h>

/** @file
 *
 * Image downloader
 *
 */

/** Receive profiler */
static struct profiler downloader_rx_profiler __profiler =
	{ .name = "downloader.rx" };

/** Data copy profiler */
static struct profiler downloader_copy_profiler __profiler =
	{ .name = "downloader.copy" };

/** A downloader */
struct downloader {
	/** Reference count for this object */
	struct refcnt refcnt;

	/** Job control interface */
	struct interface job;
	/** Data transfer interface */
	struct interface xfer;

	/** Image to contain downloaded file */
	struct image *image;
	/** Current position within image buffer */
	size_t pos;
};

/**
 * Free downloader object
 *
 * @v refcnt		Downloader reference counter
 */
static void downloader_free ( struct refcnt *refcnt ) {
	struct downloader *downloader =
		container_of ( refcnt, struct downloader, refcnt );

	image_put ( downloader->image );
	free ( downloader );
}

/**
 * Terminate download
 *
 * @v downloader	Downloader
 * @v rc		Reason for termination
 */
static void downloader_finished ( struct downloader *downloader, int rc ) {

	/* Log download status */
	if ( rc == 0 ) {
		syslog ( LOG_NOTICE, "Downloaded \"%s\"\n",
			 downloader->image->name );
	} else {
		syslog ( LOG_ERR, "Download of \"%s\" failed: %s\n",
			 downloader->image->name, strerror ( rc ) );
	}

	/* Shut down interfaces */
	intf_shutdown ( &downloader->xfer, rc );
	intf_shutdown ( &downloader->job, rc );
}

/**
 * Ensure that download buffer is large enough for the specified size
 *
 * @v downloader	Downloader
 * @v len		Required minimum size
 * @ret rc		Return status code
 */
static int downloader_ensure_size ( struct downloader *downloader,
				    size_t len ) {
	userptr_t new_buffer;

	/* If buffer is already large enough, do nothing */
	if ( len <= downloader->image->len )
		return 0;

	DBGC ( downloader, "Downloader %p extending to %zd bytes\n",
	       downloader, len );

	/* Extend buffer */
	new_buffer = urealloc ( downloader->image->data, len );
	if ( ! new_buffer ) {
		DBGC ( downloader, "Downloader %p could not extend buffer to "
		       "%zd bytes\n", downloader, len );
		return -ENOSPC;
	}
	downloader->image->data = new_buffer;
	downloader->image->len = len;

	return 0;
}

/****************************************************************************
 *
 * Job control interface
 *
 */

/**
 * Report progress of download job
 *
 * @v downloader	Downloader
 * @v progress		Progress report to fill in
 * @ret ongoing_rc	Ongoing job status code (if known)
 */
static int downloader_progress ( struct downloader *downloader,
				 struct job_progress *progress ) {

	/* This is not entirely accurate, since downloaded data may
	 * arrive out of order (e.g. with multicast protocols), but
	 * it's a reasonable first approximation.
	 */
	progress->completed = downloader->pos;
	progress->total = downloader->image->len;

	return 0;
}

/****************************************************************************
 *
 * Data transfer interface
 *
 */

/**
 * Handle received data
 *
 * @v downloader	Downloader
 * @v iobuf		Datagram I/O buffer
 * @v meta		Data transfer metadata
 * @ret rc		Return status code
 */
static int downloader_xfer_deliver ( struct downloader *downloader,
				     struct io_buffer *iobuf,
				     struct xfer_metadata *meta ) {
	size_t len;
	size_t max;
	int rc;

	/* Start profiling */
	profile_start ( &downloader_rx_profiler );

	/* Calculate new buffer position */
	if ( meta->flags & XFER_FL_ABS_OFFSET )
		downloader->pos = 0;
	downloader->pos += meta->offset;

	/* Ensure that we have enough buffer space for this data */
	len = iob_len ( iobuf );
	max = ( downloader->pos + len );
	if ( ( rc = downloader_ensure_size ( downloader, max ) ) != 0 )
		goto done;

	/* Copy data to buffer */
	profile_start ( &downloader_copy_profiler );
	copy_to_user ( downloader->image->data, downloader->pos,
		       iobuf->data, len );
	profile_stop ( &downloader_copy_profiler );

	/* Update current buffer position */
	downloader->pos += len;

 done:
	free_iob ( iobuf );
	if ( rc != 0 )
		downloader_finished ( downloader, rc );
	profile_stop ( &downloader_rx_profiler );
	return rc;
}

/** Downloader data transfer interface operations */
static struct interface_operation downloader_xfer_operations[] = {
	INTF_OP ( xfer_deliver, struct downloader *, downloader_xfer_deliver ),
	INTF_OP ( intf_close, struct downloader *, downloader_finished ),
};

/** Downloader data transfer interface descriptor */
static struct interface_descriptor downloader_xfer_desc =
	INTF_DESC ( struct downloader, xfer, downloader_xfer_operations );

/****************************************************************************
 *
 * Job control interface
 *
 */

/** Downloader job control interface operations */
static struct interface_operation downloader_job_op[] = {
	INTF_OP ( job_progress, struct downloader *, downloader_progress ),
	INTF_OP ( intf_close, struct downloader *, downloader_finished ),
};

/** Downloader job control interface descriptor */
static struct interface_descriptor downloader_job_desc =
	INTF_DESC ( struct downloader, job, downloader_job_op );

/****************************************************************************
 *
 * Instantiator
 *
 */

/**
 * Instantiate a downloader
 *
 * @v job		Job control interface
 * @v image		Image to fill with downloaded file
 * @ret rc		Return status code
 *
 * Instantiates a downloader object to download the content of the
 * specified image from its URI.
 */
int create_downloader ( struct interface *job, struct image *image ) {
	struct downloader *downloader;
	int rc;

	/* Allocate and initialise structure */
	downloader = zalloc ( sizeof ( *downloader ) );
	if ( ! downloader )
		return -ENOMEM;
	ref_init ( &downloader->refcnt, downloader_free );
	intf_init ( &downloader->job, &downloader_job_desc,
		    &downloader->refcnt );
	intf_init ( &downloader->xfer, &downloader_xfer_desc,
		    &downloader->refcnt );
	downloader->image = image_get ( image );

	/* Instantiate child objects and attach to our interfaces */
	if ( ( rc = xfer_open_uri ( &downloader->xfer, image->uri ) ) != 0 )
		goto err;

	/* Attach parent interface, mortalise self, and return */
	intf_plug_plug ( &downloader->job, job );
	ref_put ( &downloader->refcnt );
	return 0;

 err:
	downloader_finished ( downloader, rc );
	ref_put ( &downloader->refcnt );
	return rc;
}

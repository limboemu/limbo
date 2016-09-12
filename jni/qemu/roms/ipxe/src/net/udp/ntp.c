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

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <ipxe/malloc.h>
#include <ipxe/refcnt.h>
#include <ipxe/iobuf.h>
#include <ipxe/xfer.h>
#include <ipxe/open.h>
#include <ipxe/retry.h>
#include <ipxe/timer.h>
#include <ipxe/time.h>
#include <ipxe/tcpip.h>
#include <ipxe/ntp.h>

/** @file
 *
 * Network Time Protocol
 *
 */

/** An NTP client */
struct ntp_client {
	/** Reference count */
	struct refcnt refcnt;
	/** Job control interface */
	struct interface job;
	/** Data transfer interface */
	struct interface xfer;
	/** Retransmission timer */
	struct retry_timer timer;
};

/**
 * Close NTP client
 *
 * @v ntp		NTP client
 * @v rc		Reason for close
 */
static void ntp_close ( struct ntp_client *ntp, int rc ) {

	/* Stop timer */
	stop_timer ( &ntp->timer );

	/* Shut down interfaces */
	intf_shutdown ( &ntp->xfer, rc );
	intf_shutdown ( &ntp->job, rc );
}

/**
 * Send NTP request
 *
 * @v ntp		NTP client
 * @ret rc		Return status code
 */
static int ntp_request ( struct ntp_client *ntp ) {
	struct ntp_header hdr;
	int rc;

	DBGC ( ntp, "NTP %p sending request\n", ntp );

	/* Construct header */
	memset ( &hdr, 0, sizeof ( hdr ) );
	hdr.flags = ( NTP_FL_LI_UNKNOWN | NTP_FL_VN_1 | NTP_FL_MODE_CLIENT );
	hdr.transmit.seconds = htonl ( time ( NULL ) + NTP_EPOCH );
	hdr.transmit.fraction = htonl ( NTP_FRACTION_MAGIC );

	/* Send request */
	if ( ( rc = xfer_deliver_raw ( &ntp->xfer, &hdr,
				       sizeof ( hdr ) ) ) != 0 ) {
		DBGC ( ntp, "NTP %p could not send request: %s\n",
		       ntp, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Handle NTP response
 *
 * @v ntp		NTP client
 * @v iobuf		I/O buffer
 * @v meta		Data transfer metadata
 * @ret rc		Return status code
 */
static int ntp_deliver ( struct ntp_client *ntp, struct io_buffer *iobuf,
			 struct xfer_metadata *meta ) {
	struct ntp_header *hdr;
	struct sockaddr_tcpip *st_src;
	int32_t delta;
	int rc;

	/* Check source port */
	st_src = ( ( struct sockaddr_tcpip * ) meta->src );
	if ( st_src->st_port != htons ( NTP_PORT ) ) {
		DBGC ( ntp, "NTP %p received non-NTP packet:\n", ntp );
		DBGC_HDA ( ntp, 0, iobuf->data, iob_len ( iobuf ) );
		goto ignore;
	}

	/* Check packet length */
	if ( iob_len ( iobuf ) < sizeof ( *hdr ) ) {
		DBGC ( ntp, "NTP %p received malformed packet:\n", ntp );
		DBGC_HDA ( ntp, 0, iobuf->data, iob_len ( iobuf ) );
		goto ignore;
	}
	hdr = iobuf->data;

	/* Check mode */
	if ( ( hdr->flags & NTP_FL_MODE_MASK ) != NTP_FL_MODE_SERVER ) {
		DBGC ( ntp, "NTP %p received non-server packet:\n", ntp );
		DBGC_HDA ( ntp, 0, iobuf->data, iob_len ( iobuf ) );
		goto ignore;
	}

	/* Check magic value */
	if ( hdr->originate.fraction != htonl ( NTP_FRACTION_MAGIC ) ) {
		DBGC ( ntp, "NTP %p received unrecognised packet:\n", ntp );
		DBGC_HDA ( ntp, 0, iobuf->data, iob_len ( iobuf ) );
		goto ignore;
	}

	/* Check for Kiss-o'-Death packets */
	if ( ! hdr->stratum ) {
		DBGC ( ntp, "NTP %p received kiss-o'-death:\n", ntp );
		DBGC_HDA ( ntp, 0, iobuf->data, iob_len ( iobuf ) );
		rc = -EPROTO;
		goto close;
	}

	/* Calculate clock delta */
	delta = ( ntohl ( hdr->receive.seconds ) -
		  ntohl ( hdr->originate.seconds ) );
	DBGC ( ntp, "NTP %p delta %d seconds\n", ntp, delta );

	/* Adjust system clock */
	time_adjust ( delta );

	/* Success */
	rc = 0;

 close:
	ntp_close ( ntp, rc );
 ignore:
	free_iob ( iobuf );
	return 0;
}

/**
 * Handle data transfer window change
 *
 * @v ntp		NTP client
 */
static void ntp_window_changed ( struct ntp_client *ntp ) {

	/* Start timer to send initial request */
	start_timer_nodelay ( &ntp->timer );
}

/** Data transfer interface operations */
static struct interface_operation ntp_xfer_op[] = {
	INTF_OP ( xfer_deliver, struct ntp_client *, ntp_deliver ),
	INTF_OP ( xfer_window_changed, struct ntp_client *,
		  ntp_window_changed ),
	INTF_OP ( intf_close, struct ntp_client *, ntp_close ),
};

/** Data transfer interface descriptor */
static struct interface_descriptor ntp_xfer_desc =
	INTF_DESC_PASSTHRU ( struct ntp_client, xfer, ntp_xfer_op, job );

/** Job control interface operations */
static struct interface_operation ntp_job_op[] = {
	INTF_OP ( intf_close, struct ntp_client *, ntp_close ),
};

/** Job control interface descriptor */
static struct interface_descriptor ntp_job_desc =
	INTF_DESC_PASSTHRU ( struct ntp_client, job, ntp_job_op, xfer );

/**
 * Handle NTP timer expiry
 *
 * @v timer		Retransmission timer
 * @v fail		Failure indicator
 */
static void ntp_expired ( struct retry_timer *timer, int fail ) {
	struct ntp_client *ntp =
		container_of ( timer, struct ntp_client, timer );

	/* Shut down client if we have failed */
	if ( fail ) {
		ntp_close ( ntp, -ETIMEDOUT );
		return;
	}

	/* Otherwise, restart timer and (re)transmit request */
	start_timer ( &ntp->timer );
	ntp_request ( ntp );
}

/**
 * Start NTP client
 *
 * @v job		Job control interface
 * @v hostname		NTP server
 * @ret rc		Return status code
 */
int start_ntp ( struct interface *job, const char *hostname ) {
	struct ntp_client *ntp;
	union {
		struct sockaddr_tcpip st;
		struct sockaddr sa;
	} server;
	int rc;

	/* Allocate and initialise structure*/
	ntp = zalloc ( sizeof ( *ntp ) );
	if ( ! ntp ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	ref_init ( &ntp->refcnt, NULL );
	intf_init ( &ntp->job, &ntp_job_desc, &ntp->refcnt );
	intf_init ( &ntp->xfer, &ntp_xfer_desc, &ntp->refcnt );
	timer_init ( &ntp->timer, ntp_expired, &ntp->refcnt );
	set_timer_limits ( &ntp->timer, NTP_MIN_TIMEOUT, NTP_MAX_TIMEOUT );

	/* Open socket */
	memset ( &server, 0, sizeof ( server ) );
	server.st.st_port = htons ( NTP_PORT );
	if ( ( rc = xfer_open_named_socket ( &ntp->xfer, SOCK_DGRAM, &server.sa,
					     hostname, NULL ) ) != 0 ) {
		DBGC ( ntp, "NTP %p could not open socket: %s\n",
		       ntp, strerror ( rc ) );
		goto err_open;
	}

	/* Attach parent interface, mortalise self, and return */
	intf_plug_plug ( &ntp->job, job );
	ref_put ( &ntp->refcnt );
	return 0;

 err_open:
	ntp_close ( ntp, rc );
	ref_put ( &ntp->refcnt );
 err_alloc:
	return rc;
}

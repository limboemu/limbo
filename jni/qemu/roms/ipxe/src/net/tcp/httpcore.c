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

/**
 * @file
 *
 * Hyper Text Transfer Protocol (HTTP) core functionality
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <byteswap.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>
#include <ipxe/uri.h>
#include <ipxe/refcnt.h>
#include <ipxe/iobuf.h>
#include <ipxe/xfer.h>
#include <ipxe/open.h>
#include <ipxe/socket.h>
#include <ipxe/tcpip.h>
#include <ipxe/process.h>
#include <ipxe/retry.h>
#include <ipxe/timer.h>
#include <ipxe/linebuf.h>
#include <ipxe/base64.h>
#include <ipxe/base16.h>
#include <ipxe/md5.h>
#include <ipxe/blockdev.h>
#include <ipxe/acpi.h>
#include <ipxe/version.h>
#include <ipxe/params.h>
#include <ipxe/profile.h>
#include <ipxe/http.h>

/* Disambiguate the various error causes */
#define EACCES_401 __einfo_error ( EINFO_EACCES_401 )
#define EINFO_EACCES_401 \
	__einfo_uniqify ( EINFO_EACCES, 0x01, "HTTP 401 Unauthorized" )
#define EIO_OTHER __einfo_error ( EINFO_EIO_OTHER )
#define EINFO_EIO_OTHER \
	__einfo_uniqify ( EINFO_EIO, 0x01, "Unrecognised HTTP response code" )
#define EIO_CONTENT_LENGTH __einfo_error ( EINFO_EIO_CONTENT_LENGTH )
#define EINFO_EIO_CONTENT_LENGTH \
	__einfo_uniqify ( EINFO_EIO, 0x02, "Content length mismatch" )
#define EINVAL_RESPONSE __einfo_error ( EINFO_EINVAL_RESPONSE )
#define EINFO_EINVAL_RESPONSE \
	__einfo_uniqify ( EINFO_EINVAL, 0x01, "Invalid content length" )
#define EINVAL_HEADER __einfo_error ( EINFO_EINVAL_HEADER )
#define EINFO_EINVAL_HEADER \
	__einfo_uniqify ( EINFO_EINVAL, 0x02, "Invalid header" )
#define EINVAL_CONTENT_LENGTH __einfo_error ( EINFO_EINVAL_CONTENT_LENGTH )
#define EINFO_EINVAL_CONTENT_LENGTH \
	__einfo_uniqify ( EINFO_EINVAL, 0x03, "Invalid content length" )
#define EINVAL_CHUNK_LENGTH __einfo_error ( EINFO_EINVAL_CHUNK_LENGTH )
#define EINFO_EINVAL_CHUNK_LENGTH \
	__einfo_uniqify ( EINFO_EINVAL, 0x04, "Invalid chunk length" )
#define ENOENT_404 __einfo_error ( EINFO_ENOENT_404 )
#define EINFO_ENOENT_404 \
	__einfo_uniqify ( EINFO_ENOENT, 0x01, "HTTP 404 Not Found" )
#define EPERM_403 __einfo_error ( EINFO_EPERM_403 )
#define EINFO_EPERM_403 \
	__einfo_uniqify ( EINFO_EPERM, 0x01, "HTTP 403 Forbidden" )
#define EPROTO_UNSOLICITED __einfo_error ( EINFO_EPROTO_UNSOLICITED )
#define EINFO_EPROTO_UNSOLICITED \
	__einfo_uniqify ( EINFO_EPROTO, 0x01, "Unsolicited data" )

/** Block size used for HTTP block device request */
#define HTTP_BLKSIZE 512

/** Retry delay used when we cannot understand the Retry-After header */
#define HTTP_RETRY_SECONDS 5

/** Receive profiler */
static struct profiler http_rx_profiler __profiler = { .name = "http.rx" };

/** Data transfer profiler */
static struct profiler http_xfer_profiler __profiler = { .name = "http.xfer" };

/** HTTP flags */
enum http_flags {
	/** Request is waiting to be transmitted */
	HTTP_TX_PENDING = 0x0001,
	/** Fetch header only */
	HTTP_HEAD_ONLY = 0x0002,
	/** Client would like to keep connection alive */
	HTTP_CLIENT_KEEPALIVE = 0x0004,
	/** Server will keep connection alive */
	HTTP_SERVER_KEEPALIVE = 0x0008,
	/** Discard the current request and try again */
	HTTP_TRY_AGAIN = 0x0010,
	/** Provide Basic authentication details */
	HTTP_BASIC_AUTH = 0x0020,
	/** Provide Digest authentication details */
	HTTP_DIGEST_AUTH = 0x0040,
	/** Socket must be reopened */
	HTTP_REOPEN_SOCKET = 0x0080,
};

/** HTTP receive state */
enum http_rx_state {
	HTTP_RX_RESPONSE = 0,
	HTTP_RX_HEADER,
	HTTP_RX_CHUNK_LEN,
	/* In HTTP_RX_DATA, it is acceptable for the server to close
	 * the connection (unless we are in the middle of a chunked
	 * transfer).
	 */
	HTTP_RX_DATA,
	/* In the following states, it is acceptable for the server to
	 * close the connection.
	 */
	HTTP_RX_TRAILER,
	HTTP_RX_IDLE,
	HTTP_RX_DEAD,
};

/**
 * An HTTP request
 *
 */
struct http_request {
	/** Reference count */
	struct refcnt refcnt;
	/** Data transfer interface */
	struct interface xfer;
	/** Partial transfer interface */
	struct interface partial;

	/** URI being fetched */
	struct uri *uri;
	/** Default port */
	unsigned int default_port;
	/** Filter (if any) */
	int ( * filter ) ( struct interface *xfer,
			   const char *name,
			   struct interface **next );
	/** Transport layer interface */
	struct interface socket;

	/** Flags */
	unsigned int flags;
	/** Starting offset of partial transfer (if applicable) */
	size_t partial_start;
	/** Length of partial transfer (if applicable) */
	size_t partial_len;

	/** TX process */
	struct process process;

	/** RX state */
	enum http_rx_state rx_state;
	/** Response code */
	unsigned int code;
	/** Received length */
	size_t rx_len;
	/** Length remaining (or 0 if unknown) */
	size_t remaining;
	/** HTTP is using Transfer-Encoding: chunked */
	int chunked;
	/** Current chunk length remaining (if applicable) */
	size_t chunk_remaining;
	/** Line buffer for received header lines */
	struct line_buffer linebuf;
	/** Receive data buffer (if applicable) */
	userptr_t rx_buffer;

	/** Authentication realm (if any) */
	char *auth_realm;
	/** Authentication nonce (if any) */
	char *auth_nonce;
	/** Authentication opaque string (if any) */
	char *auth_opaque;

	/** Request retry timer */
	struct retry_timer timer;
	/** Retry delay (in timer ticks) */
	unsigned long retry_delay;
};

/**
 * Free HTTP request
 *
 * @v refcnt		Reference counter
 */
static void http_free ( struct refcnt *refcnt ) {
	struct http_request *http =
		container_of ( refcnt, struct http_request, refcnt );

	uri_put ( http->uri );
	empty_line_buffer ( &http->linebuf );
	free ( http->auth_realm );
	free ( http->auth_nonce );
	free ( http->auth_opaque );
	free ( http );
};

/**
 * Close HTTP request
 *
 * @v http		HTTP request
 * @v rc		Return status code
 */
static void http_close ( struct http_request *http, int rc ) {

	/* Prevent further processing of any current packet */
	http->rx_state = HTTP_RX_DEAD;

	/* Prevent reconnection */
	http->flags &= ~HTTP_CLIENT_KEEPALIVE;

	/* Remove process */
	process_del ( &http->process );

	/* Close all data transfer interfaces */
	intf_shutdown ( &http->socket, rc );
	intf_shutdown ( &http->partial, rc );
	intf_shutdown ( &http->xfer, rc );
}

/**
 * Open HTTP socket
 *
 * @v http		HTTP request
 * @ret rc		Return status code
 */
static int http_socket_open ( struct http_request *http ) {
	struct uri *uri = http->uri;
	struct sockaddr_tcpip server;
	struct interface *socket;
	int rc;

	/* Open socket */
	memset ( &server, 0, sizeof ( server ) );
	server.st_port = htons ( uri_port ( uri, http->default_port ) );
	socket = &http->socket;
	if ( http->filter ) {
		if ( ( rc = http->filter ( socket, uri->host, &socket ) ) != 0 )
			return rc;
	}
	if ( ( rc = xfer_open_named_socket ( socket, SOCK_STREAM,
					     ( struct sockaddr * ) &server,
					     uri->host, NULL ) ) != 0 )
		return rc;

	return 0;
}

/**
 * Retry HTTP request
 *
 * @v timer		Retry timer
 * @v fail		Failure indicator
 */
static void http_retry ( struct retry_timer *timer, int fail __unused ) {
	struct http_request *http =
		container_of ( timer, struct http_request, timer );
	int rc;

	/* Reopen socket if required */
	if ( http->flags & HTTP_REOPEN_SOCKET ) {
		http->flags &= ~HTTP_REOPEN_SOCKET;
		DBGC ( http, "HTTP %p reopening connection\n", http );
		if ( ( rc = http_socket_open ( http ) ) != 0 ) {
			http_close ( http, rc );
			return;
		}
	}

	/* Retry the request if applicable */
	if ( http->flags & HTTP_TRY_AGAIN ) {
		http->flags &= ~HTTP_TRY_AGAIN;
		DBGC ( http, "HTTP %p retrying request\n", http );
		http->flags |= HTTP_TX_PENDING;
		http->rx_state = HTTP_RX_RESPONSE;
		process_add ( &http->process );
	}
}

/**
 * Mark HTTP request as completed successfully
 *
 * @v http		HTTP request
 */
static void http_done ( struct http_request *http ) {

	/* If we are not at an appropriate stage of the protocol
	 * (including being in the middle of a chunked transfer),
	 * force an error.
	 */
	if ( ( http->rx_state < HTTP_RX_DATA ) || ( http->chunked != 0 ) ) {
		DBGC ( http, "HTTP %p connection closed unexpectedly in state "
		       "%d\n", http, http->rx_state );
		http_close ( http, -ECONNRESET );
		return;
	}

	/* If we had a Content-Length, and the received content length
	 * isn't correct, force an error
	 */
	if ( http->remaining != 0 ) {
		DBGC ( http, "HTTP %p incorrect length %zd, should be %zd\n",
		       http, http->rx_len, ( http->rx_len + http->remaining ) );
		http_close ( http, -EIO_CONTENT_LENGTH );
		return;
	}

	/* Enter idle state */
	http->rx_state = HTTP_RX_IDLE;
	http->rx_len = 0;
	assert ( http->remaining == 0 );
	assert ( http->chunked == 0 );
	assert ( http->chunk_remaining == 0 );

	/* Close partial transfer interface */
	if ( ! ( http->flags & HTTP_TRY_AGAIN ) )
		intf_restart ( &http->partial, 0 );

	/* Close everything unless we want to keep the connection alive */
	if ( ! ( http->flags & ( HTTP_CLIENT_KEEPALIVE | HTTP_TRY_AGAIN ) ) ) {
		http_close ( http, 0 );
		return;
	}

	/* If the server is not intending to keep the connection
	 * alive, then close the socket and mark it as requiring
	 * reopening.
	 */
	if ( ! ( http->flags & HTTP_SERVER_KEEPALIVE ) ) {
		intf_restart ( &http->socket, 0 );
		http->flags &= ~HTTP_SERVER_KEEPALIVE;
		http->flags |= HTTP_REOPEN_SOCKET;
	}

	/* Start request retry timer */
	start_timer_fixed ( &http->timer, http->retry_delay );
	http->retry_delay = 0;
}

/**
 * Convert HTTP response code to return status code
 *
 * @v response		HTTP response code
 * @ret rc		Return status code
 */
static int http_response_to_rc ( unsigned int response ) {
	switch ( response ) {
	case 200:
	case 206:
	case 301:
	case 302:
	case 303:
		return 0;
	case 404:
		return -ENOENT_404;
	case 403:
		return -EPERM_403;
	case 401:
		return -EACCES_401;
	default:
		return -EIO_OTHER;
	}
}

/**
 * Handle HTTP response
 *
 * @v http		HTTP request
 * @v response		HTTP response
 * @ret rc		Return status code
 */
static int http_rx_response ( struct http_request *http, char *response ) {
	char *spc;

	DBGC ( http, "HTTP %p response \"%s\"\n", http, response );

	/* Check response starts with "HTTP/" */
	if ( strncmp ( response, "HTTP/", 5 ) != 0 )
		return -EINVAL_RESPONSE;

	/* Locate and store response code */
	spc = strchr ( response, ' ' );
	if ( ! spc )
		return -EINVAL_RESPONSE;
	http->code = strtoul ( spc, NULL, 10 );

	/* Move to receive headers */
	http->rx_state = ( ( http->flags & HTTP_HEAD_ONLY ) ?
			   HTTP_RX_TRAILER : HTTP_RX_HEADER );
	return 0;
}

/**
 * Handle HTTP Location header
 *
 * @v http		HTTP request
 * @v value		HTTP header value
 * @ret rc		Return status code
 */
static int http_rx_location ( struct http_request *http, char *value ) {
	int rc;

	/* Redirect to new location */
	DBGC ( http, "HTTP %p redirecting to %s\n", http, value );
	if ( ( rc = xfer_redirect ( &http->xfer, LOCATION_URI_STRING,
				    value ) ) != 0 ) {
		DBGC ( http, "HTTP %p could not redirect: %s\n",
		       http, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Handle HTTP Content-Length header
 *
 * @v http		HTTP request
 * @v value		HTTP header value
 * @ret rc		Return status code
 */
static int http_rx_content_length ( struct http_request *http, char *value ) {
	struct block_device_capacity capacity;
	size_t content_len;
	char *endp;

	/* Parse content length */
	content_len = strtoul ( value, &endp, 10 );
	if ( ! ( ( *endp == '\0' ) || isspace ( *endp ) ) ) {
		DBGC ( http, "HTTP %p invalid Content-Length \"%s\"\n",
		       http, value );
		return -EINVAL_CONTENT_LENGTH;
	}

	/* If we already have an expected content length, and this
	 * isn't it, then complain
	 */
	if ( http->remaining && ( http->remaining != content_len ) ) {
		DBGC ( http, "HTTP %p incorrect Content-Length %zd (expected "
		       "%zd)\n", http, content_len, http->remaining );
		return -EIO_CONTENT_LENGTH;
	}
	if ( ! ( http->flags & HTTP_HEAD_ONLY ) )
		http->remaining = content_len;

	/* Do nothing more if we are retrying the request */
	if ( http->flags & HTTP_TRY_AGAIN )
		return 0;

	/* Use seek() to notify recipient of filesize */
	xfer_seek ( &http->xfer, http->remaining );
	xfer_seek ( &http->xfer, 0 );

	/* Report block device capacity if applicable */
	if ( http->flags & HTTP_HEAD_ONLY ) {
		capacity.blocks = ( content_len / HTTP_BLKSIZE );
		capacity.blksize = HTTP_BLKSIZE;
		capacity.max_count = -1U;
		block_capacity ( &http->partial, &capacity );
	}
	return 0;
}

/**
 * Handle HTTP Transfer-Encoding header
 *
 * @v http		HTTP request
 * @v value		HTTP header value
 * @ret rc		Return status code
 */
static int http_rx_transfer_encoding ( struct http_request *http, char *value ){

	if ( strcasecmp ( value, "chunked" ) == 0 ) {
		/* Mark connection as using chunked transfer encoding */
		http->chunked = 1;
	}

	return 0;
}

/**
 * Handle HTTP Connection header
 *
 * @v http		HTTP request
 * @v value		HTTP header value
 * @ret rc		Return status code
 */
static int http_rx_connection ( struct http_request *http, char *value ) {

	if ( strcasecmp ( value, "keep-alive" ) == 0 ) {
		/* Mark connection as being kept alive by the server */
		http->flags |= HTTP_SERVER_KEEPALIVE;
	}

	return 0;
}

/**
 * Handle WWW-Authenticate Basic header
 *
 * @v http		HTTP request
 * @v params		Parameters
 * @ret rc		Return status code
 */
static int http_rx_basic_auth ( struct http_request *http, char *params ) {

	DBGC ( http, "HTTP %p Basic authentication required (%s)\n",
	       http, params );

	/* If we received a 401 Unauthorized response, then retry
	 * using Basic authentication
	 */
	if ( ( http->code == 401 ) &&
	     ( ! ( http->flags & HTTP_BASIC_AUTH ) ) &&
	     ( http->uri->user != NULL ) ) {
		http->flags |= ( HTTP_TRY_AGAIN | HTTP_BASIC_AUTH );
	}

	return 0;
}

/**
 * Parse Digest authentication parameter
 *
 * @v params		Parameters
 * @v name		Parameter name (including trailing "=\"")
 * @ret value		Parameter value, or NULL
 */
static char * http_digest_param ( char *params, const char *name ) {
	char *key;
	char *value;
	char *terminator;

	/* Locate parameter */
	key = strstr ( params, name );
	if ( ! key )
		return NULL;

	/* Extract value */
	value = ( key + strlen ( name ) );
	terminator = strchr ( value, '"' );
	if ( ! terminator )
		return NULL;
	return strndup ( value, ( terminator - value ) );
}

/**
 * Handle WWW-Authenticate Digest header
 *
 * @v http		HTTP request
 * @v params		Parameters
 * @ret rc		Return status code
 */
static int http_rx_digest_auth ( struct http_request *http, char *params ) {

	DBGC ( http, "HTTP %p Digest authentication required (%s)\n",
	       http, params );

	/* If we received a 401 Unauthorized response, then retry
	 * using Digest authentication
	 */
	if ( ( http->code == 401 ) &&
	     ( ! ( http->flags & HTTP_DIGEST_AUTH ) ) &&
	     ( http->uri->user != NULL ) ) {

		/* Extract realm */
		free ( http->auth_realm );
		http->auth_realm = http_digest_param ( params, "realm=\"" );
		if ( ! http->auth_realm ) {
			DBGC ( http, "HTTP %p Digest prompt missing realm\n",
			       http );
			return -EINVAL_HEADER;
		}

		/* Extract nonce */
		free ( http->auth_nonce );
		http->auth_nonce = http_digest_param ( params, "nonce=\"" );
		if ( ! http->auth_nonce ) {
			DBGC ( http, "HTTP %p Digest prompt missing nonce\n",
			       http );
			return -EINVAL_HEADER;
		}

		/* Extract opaque */
		free ( http->auth_opaque );
		http->auth_opaque = http_digest_param ( params, "opaque=\"" );
		if ( ! http->auth_opaque ) {
			/* Not an error; "opaque" is optional */
		}

		http->flags |= ( HTTP_TRY_AGAIN | HTTP_DIGEST_AUTH );
	}

	return 0;
}

/** An HTTP WWW-Authenticate header handler */
struct http_auth_header_handler {
	/** Scheme (e.g. "Basic") */
	const char *scheme;
	/** Handle received parameters
	 *
	 * @v http	HTTP request
	 * @v params	Parameters
	 * @ret rc	Return status code
	 */
	int ( * rx ) ( struct http_request *http, char *params );
};

/** List of HTTP WWW-Authenticate header handlers */
static struct http_auth_header_handler http_auth_header_handlers[] = {
	{
		.scheme = "Basic",
		.rx = http_rx_basic_auth,
	},
	{
		.scheme = "Digest",
		.rx = http_rx_digest_auth,
	},
	{ NULL, NULL },
};

/**
 * Handle HTTP WWW-Authenticate header
 *
 * @v http		HTTP request
 * @v value		HTTP header value
 * @ret rc		Return status code
 */
static int http_rx_www_authenticate ( struct http_request *http, char *value ) {
	struct http_auth_header_handler *handler;
	char *separator;
	char *scheme;
	char *params;
	int rc;

	/* Extract scheme */
	separator = strchr ( value, ' ' );
	if ( ! separator ) {
		DBGC ( http, "HTTP %p malformed WWW-Authenticate header\n",
		       http );
		return -EINVAL_HEADER;
	}
	*separator = '\0';
	scheme = value;
	params = ( separator + 1 );

	/* Hand off to header handler, if one exists */
	for ( handler = http_auth_header_handlers; handler->scheme; handler++ ){
		if ( strcasecmp ( scheme, handler->scheme ) == 0 ) {
			if ( ( rc = handler->rx ( http, params ) ) != 0 )
				return rc;
			break;
		}
	}
	return 0;
}

/**
 * Handle HTTP Retry-After header
 *
 * @v http		HTTP request
 * @v value		HTTP header value
 * @ret rc		Return status code
 */
static int http_rx_retry_after ( struct http_request *http, char *value ) {
	unsigned long seconds;
	char *endp;

	DBGC ( http, "HTTP %p retry requested (%s)\n", http, value );

	/* If we received a 503 Service Unavailable response, then
	 * retry after the specified number of seconds.  If the value
	 * is not a simple number of seconds (e.g. a full HTTP date),
	 * then retry after a fixed delay, since we don't have code
	 * able to parse full HTTP dates.
	 */
	if ( http->code == 503 ) {
		seconds = strtoul ( value, &endp, 10 );
		if ( *endp != '\0' ) {
			seconds = HTTP_RETRY_SECONDS;
			DBGC ( http, "HTTP %p cannot understand \"%s\"; "
			       "using %ld seconds\n", http, value, seconds );
		}
		http->flags |= HTTP_TRY_AGAIN;
		http->retry_delay = ( seconds * TICKS_PER_SEC );
	}

	return 0;
}

/** An HTTP header handler */
struct http_header_handler {
	/** Name (e.g. "Content-Length") */
	const char *header;
	/** Handle received header
	 *
	 * @v http	HTTP request
	 * @v value	HTTP header value
	 * @ret rc	Return status code
	 *
	 * If an error is returned, the download will be aborted.
	 */
	int ( * rx ) ( struct http_request *http, char *value );
};

/** List of HTTP header handlers */
static struct http_header_handler http_header_handlers[] = {
	{
		.header = "Location",
		.rx = http_rx_location,
	},
	{
		.header = "Content-Length",
		.rx = http_rx_content_length,
	},
	{
		.header = "Transfer-Encoding",
		.rx = http_rx_transfer_encoding,
	},
	{
		.header = "Connection",
		.rx = http_rx_connection,
	},
	{
		.header = "WWW-Authenticate",
		.rx = http_rx_www_authenticate,
	},
	{
		.header = "Retry-After",
		.rx = http_rx_retry_after,
	},
	{ NULL, NULL }
};

/**
 * Handle HTTP header
 *
 * @v http		HTTP request
 * @v header		HTTP header
 * @ret rc		Return status code
 */
static int http_rx_header ( struct http_request *http, char *header ) {
	struct http_header_handler *handler;
	char *separator;
	char *value;
	int rc;

	/* An empty header line marks the end of this phase */
	if ( ! header[0] ) {
		empty_line_buffer ( &http->linebuf );

		/* Handle response code */
		if ( ! ( http->flags & HTTP_TRY_AGAIN ) ) {
			if ( ( rc = http_response_to_rc ( http->code ) ) != 0 )
				return rc;
		}

		/* Move to next state */
		if ( http->rx_state == HTTP_RX_HEADER ) {
			DBGC ( http, "HTTP %p start of data\n", http );
			http->rx_state = ( http->chunked ?
					   HTTP_RX_CHUNK_LEN : HTTP_RX_DATA );
			if ( ( http->partial_len != 0 ) &&
			     ( ! ( http->flags & HTTP_TRY_AGAIN ) ) ) {
				http->remaining = http->partial_len;
			}
			return 0;
		} else {
			DBGC ( http, "HTTP %p end of trailer\n", http );
			http_done ( http );
			return 0;
		}
	}

	DBGC ( http, "HTTP %p header \"%s\"\n", http, header );

	/* Split header at the ": " */
	separator = strstr ( header, ": " );
	if ( ! separator ) {
		DBGC ( http, "HTTP %p malformed header\n", http );
		return -EINVAL_HEADER;
	}
	*separator = '\0';
	value = ( separator + 2 );

	/* Hand off to header handler, if one exists */
	for ( handler = http_header_handlers ; handler->header ; handler++ ) {
		if ( strcasecmp ( header, handler->header ) == 0 ) {
			if ( ( rc = handler->rx ( http, value ) ) != 0 )
				return rc;
			break;
		}
	}
	return 0;
}

/**
 * Handle HTTP chunk length
 *
 * @v http		HTTP request
 * @v length		HTTP chunk length
 * @ret rc		Return status code
 */
static int http_rx_chunk_len ( struct http_request *http, char *length ) {
	char *endp;

	/* Skip blank lines between chunks */
	if ( length[0] == '\0' )
		return 0;

	/* Parse chunk length */
	http->chunk_remaining = strtoul ( length, &endp, 16 );
	if ( *endp != '\0' ) {
		DBGC ( http, "HTTP %p invalid chunk length \"%s\"\n",
		       http, length );
		return -EINVAL_CHUNK_LENGTH;
	}

	/* Terminate chunked encoding if applicable */
	if ( http->chunk_remaining == 0 ) {
		DBGC ( http, "HTTP %p end of chunks\n", http );
		http->chunked = 0;
		http->rx_state = HTTP_RX_TRAILER;
		return 0;
	}

	/* Use seek() to notify recipient of new filesize */
	DBGC ( http, "HTTP %p start of chunk of length %zd\n",
	       http, http->chunk_remaining );
	if ( ! ( http->flags & HTTP_TRY_AGAIN ) ) {
		xfer_seek ( &http->xfer,
			    ( http->rx_len + http->chunk_remaining ) );
		xfer_seek ( &http->xfer, http->rx_len );
	}

	/* Start receiving data */
	http->rx_state = HTTP_RX_DATA;

	return 0;
}

/** An HTTP line-based data handler */
struct http_line_handler {
	/** Handle line
	 *
	 * @v http	HTTP request
	 * @v line	Line to handle
	 * @ret rc	Return status code
	 */
	int ( * rx ) ( struct http_request *http, char *line );
};

/** List of HTTP line-based data handlers */
static struct http_line_handler http_line_handlers[] = {
	[HTTP_RX_RESPONSE]	= { .rx = http_rx_response },
	[HTTP_RX_HEADER]	= { .rx = http_rx_header },
	[HTTP_RX_CHUNK_LEN]	= { .rx = http_rx_chunk_len },
	[HTTP_RX_TRAILER]	= { .rx = http_rx_header },
};

/**
 * Handle new data arriving via HTTP connection
 *
 * @v http		HTTP request
 * @v iobuf		I/O buffer
 * @v meta		Data transfer metadata
 * @ret rc		Return status code
 */
static int http_socket_deliver ( struct http_request *http,
				 struct io_buffer *iobuf,
				 struct xfer_metadata *meta __unused ) {
	struct http_line_handler *lh;
	char *line;
	size_t data_len;
	ssize_t line_len;
	int rc = 0;

	profile_start ( &http_rx_profiler );
	while ( iobuf && iob_len ( iobuf ) ) {

		switch ( http->rx_state ) {
		case HTTP_RX_IDLE:
			/* Receiving any data in this state is an error */
			DBGC ( http, "HTTP %p received %zd bytes while %s\n",
			       http, iob_len ( iobuf ),
			       ( ( http->rx_state == HTTP_RX_IDLE ) ?
				 "idle" : "dead" ) );
			rc = -EPROTO_UNSOLICITED;
			goto done;
		case HTTP_RX_DEAD:
			/* Do no further processing */
			goto done;
		case HTTP_RX_DATA:
			/* Pass received data to caller */
			data_len = iob_len ( iobuf );
			if ( http->chunk_remaining &&
			     ( http->chunk_remaining < data_len ) ) {
				data_len = http->chunk_remaining;
			}
			if ( http->remaining &&
			     ( http->remaining < data_len ) ) {
				data_len = http->remaining;
			}
			if ( http->flags & HTTP_TRY_AGAIN ) {
				/* Discard all received data */
				iob_pull ( iobuf, data_len );
			} else if ( http->rx_buffer != UNULL ) {
				/* Copy to partial transfer buffer */
				copy_to_user ( http->rx_buffer, http->rx_len,
					       iobuf->data, data_len );
				iob_pull ( iobuf, data_len );
			} else if ( data_len < iob_len ( iobuf ) ) {
				/* Deliver partial buffer as raw data */
				profile_start ( &http_xfer_profiler );
				rc = xfer_deliver_raw ( &http->xfer,
							iobuf->data, data_len );
				iob_pull ( iobuf, data_len );
				if ( rc != 0 )
					goto done;
				profile_stop ( &http_xfer_profiler );
			} else {
				/* Deliver whole I/O buffer */
				profile_start ( &http_xfer_profiler );
				if ( ( rc = xfer_deliver_iob ( &http->xfer,
						 iob_disown ( iobuf ) ) ) != 0 )
					goto done;
				profile_stop ( &http_xfer_profiler );
			}
			http->rx_len += data_len;
			if ( http->chunk_remaining ) {
				http->chunk_remaining -= data_len;
				if ( http->chunk_remaining == 0 )
					http->rx_state = HTTP_RX_CHUNK_LEN;
			}
			if ( http->remaining ) {
				http->remaining -= data_len;
				if ( ( http->remaining == 0 ) &&
				     ( http->rx_state == HTTP_RX_DATA ) ) {
					http_done ( http );
				}
			}
			break;
		case HTTP_RX_RESPONSE:
		case HTTP_RX_HEADER:
		case HTTP_RX_CHUNK_LEN:
		case HTTP_RX_TRAILER:
			/* In the other phases, buffer and process a
			 * line at a time
			 */
			line_len = line_buffer ( &http->linebuf, iobuf->data,
						 iob_len ( iobuf ) );
			if ( line_len < 0 ) {
				rc = line_len;
				DBGC ( http, "HTTP %p could not buffer line: "
				       "%s\n", http, strerror ( rc ) );
				goto done;
			}
			iob_pull ( iobuf, line_len );
			line = buffered_line ( &http->linebuf );
			if ( line ) {
				lh = &http_line_handlers[http->rx_state];
				if ( ( rc = lh->rx ( http, line ) ) != 0 )
					goto done;
			}
			break;
		default:
			assert ( 0 );
			break;
		}
	}

 done:
	if ( rc )
		http_close ( http, rc );
	free_iob ( iobuf );
	profile_stop ( &http_rx_profiler );
	return rc;
}

/**
 * Check HTTP socket flow control window
 *
 * @v http		HTTP request
 * @ret len		Length of window
 */
static size_t http_socket_window ( struct http_request *http __unused ) {

	/* Window is always open.  This is to prevent TCP from
	 * stalling if our parent window is not currently open.
	 */
	return ( ~( ( size_t ) 0 ) );
}

/**
 * Close HTTP socket
 *
 * @v http		HTTP request
 * @v rc		Reason for close
 */
static void http_socket_close ( struct http_request *http, int rc ) {

	/* If we have an error, terminate */
	if ( rc != 0 ) {
		http_close ( http, rc );
		return;
	}

	/* Mark HTTP request as complete */
	http_done ( http );
}

/**
 * Generate HTTP Basic authorisation string
 *
 * @v http		HTTP request
 * @ret auth		Authorisation string, or NULL on error
 *
 * The authorisation string is dynamically allocated, and must be
 * freed by the caller.
 */
static char * http_basic_auth ( struct http_request *http ) {
	const char *user = http->uri->user;
	const char *password = ( http->uri->password ?
				 http->uri->password : "" );
	size_t user_pw_len =
		( strlen ( user ) + 1 /* ":" */ + strlen ( password ) );
	char user_pw[ user_pw_len + 1 /* NUL */ ];
	size_t user_pw_base64_len = base64_encoded_len ( user_pw_len );
	char user_pw_base64[ user_pw_base64_len + 1 /* NUL */ ];
	char *auth;
	int len;

	/* Sanity check */
	assert ( user != NULL );

	/* Make "user:password" string from decoded fields */
	snprintf ( user_pw, sizeof ( user_pw ), "%s:%s", user, password );

	/* Base64-encode the "user:password" string */
	base64_encode ( ( void * ) user_pw, user_pw_len, user_pw_base64 );

	/* Generate the authorisation string */
	len = asprintf ( &auth, "Authorization: Basic %s\r\n",
			 user_pw_base64 );
	if ( len < 0 )
		return NULL;

	return auth;
}

/**
 * Generate HTTP Digest authorisation string
 *
 * @v http		HTTP request
 * @v method		HTTP method (e.g. "GET")
 * @v uri		HTTP request URI (e.g. "/index.html")
 * @ret auth		Authorisation string, or NULL on error
 *
 * The authorisation string is dynamically allocated, and must be
 * freed by the caller.
 */
static char * http_digest_auth ( struct http_request *http,
				 const char *method, const char *uri ) {
	const char *user = http->uri->user;
	const char *password = ( http->uri->password ?
				 http->uri->password : "" );
	const char *realm = http->auth_realm;
	const char *nonce = http->auth_nonce;
	const char *opaque = http->auth_opaque;
	static const char colon = ':';
	uint8_t ctx[MD5_CTX_SIZE];
	uint8_t digest[MD5_DIGEST_SIZE];
	char ha1[ base16_encoded_len ( sizeof ( digest ) ) + 1 /* NUL */ ];
	char ha2[ base16_encoded_len ( sizeof ( digest ) ) + 1 /* NUL */ ];
	char response[ base16_encoded_len ( sizeof ( digest ) ) + 1 /* NUL */ ];
	char *auth;
	int len;

	/* Sanity checks */
	assert ( user != NULL );
	assert ( realm != NULL );
	assert ( nonce != NULL );

	/* Generate HA1 */
	digest_init ( &md5_algorithm, ctx );
	digest_update ( &md5_algorithm, ctx, user, strlen ( user ) );
	digest_update ( &md5_algorithm, ctx, &colon, sizeof ( colon ) );
	digest_update ( &md5_algorithm, ctx, realm, strlen ( realm ) );
	digest_update ( &md5_algorithm, ctx, &colon, sizeof ( colon ) );
	digest_update ( &md5_algorithm, ctx, password, strlen ( password ) );
	digest_final ( &md5_algorithm, ctx, digest );
	base16_encode ( digest, sizeof ( digest ), ha1 );

	/* Generate HA2 */
	digest_init ( &md5_algorithm, ctx );
	digest_update ( &md5_algorithm, ctx, method, strlen ( method ) );
	digest_update ( &md5_algorithm, ctx, &colon, sizeof ( colon ) );
	digest_update ( &md5_algorithm, ctx, uri, strlen ( uri ) );
	digest_final ( &md5_algorithm, ctx, digest );
	base16_encode ( digest, sizeof ( digest ), ha2 );

	/* Generate response */
	digest_init ( &md5_algorithm, ctx );
	digest_update ( &md5_algorithm, ctx, ha1, strlen ( ha1 ) );
	digest_update ( &md5_algorithm, ctx, &colon, sizeof ( colon ) );
	digest_update ( &md5_algorithm, ctx, nonce, strlen ( nonce ) );
	digest_update ( &md5_algorithm, ctx, &colon, sizeof ( colon ) );
	digest_update ( &md5_algorithm, ctx, ha2, strlen ( ha2 ) );
	digest_final ( &md5_algorithm, ctx, digest );
	base16_encode ( digest, sizeof ( digest ), response );

	/* Generate the authorisation string */
	len = asprintf ( &auth, "Authorization: Digest username=\"%s\", "
			 "realm=\"%s\", nonce=\"%s\", uri=\"%s\", "
			 "%s%s%sresponse=\"%s\"\r\n", user, realm, nonce, uri,
			 ( opaque ? "opaque=\"" : "" ),
			 ( opaque ? opaque : "" ),
			 ( opaque ? "\", " : "" ), response );
	if ( len < 0 )
		return NULL;

	return auth;
}

/**
 * Generate HTTP POST parameter list
 *
 * @v http		HTTP request
 * @v buf		Buffer to contain HTTP POST parameters
 * @v len		Length of buffer
 * @ret len		Length of parameter list (excluding terminating NUL)
 */
static size_t http_post_params ( struct http_request *http,
				 char *buf, size_t len ) {
	struct parameter *param;
	ssize_t remaining = len;
	size_t frag_len;

	/* Add each parameter in the form "key=value", joined with "&" */
	len = 0;
	for_each_param ( param, http->uri->params ) {

		/* Add the "&", if applicable */
		if ( len ) {
			if ( remaining > 0 )
				*buf = '&';
			buf++;
			len++;
			remaining--;
		}

		/* URI-encode the key */
		frag_len = uri_encode ( param->key, 0, buf, remaining );
		buf += frag_len;
		len += frag_len;
		remaining -= frag_len;

		/* Add the "=" */
		if ( remaining > 0 )
			*buf = '=';
		buf++;
		len++;
		remaining--;

		/* URI-encode the value */
		frag_len = uri_encode ( param->value, 0, buf, remaining );
		buf += frag_len;
		len += frag_len;
		remaining -= frag_len;
	}

	/* Ensure string is NUL-terminated even if no parameters are present */
	if ( remaining > 0 )
		*buf = '\0';

	return len;
}

/**
 * Generate HTTP POST body
 *
 * @v http		HTTP request
 * @ret post		I/O buffer containing POST body, or NULL on error
 */
static struct io_buffer * http_post ( struct http_request *http ) {
	struct io_buffer *post;
	size_t len;
	size_t check_len;

	/* Calculate length of parameter list */
	len = http_post_params ( http, NULL, 0 );

	/* Allocate parameter list */
	post = alloc_iob ( len + 1 /* NUL */ );
	if ( ! post )
		return NULL;

	/* Fill parameter list */
	check_len = http_post_params ( http, iob_put ( post, len ),
				       ( len + 1 /* NUL */ ) );
	assert ( len == check_len );
	DBGC ( http, "HTTP %p POST %s\n", http, ( ( char * ) post->data ) );

	return post;
}

/**
 * HTTP process
 *
 * @v http		HTTP request
 */
static void http_step ( struct http_request *http ) {
	struct io_buffer *post;
	struct uri host_uri;
	struct uri path_uri;
	char *host_uri_string;
	char *path_uri_string;
	char *method;
	char *range;
	char *auth;
	char *content;
	int len;
	int rc;

	/* Do nothing if we have already transmitted the request */
	if ( ! ( http->flags & HTTP_TX_PENDING ) )
		return;

	/* Do nothing until socket is ready */
	if ( ! xfer_window ( &http->socket ) )
		return;

	/* Force a HEAD request if we have nowhere to send any received data */
	if ( ( xfer_window ( &http->xfer ) == 0 ) &&
	     ( http->rx_buffer == UNULL ) ) {
		http->flags |= ( HTTP_HEAD_ONLY | HTTP_CLIENT_KEEPALIVE );
	}

	/* Determine method */
	method = ( ( http->flags & HTTP_HEAD_ONLY ) ? "HEAD" :
		   ( http->uri->params ? "POST" : "GET" ) );

	/* Construct host URI */
	memset ( &host_uri, 0, sizeof ( host_uri ) );
	host_uri.host = http->uri->host;
	host_uri.port = http->uri->port;
	host_uri_string = format_uri_alloc ( &host_uri );
	if ( ! host_uri_string ) {
		rc = -ENOMEM;
		goto err_host_uri;
	}

	/* Construct path URI */
	memset ( &path_uri, 0, sizeof ( path_uri ) );
	path_uri.path = ( http->uri->path ? http->uri->path : "/" );
	path_uri.query = http->uri->query;
	path_uri_string = format_uri_alloc ( &path_uri );
	if ( ! path_uri_string ) {
		rc = -ENOMEM;
		goto err_path_uri;
	}

	/* Calculate range request parameters if applicable */
	if ( http->partial_len ) {
		len = asprintf ( &range, "Range: bytes=%zd-%zd\r\n",
				 http->partial_start,
				 ( http->partial_start + http->partial_len
				   - 1 ) );
		if ( len < 0 ) {
			rc = len;
			goto err_range;
		}
	} else {
		range = NULL;
	}

	/* Construct authorisation, if applicable */
	if ( http->flags & HTTP_BASIC_AUTH ) {
		auth = http_basic_auth ( http );
		if ( ! auth ) {
			rc = -ENOMEM;
			goto err_auth;
		}
	} else if ( http->flags & HTTP_DIGEST_AUTH ) {
		auth = http_digest_auth ( http, method, path_uri_string );
		if ( ! auth ) {
			rc = -ENOMEM;
			goto err_auth;
		}
	} else {
		auth = NULL;
	}

	/* Construct POST content, if applicable */
	if ( http->uri->params ) {
		post = http_post ( http );
		if ( ! post ) {
			rc = -ENOMEM;
			goto err_post;
		}
		len = asprintf ( &content, "Content-Type: "
				 "application/x-www-form-urlencoded\r\n"
				 "Content-Length: %zd\r\n", iob_len ( post ) );
		if ( len < 0 ) {
			rc = len;
			goto err_content;
		}
	} else {
		post = NULL;
		content = NULL;
	}

	/* Mark request as transmitted */
	http->flags &= ~HTTP_TX_PENDING;

	/* Send request */
	if ( ( rc = xfer_printf ( &http->socket,
				  "%s %s HTTP/1.1\r\n"
				  "User-Agent: iPXE/%s\r\n"
				  "Host: %s\r\n"
				  "%s%s%s%s"
				  "\r\n",
				  method, path_uri_string, product_version,
				  host_uri_string,
				  ( ( http->flags & HTTP_CLIENT_KEEPALIVE ) ?
				    "Connection: keep-alive\r\n" : "" ),
				  ( range ? range : "" ),
				  ( auth ? auth : "" ),
				  ( content ? content : "" ) ) ) != 0 ) {
		goto err_xfer;
	}

	/* Send POST content, if applicable */
	if ( post ) {
		if ( ( rc = xfer_deliver_iob ( &http->socket,
					       iob_disown ( post ) ) ) != 0 )
			goto err_xfer_post;
	}

 err_xfer_post:
 err_xfer:
	free ( content );
 err_content:
	free ( post );
 err_post:
	free ( auth );
 err_auth:
	free ( range );
 err_range:
	free ( path_uri_string );
 err_path_uri:
	free ( host_uri_string );
 err_host_uri:
	if ( rc != 0 )
		http_close ( http, rc );
}

/**
 * Check HTTP data transfer flow control window
 *
 * @v http		HTTP request
 * @ret len		Length of window
 */
static size_t http_xfer_window ( struct http_request *http ) {

	/* New block commands may be issued only when we are idle */
	return ( ( http->rx_state == HTTP_RX_IDLE ) ? 1 : 0 );
}

/**
 * Initiate HTTP partial read
 *
 * @v http		HTTP request
 * @v partial		Partial transfer interface
 * @v offset		Starting offset
 * @v buffer		Data buffer
 * @v len		Length
 * @ret rc		Return status code
 */
static int http_partial_read ( struct http_request *http,
			       struct interface *partial,
			       size_t offset, userptr_t buffer, size_t len ) {

	/* Sanity check */
	if ( http_xfer_window ( http ) == 0 )
		return -EBUSY;

	/* Initialise partial transfer parameters */
	http->rx_buffer = buffer;
	http->partial_start = offset;
	http->partial_len = len;

	/* Schedule request */
	http->rx_state = HTTP_RX_RESPONSE;
	http->flags = ( HTTP_TX_PENDING | HTTP_CLIENT_KEEPALIVE );
	if ( ! len )
		http->flags |= HTTP_HEAD_ONLY;
	process_add ( &http->process );

	/* Attach to parent interface and return */
	intf_plug_plug ( &http->partial, partial );

	return 0;
}

/**
 * Issue HTTP block device read
 *
 * @v http		HTTP request
 * @v block		Block data interface
 * @v lba		Starting logical block address
 * @v count		Number of blocks to transfer
 * @v buffer		Data buffer
 * @v len		Length of data buffer
 * @ret rc		Return status code
 */
static int http_block_read ( struct http_request *http,
			     struct interface *block,
			     uint64_t lba, unsigned int count,
			     userptr_t buffer, size_t len __unused ) {

	return http_partial_read ( http, block, ( lba * HTTP_BLKSIZE ),
				   buffer, ( count * HTTP_BLKSIZE ) );
}

/**
 * Read HTTP block device capacity
 *
 * @v http		HTTP request
 * @v block		Block data interface
 * @ret rc		Return status code
 */
static int http_block_read_capacity ( struct http_request *http,
				      struct interface *block ) {

	return http_partial_read ( http, block, 0, 0, 0 );
}

/**
 * Describe HTTP device in an ACPI table
 *
 * @v http		HTTP request
 * @v acpi		ACPI table
 * @v len		Length of ACPI table
 * @ret rc		Return status code
 */
static int http_acpi_describe ( struct http_request *http,
				struct acpi_description_header *acpi,
				size_t len ) {

	DBGC ( http, "HTTP %p cannot yet describe device in an ACPI table\n",
	       http );
	( void ) acpi;
	( void ) len;
	return 0;
}

/** HTTP socket interface operations */
static struct interface_operation http_socket_operations[] = {
	INTF_OP ( xfer_window, struct http_request *, http_socket_window ),
	INTF_OP ( xfer_deliver, struct http_request *, http_socket_deliver ),
	INTF_OP ( xfer_window_changed, struct http_request *, http_step ),
	INTF_OP ( intf_close, struct http_request *, http_socket_close ),
};

/** HTTP socket interface descriptor */
static struct interface_descriptor http_socket_desc =
	INTF_DESC_PASSTHRU ( struct http_request, socket,
			     http_socket_operations, xfer );

/** HTTP partial transfer interface operations */
static struct interface_operation http_partial_operations[] = {
	INTF_OP ( intf_close, struct http_request *, http_close ),
};

/** HTTP partial transfer interface descriptor */
static struct interface_descriptor http_partial_desc =
	INTF_DESC ( struct http_request, partial, http_partial_operations );

/** HTTP data transfer interface operations */
static struct interface_operation http_xfer_operations[] = {
	INTF_OP ( xfer_window, struct http_request *, http_xfer_window ),
	INTF_OP ( block_read, struct http_request *, http_block_read ),
	INTF_OP ( block_read_capacity, struct http_request *,
		  http_block_read_capacity ),
	INTF_OP ( intf_close, struct http_request *, http_close ),
	INTF_OP ( acpi_describe, struct http_request *, http_acpi_describe ),
};

/** HTTP data transfer interface descriptor */
static struct interface_descriptor http_xfer_desc =
	INTF_DESC_PASSTHRU ( struct http_request, xfer,
			     http_xfer_operations, socket );

/** HTTP process descriptor */
static struct process_descriptor http_process_desc =
	PROC_DESC_ONCE ( struct http_request, process, http_step );

/**
 * Initiate an HTTP connection, with optional filter
 *
 * @v xfer		Data transfer interface
 * @v uri		Uniform Resource Identifier
 * @v default_port	Default port number
 * @v filter		Filter to apply to socket, or NULL
 * @ret rc		Return status code
 */
int http_open_filter ( struct interface *xfer, struct uri *uri,
		       unsigned int default_port,
		       int ( * filter ) ( struct interface *xfer,
					  const char *name,
					  struct interface **next ) ) {
	struct http_request *http;
	int rc;

	/* Sanity checks */
	if ( ! uri->host )
		return -EINVAL;

	/* Allocate and populate HTTP structure */
	http = zalloc ( sizeof ( *http ) );
	if ( ! http )
		return -ENOMEM;
	ref_init ( &http->refcnt, http_free );
	intf_init ( &http->xfer, &http_xfer_desc, &http->refcnt );
	intf_init ( &http->partial, &http_partial_desc, &http->refcnt );
	http->uri = uri_get ( uri );
	http->default_port = default_port;
	http->filter = filter;
	intf_init ( &http->socket, &http_socket_desc, &http->refcnt );
	process_init ( &http->process, &http_process_desc, &http->refcnt );
	timer_init ( &http->timer, http_retry, &http->refcnt );
	http->flags = HTTP_TX_PENDING;

	/* Open socket */
	if ( ( rc = http_socket_open ( http ) ) != 0 )
		goto err;

	/* Attach to parent interface, mortalise self, and return */
	intf_plug_plug ( &http->xfer, xfer );
	ref_put ( &http->refcnt );
	return 0;

 err:
	DBGC ( http, "HTTP %p could not create request: %s\n",
	       http, strerror ( rc ) );
	http_close ( http, rc );
	ref_put ( &http->refcnt );
	return rc;
}

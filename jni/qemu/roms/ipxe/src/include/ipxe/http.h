#ifndef _IPXE_HTTP_H
#define _IPXE_HTTP_H

/** @file
 *
 * Hyper Text Transport Protocol
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

/** HTTP default port */
#define HTTP_PORT 80

/** HTTPS default port */
#define HTTPS_PORT 443

extern int http_open_filter ( struct interface *xfer, struct uri *uri,
			      unsigned int default_port,
			      int ( * filter ) ( struct interface *,
						 const char *,
						 struct interface ** ) );

#endif /* _IPXE_HTTP_H */

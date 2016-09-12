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
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/** @file
 *
 * Uniform Resource Identifiers
 *
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <ctype.h>
#include <ipxe/vsprintf.h>
#include <ipxe/params.h>
#include <ipxe/tcpip.h>
#include <ipxe/uri.h>

/**
 * Decode URI field
 *
 * @v encoded		Encoded field
 * @v buf		Data buffer
 * @v len		Length
 * @ret len		Length of data
 *
 * URI decoding can never increase the length of a string; we can
 * therefore safely decode in place.
 */
size_t uri_decode ( const char *encoded, void *buf, size_t len ) {
	uint8_t *out = buf;
	unsigned int count = 0;
	char hexbuf[3];
	char *hexbuf_end;
	char c;
	char decoded;
	unsigned int skip;

	/* Copy string, decoding escaped characters as necessary */
	while ( ( c = *(encoded++) ) ) {
		if ( c == '%' ) {
			snprintf ( hexbuf, sizeof ( hexbuf ), "%s", encoded );
			decoded = strtoul ( hexbuf, &hexbuf_end, 16 );
			skip = ( hexbuf_end - hexbuf );
			encoded += skip;
			if ( skip )
				c = decoded;
		}
		if ( count < len )
			out[count] = c;
		count++;
	}
	return count;
}

/**
 * Decode URI field in-place
 *
 * @v uri		URI
 * @v field		URI field index
 */
static void uri_decode_inplace ( struct uri *uri, unsigned int field ) {
	const char *encoded = uri_field ( uri, field );
	char *decoded = ( ( char * ) encoded );
	size_t len;

	/* Do nothing if field is not present */
	if ( ! encoded )
		return;

	/* Decode field in place */
	len = uri_decode ( encoded, decoded, strlen ( encoded ) );

	/* Terminate decoded string */
	decoded[len] = '\0';
}

/**
 * Check if character should be escaped within a URI field
 *
 * @v c			Character
 * @v field		URI field index
 * @ret escaped		Character should be escaped
 */
static int uri_character_escaped ( char c, unsigned int field ) {

	/* Non-printing characters and whitespace should always be
	 * escaped, since they cannot sensibly be displayed as part of
	 * a coherent URL string.  (This test also catches control
	 * characters such as CR and LF, which could affect the
	 * operation of line-based protocols such as HTTP.)
	 *
	 * We should also escape characters which would alter the
	 * interpretation of the URL if not escaped, i.e. characters
	 * which have significance to the URL parser.  We should not
	 * blindly escape all such characters, because this would lead
	 * to some very strange-looking URLs (e.g. if we were to
	 * always escape '/' as "%2F" even within the URI path).
	 *
	 * We do not need to be perfect.  Our primary role is as a
	 * consumer of URIs rather than a producer; the main situation
	 * in which we produce a URI string is for display to a human
	 * user, who can probably tolerate some variance from the
	 * formal specification.  The only situation in which we
	 * currently produce a URI string to be consumed by a computer
	 * is when constructing an HTTP request URI, which contains
	 * only the path and query fields.
	 *
	 * We can therefore sacrifice some correctness for the sake of
	 * code size.  For example, colons within the URI host should
	 * be escaped unless they form part of an IPv6 literal
	 * address; doing this correctly would require the URI
	 * formatter to be aware of whether or not the URI host
	 * contained an IPv4 address, an IPv6 address, or a host name.
	 * We choose to simplify and never escape colons within the
	 * URI host field: in the event of a pathological hostname
	 * containing colons, this could potentially produce a URI
	 * string which could not be reparsed.
	 *
	 * After excluding non-printing characters, whitespace, and
	 * '%', the full set of characters with significance to the
	 * URL parser is "/#:@?".  We choose for each URI field which
	 * of these require escaping in our use cases.
	 *
	 * For the scheme field (equivalently, if field is zero), we
	 * escape anything that has significance not just for our URI
	 * parser but for any other URI parsers (e.g. HTTP query
	 * string parsers, which care about '=' and '&').
	 */
	static const char *escaped[URI_FIELDS] = {
		/* Scheme or default: escape everything */
		[URI_SCHEME]	= "/#:@?=&",
		/* Opaque part: escape characters which would affect
		 * the reparsing of the URI, allowing everything else
		 * (e.g. ':', which will appear in iSCSI URIs).
		 */
		[URI_OPAQUE]	= "#",
		/* User name: escape everything */
		[URI_USER]	= "/#:@?",
		/* Password: escape everything */
		[URI_PASSWORD]	= "/#:@?",
		/* Host name: escape everything except ':', which may
		 * appear as part of an IPv6 literal address.
		 */
		[URI_HOST]	= "/#@?",
		/* Port number: escape everything */
		[URI_PORT]	= "/#:@?",
		/* Path: escape everything except '/', which usually
		 * appears within paths.
		 */
		[URI_PATH]	= "#:@?",
		/* Query: escape everything except '/', which
		 * sometimes appears within queries.
		 */
		[URI_QUERY]	= "#:@?",
		/* Fragment: escape everything */
		[URI_FRAGMENT]	= "/#:@?",
	};

	return ( /* Always escape non-printing characters and whitespace */
		 ( ! isprint ( c ) ) || ( c == ' ' ) ||
		 /* Always escape '%' */
		 ( c == '%' ) ||
		 /* Escape field-specific characters */
		 strchr ( escaped[field], c ) );
}

/**
 * Encode URI field
 *
 * @v field		URI field index
 * @v raw		Raw data
 * @v raw_len		Length of raw data
 * @v buf		Buffer
 * @v len		Length of buffer
 * @ret len		Length of encoded string (excluding NUL)
 */
size_t uri_encode ( unsigned int field, const void *raw, size_t raw_len,
		    char *buf, ssize_t len ) {
	const uint8_t *raw_bytes = ( ( const uint8_t * ) raw );
	ssize_t remaining = len;
	size_t used;
	char c;

	/* Ensure encoded string is NUL-terminated even if empty */
	if ( len > 0 )
		buf[0] = '\0';

	/* Copy string, escaping as necessary */
	while ( raw_len-- ) {
		c = *(raw_bytes++);
		if ( uri_character_escaped ( c, field ) ) {
			used = ssnprintf ( buf, remaining, "%%%02X", c );
		} else {
			used = ssnprintf ( buf, remaining, "%c", c );
		}
		buf += used;
		remaining -= used;
	}

	return ( len - remaining );
}

/**
 * Encode URI field string
 *
 * @v field		URI field index
 * @v string		String
 * @v buf		Buffer
 * @v len		Length of buffer
 * @ret len		Length of encoded string (excluding NUL)
 */
size_t uri_encode_string ( unsigned int field, const char *string,
			   char *buf, ssize_t len ) {

	return uri_encode ( field, string, strlen ( string ), buf, len );
}

/**
 * Dump URI for debugging
 *
 * @v uri		URI
 */
static void uri_dump ( const struct uri *uri ) {

	if ( ! uri )
		return;
	if ( uri->scheme )
		DBGC ( uri, " scheme \"%s\"", uri->scheme );
	if ( uri->opaque )
		DBGC ( uri, " opaque \"%s\"", uri->opaque );
	if ( uri->user )
		DBGC ( uri, " user \"%s\"", uri->user );
	if ( uri->password )
		DBGC ( uri, " password \"%s\"", uri->password );
	if ( uri->host )
		DBGC ( uri, " host \"%s\"", uri->host );
	if ( uri->port )
		DBGC ( uri, " port \"%s\"", uri->port );
	if ( uri->path )
		DBGC ( uri, " path \"%s\"", uri->path );
	if ( uri->query )
		DBGC ( uri, " query \"%s\"", uri->query );
	if ( uri->fragment )
		DBGC ( uri, " fragment \"%s\"", uri->fragment );
	if ( uri->params )
		DBGC ( uri, " params \"%s\"", uri->params->name );
}

/**
 * Free URI
 *
 * @v refcnt		Reference count
 */
static void uri_free ( struct refcnt *refcnt ) {
	struct uri *uri = container_of ( refcnt, struct uri, refcnt );

	params_put ( uri->params );
	free ( uri );
}

/**
 * Parse URI
 *
 * @v uri_string	URI as a string
 * @ret uri		URI
 *
 * Splits a URI into its component parts.  The return URI structure is
 * dynamically allocated and must eventually be freed by calling
 * uri_put().
 */
struct uri * parse_uri ( const char *uri_string ) {
	struct uri *uri;
	struct parameters *params;
	char *raw;
	char *tmp;
	char *path;
	char *authority;
	size_t raw_len;
	unsigned int field;

	/* Allocate space for URI struct and a copy of the string */
	raw_len = ( strlen ( uri_string ) + 1 /* NUL */ );
	uri = zalloc ( sizeof ( *uri ) + raw_len );
	if ( ! uri )
		return NULL;
	ref_init ( &uri->refcnt, uri_free );
	raw = ( ( ( void * ) uri ) + sizeof ( *uri ) );

	/* Copy in the raw string */
	memcpy ( raw, uri_string, raw_len );

	/* Identify the parameter list, if present */
	if ( ( tmp = strstr ( raw, "##params" ) ) ) {
		*tmp = '\0';
		tmp += 8 /* "##params" */;
		params = find_parameters ( *tmp ? ( tmp + 1 ) : NULL );
		if ( params ) {
			uri->params = claim_parameters ( params );
		} else {
			/* Ignore non-existent submission blocks */
		}
	}

	/* Chop off the fragment, if it exists */
	if ( ( tmp = strchr ( raw, '#' ) ) ) {
		*(tmp++) = '\0';
		uri->fragment = tmp;
	}

	/* Identify absolute/relative URI */
	if ( ( tmp = strchr ( raw, ':' ) ) ) {
		/* Absolute URI: identify hierarchical/opaque */
		uri->scheme = raw;
		*(tmp++) = '\0';
		if ( *tmp == '/' ) {
			/* Absolute URI with hierarchical part */
			path = tmp;
		} else {
			/* Absolute URI with opaque part */
			uri->opaque = tmp;
			path = NULL;
		}
	} else {
		/* Relative URI */
		path = raw;
	}

	/* If we don't have a path (i.e. we have an absolute URI with
	 * an opaque portion, we're already finished processing
	 */
	if ( ! path )
		goto done;

	/* Chop off the query, if it exists */
	if ( ( tmp = strchr ( path, '?' ) ) ) {
		*(tmp++) = '\0';
		uri->query = tmp;
	}

	/* If we have no path remaining, then we're already finished
	 * processing.
	 */
	if ( ! path[0] )
		goto done;

	/* Identify net/absolute/relative path */
	if ( uri->scheme && ( strncmp ( path, "//", 2 ) == 0 ) ) {
		/* Net path.  If this is terminated by the first '/'
		 * of an absolute path, then we have no space for a
		 * terminator after the authority field, so shuffle
		 * the authority down by one byte, overwriting one of
		 * the two slashes.
		 */
		authority = ( path + 2 );
		if ( ( tmp = strchr ( authority, '/' ) ) ) {
			/* Shuffle down */
			uri->path = tmp;
			memmove ( ( authority - 1 ), authority,
				  ( tmp - authority ) );
			authority--;
			*(--tmp) = '\0';
		}
	} else {
		/* Absolute/relative path */
		uri->path = path;
		authority = NULL;
	}

	/* If we don't have an authority (i.e. we have a non-net
	 * path), we're already finished processing
	 */
	if ( ! authority )
		goto done;

	/* Split authority into user[:password] and host[:port] portions */
	if ( ( tmp = strchr ( authority, '@' ) ) ) {
		/* Has user[:password] */
		*(tmp++) = '\0';
		uri->host = tmp;
		uri->user = authority;
		if ( ( tmp = strchr ( authority, ':' ) ) ) {
			/* Has password */
			*(tmp++) = '\0';
			uri->password = tmp;
		}
	} else {
		/* No user:password */
		uri->host = authority;
	}

	/* Split host into host[:port] */
	if ( ( uri->host[ strlen ( uri->host ) - 1 ] != ']' ) &&
	     ( tmp = strrchr ( uri->host, ':' ) ) ) {
		*(tmp++) = '\0';
		uri->port = tmp;
	}

 done:
	/* Decode fields in-place */
	for ( field = 0 ; field < URI_FIELDS ; field++ )
		uri_decode_inplace ( uri, field );

	DBGC ( uri, "URI parsed \"%s\" to", uri_string );
	uri_dump ( uri );
	DBGC ( uri, "\n" );

	return uri;
}

/**
 * Get port from URI
 *
 * @v uri		URI, or NULL
 * @v default_port	Default port to use if none specified in URI
 * @ret port		Port
 */
unsigned int uri_port ( const struct uri *uri, unsigned int default_port ) {

	if ( ( ! uri ) || ( ! uri->port ) )
		return default_port;

	return ( strtoul ( uri->port, NULL, 0 ) );
}

/**
 * Format URI
 *
 * @v uri		URI
 * @v buf		Buffer to fill with URI string
 * @v size		Size of buffer
 * @ret len		Length of URI string
 */
size_t format_uri ( const struct uri *uri, char *buf, size_t len ) {
	static const char prefixes[URI_FIELDS] = {
		[URI_PASSWORD] = ':',
		[URI_PORT] = ':',
		[URI_QUERY] = '?',
		[URI_FRAGMENT] = '#',
	};
	char prefix;
	size_t used = 0;
	unsigned int field;

	/* Ensure buffer is NUL-terminated */
	if ( len )
		buf[0] = '\0';

	/* Special-case NULL URI */
	if ( ! uri )
		return 0;

	/* Generate fields */
	for ( field = 0 ; field < URI_FIELDS ; field++ ) {

		/* Skip non-existent fields */
		if ( ! uri_field ( uri, field ) )
			continue;

		/* Prefix this field, if applicable */
		prefix = prefixes[field];
		if ( ( field == URI_HOST ) && ( uri->user != NULL ) )
			prefix = '@';
		if ( prefix ) {
			used += ssnprintf ( ( buf + used ), ( len - used ),
					    "%c", prefix );
		}

		/* Encode this field */
		used += uri_encode_string ( field, uri_field ( uri, field ),
					    ( buf + used ), ( len - used ) );

		/* Suffix this field, if applicable */
		if ( field == URI_SCHEME ) {
			used += ssnprintf ( ( buf + used ), ( len - used ),
					    ":%s", ( uri->host ? "//" : "" ) );
		}
	}

	if ( len ) {
		DBGC ( uri, "URI formatted" );
		uri_dump ( uri );
		DBGC ( uri, " to \"%s%s\"\n", buf,
		       ( ( used > len ) ? "<TRUNCATED>" : "" ) );
	}

	return used;
}

/**
 * Format URI
 *
 * @v uri		URI
 * @ret string		URI string, or NULL on failure
 *
 * The caller is responsible for eventually freeing the allocated
 * memory.
 */
char * format_uri_alloc ( const struct uri *uri ) {
	size_t len;
	char *string;

	len = ( format_uri ( uri, NULL, 0 ) + 1 /* NUL */ );
	string = malloc ( len );
	if ( string )
		format_uri ( uri, string, len );
	return string;
}

/**
 * Copy URI fields
 *
 * @v src		Source URI
 * @v dest		Destination URI, or NULL to calculate length
 * @ret len		Length of raw URI
 */
static size_t uri_copy_fields ( const struct uri *src, struct uri *dest ) {
	size_t len = sizeof ( *dest );
	char *out = ( ( void * ) dest + len );
	unsigned int field;
	size_t field_len;

	/* Copy existent fields */
	for ( field = 0 ; field < URI_FIELDS ; field++ ) {

		/* Skip non-existent fields */
		if ( ! uri_field ( src, field ) )
			continue;

		/* Calculate field length */
		field_len = ( strlen ( uri_field ( src, field ) )
			      + 1 /* NUL */ );
		len += field_len;

		/* Copy field, if applicable */
		if ( dest ) {
			memcpy ( out, uri_field ( src, field ), field_len );
			uri_field ( dest, field ) = out;
			out += field_len;
		}
	}
	return len;
}

/**
 * Duplicate URI
 *
 * @v uri		URI
 * @ret uri		Duplicate URI
 *
 * Creates a modifiable copy of a URI.
 */
struct uri * uri_dup ( const struct uri *uri ) {
	struct uri *dup;
	size_t len;

	/* Allocate new URI */
	len = uri_copy_fields ( uri, NULL );
	dup = zalloc ( len );
	if ( ! dup )
		return NULL;
	ref_init ( &dup->refcnt, uri_free );

	/* Copy fields */
	uri_copy_fields ( uri, dup );

	/* Copy parameters */
	dup->params = params_get ( uri->params );

	DBGC ( uri, "URI duplicated" );
	uri_dump ( uri );
	DBGC ( uri, "\n" );

	return dup;
}

/**
 * Resolve base+relative path
 *
 * @v base_uri		Base path
 * @v relative_uri	Relative path
 * @ret resolved_uri	Resolved path, or NULL on failure
 *
 * Takes a base path (e.g. "/var/lib/tftpboot/vmlinuz" and a relative
 * path (e.g. "initrd.gz") and produces a new path
 * (e.g. "/var/lib/tftpboot/initrd.gz").  Note that any non-directory
 * portion of the base path will automatically be stripped; this
 * matches the semantics used when resolving the path component of
 * URIs.
 */
char * resolve_path ( const char *base_path,
		      const char *relative_path ) {
	char *base_copy;
	char *base_tmp;
	char *resolved;

	/* If relative path is absolute, just re-use it */
	if ( relative_path[0] == '/' )
		return strdup ( relative_path );

	/* Create modifiable copy of path for dirname() */
	base_copy = strdup ( base_path );
	if ( ! base_copy )
		return NULL;

	/* Strip filename portion of base path */
	base_tmp = dirname ( base_copy );

	/* Process "./" and "../" elements */
	while ( *relative_path == '.' ) {
		relative_path++;
		if ( *relative_path == 0 ) {
			/* Do nothing */
		} else if ( *relative_path == '/' ) {
			relative_path++;
		} else if ( *relative_path == '.' ) {
			relative_path++;
			if ( *relative_path == 0 ) {
				base_tmp = dirname ( base_tmp );
			} else if ( *relative_path == '/' ) {
				base_tmp = dirname ( base_tmp );
				relative_path++;
			} else {
				relative_path -= 2;
				break;
			}
		} else {
			relative_path--;
			break;
		}
	}

	/* Create and return new path */
	if ( asprintf ( &resolved, "%s%s%s", base_tmp,
			( ( base_tmp[ strlen ( base_tmp ) - 1 ] == '/' ) ?
			  "" : "/" ), relative_path ) < 0 )
		resolved = NULL;
	free ( base_copy );
	return resolved;
}

/**
 * Resolve base+relative URI
 *
 * @v base_uri		Base URI, or NULL
 * @v relative_uri	Relative URI
 * @ret resolved_uri	Resolved URI, or NULL on failure
 *
 * Takes a base URI (e.g. "http://ipxe.org/kernels/vmlinuz" and a
 * relative URI (e.g. "../initrds/initrd.gz") and produces a new URI
 * (e.g. "http://ipxe.org/initrds/initrd.gz").
 */
struct uri * resolve_uri ( const struct uri *base_uri,
			   struct uri *relative_uri ) {
	struct uri tmp_uri;
	char *tmp_path = NULL;
	struct uri *new_uri;

	/* If relative URI is absolute, just re-use it */
	if ( uri_is_absolute ( relative_uri ) || ( ! base_uri ) )
		return uri_get ( relative_uri );

	/* Mangle URI */
	memcpy ( &tmp_uri, base_uri, sizeof ( tmp_uri ) );
	if ( relative_uri->path ) {
		tmp_path = resolve_path ( ( base_uri->path ?
					    base_uri->path : "/" ),
					  relative_uri->path );
		tmp_uri.path = tmp_path;
		tmp_uri.query = relative_uri->query;
		tmp_uri.fragment = relative_uri->fragment;
		tmp_uri.params = relative_uri->params;
	} else if ( relative_uri->query ) {
		tmp_uri.query = relative_uri->query;
		tmp_uri.fragment = relative_uri->fragment;
		tmp_uri.params = relative_uri->params;
	} else if ( relative_uri->fragment ) {
		tmp_uri.fragment = relative_uri->fragment;
		tmp_uri.params = relative_uri->params;
	} else if ( relative_uri->params ) {
		tmp_uri.params = relative_uri->params;
	}

	/* Create demangled URI */
	new_uri = uri_dup ( &tmp_uri );
	free ( tmp_path );
	return new_uri;
}

/**
 * Construct TFTP URI from server address and filename
 *
 * @v sa_server		Server address
 * @v filename		Filename
 * @ret uri		URI, or NULL on failure
 */
static struct uri * tftp_uri ( struct sockaddr *sa_server,
			       const char *filename ) {
	struct sockaddr_tcpip *st_server =
		( ( struct sockaddr_tcpip * ) sa_server );
	char buf[ 6 /* "65535" + NUL */ ];
	char *path;
	struct uri tmp;
	struct uri *uri = NULL;

	/* Initialise TFTP URI */
	memset ( &tmp, 0, sizeof ( tmp ) );
	tmp.scheme = "tftp";

	/* Construct TFTP server address */
	tmp.host = sock_ntoa ( sa_server );
	if ( ! tmp.host )
		goto err_host;

	/* Construct TFTP server port, if applicable */
	if ( st_server->st_port ) {
		snprintf ( buf, sizeof ( buf ), "%d",
			   ntohs ( st_server->st_port ) );
		tmp.port = buf;
	}

	/* Construct TFTP path */
	if ( asprintf ( &path, "/%s", filename ) < 0 )
		goto err_path;
	tmp.path = path;

	/* Demangle URI */
	uri = uri_dup ( &tmp );
	if ( ! uri )
		goto err_uri;

 err_uri:
	free ( path );
 err_path:
 err_host:
	return uri;
}

/**
 * Construct URI from server address and filename
 *
 * @v sa_server		Server address
 * @v filename		Filename
 * @ret uri		URI, or NULL on failure
 *
 * PXE TFTP filenames specified via the DHCP next-server field often
 * contain characters such as ':' or '#' which would confuse the
 * generic URI parser.  We provide a mechanism for directly
 * constructing a TFTP URI from the next-server and filename.
 */
struct uri * pxe_uri ( struct sockaddr *sa_server, const char *filename ) {
	struct uri *uri;

	/* Fail if filename is empty */
	if ( ! ( filename && filename[0] ) )
		return NULL;

	/* If filename is a hierarchical absolute URI, then use that
	 * URI.  (We accept only hierarchical absolute URIs, since PXE
	 * filenames sometimes start with DOS drive letters such as
	 * "C:\", which get misinterpreted as opaque absolute URIs.)
	 */
	uri = parse_uri ( filename );
	if ( uri && uri_is_absolute ( uri ) && ( ! uri->opaque ) )
		return uri;
	uri_put ( uri );

	/* Otherwise, construct a TFTP URI directly */
	return tftp_uri ( sa_server, filename );
}

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
 * Hyper Text Transfer Protocol (HTTP)
 *
 */

#include <stddef.h>
#include <ipxe/open.h>
#include <ipxe/http.h>
#include <ipxe/features.h>

FEATURE ( FEATURE_PROTOCOL, "HTTP", DHCP_EB_FEATURE_HTTP, 1 );

/**
 * Initiate an HTTP connection
 *
 * @v xfer		Data transfer interface
 * @v uri		Uniform Resource Identifier
 * @ret rc		Return status code
 */
static int http_open ( struct interface *xfer, struct uri *uri ) {
	return http_open_filter ( xfer, uri, HTTP_PORT, NULL );
}

/** HTTP URI opener */
struct uri_opener http_uri_opener __uri_opener = {
	.scheme	= "http",
	.open	= http_open,
};

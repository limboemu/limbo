/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <config/general.h>

/** @file
 *
 * Routing management configuration options
 *
 */

/*
 * Drag in routing management for relevant protocols
 *
 */
#ifdef NET_PROTO_IPV4
REQUIRE_OBJECT ( route_ipv4 );
#endif
#ifdef NET_PROTO_IPV6
REQUIRE_OBJECT ( route_ipv6 );
#endif

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
 * Ethernet configuration options
 *
 */

/*
 * Drag in Ethernet-specific protocols
 */
#ifdef SANBOOT_PROTO_AOE
REQUIRE_OBJECT ( aoe );
#endif
#ifdef NET_PROTO_FCOE
REQUIRE_OBJECT ( fcoe );
#endif

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
 * Infiniband configuration options
 *
 */

/*
 * Drag in Infiniband-specific protocols
 */
#ifdef SANBOOT_PROTO_IB_SRP
REQUIRE_OBJECT ( ib_srp );
#endif

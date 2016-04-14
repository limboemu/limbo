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
 * Fibre Channel configuration options
 *
 */

/*
 * Drag in Fibre Channel-specific commands
 *
 */
#ifdef FCMGMT_CMD
REQUIRE_OBJECT ( fcmgmt_cmd );
#endif

/*
 * Drag in Fibre Channel-specific protocols
 */
#ifdef SANBOOT_PROTO_FCP
REQUIRE_OBJECT ( fcp );
#endif

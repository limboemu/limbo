/*
 * Copyright (C) 2016 Michael Brown <mbrown@fensystems.co.uk>.
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

#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <ipxe/command.h>
#include <ipxe/parseopt.h>
#include <usr/ntpmgmt.h>

/** @file
 *
 * NTP commands
 *
 */

/** "ntp" options */
struct ntp_options {};

/** "ntp" option list */
static struct option_descriptor ntp_opts[] = {};

/** "ntp" command descriptor */
static struct command_descriptor ntp_cmd =
	COMMAND_DESC ( struct ntp_options, ntp_opts, 1, 1, "<server>" );

/**
 * "ntp" command
 *
 * @v argc		Argument count
 * @v argv		Argument list
 * @ret rc		Return status code
 */
static int ntp_exec ( int argc, char **argv ) {
	struct ntp_options opts;
	const char *hostname;
	int rc;

	/* Parse options */
	if ( ( rc = parse_options ( argc, argv, &ntp_cmd, &opts ) ) != 0 )
		return rc;

	/* Parse hostname */
	hostname = argv[optind];

	/* Get time and date via NTP */
	if ( ( rc = ntp ( hostname ) ) != 0 ) {
		printf ( "Could not get time and date: %s\n", strerror ( rc ) );
		return rc;
	}

	return 0;
}

/** NTP command */
struct command ntp_command __command = {
	.name = "ntp",
	.exec = ntp_exec,
};

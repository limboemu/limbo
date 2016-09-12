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
#include <errno.h>
#include <getopt.h>
#include <ipxe/command.h>
#include <ipxe/parseopt.h>
#include <ipxe/infiniband.h>
#include <usr/ibmgmt.h>

/** @file
 *
 * Infiniband device management commands
 *
 */

/** "ibstat" options */
struct ibstat_options {};

/** "ibstat" option list */
static struct option_descriptor ibstat_opts[] = {};

/** "ibstat" command descriptor */
static struct command_descriptor ibstat_cmd =
	COMMAND_DESC ( struct ibstat_options, ibstat_opts, 0, 0, "" );

/**
 * The "ibstat" command
 *
 * @v argc		Argument count
 * @v argv		Argument list
 * @ret rc		Return status code
 */
static int ibstat_exec ( int argc, char **argv ) {
	struct ibstat_options opts;
	struct ib_device *ibdev;
	int rc;

	/* Parse options */
	if ( ( rc = parse_options ( argc, argv, &ibstat_cmd, &opts ) ) != 0 )
		return rc;

	/* Show all Infiniband devices */
	for_each_ibdev ( ibdev )
		ibstat ( ibdev );

	return 0;
}

/** Infiniband commands */
struct command ibmgmt_commands[] __command = {
	{
		.name = "ibstat",
		.exec = ibstat_exec,
	},
};

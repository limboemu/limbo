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

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <byteswap.h>
#include <ipxe/infiniband.h>
#include <usr/ibmgmt.h>

/** @file
 *
 * Infiniband device management
 *
 */

/**
 * Print status of Infiniband device
 *
 * @v ibdev		Infiniband device
 */
void ibstat ( struct ib_device *ibdev ) {
	struct ib_queue_pair *qp;

	printf ( "%s: " IB_GUID_FMT " using %s on %s port %d (%s)\n",
		 ibdev->name, IB_GUID_ARGS ( &ibdev->gid.s.guid ),
		 ibdev->dev->driver_name, ibdev->dev->name, ibdev->port,
		 ( ib_is_open ( ibdev ) ? "open" : "closed" ) );
	if ( ib_link_ok ( ibdev ) ) {
		printf ( "  [Link:up LID %d prefix " IB_GUID_FMT "]\n",
			 ibdev->lid, IB_GUID_ARGS ( &ibdev->gid.s.prefix ) );
	} else {
		printf ( "  [Link:down, port state %d]\n", ibdev->port_state );
	}
	list_for_each_entry ( qp, &ibdev->qps, list ) {
		printf ( "  QPN %#lx send %d/%d recv %d/%d %s\n",
			 qp->qpn, qp->send.fill, qp->send.num_wqes,
			 qp->recv.fill, qp->recv.num_wqes, qp->name );
	}
}

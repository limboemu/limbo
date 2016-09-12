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
#include <byteswap.h>
#include <ipxe/infiniband.h>
#include <ipxe/ib_mi.h>
#include <ipxe/ib_service.h>

/** @file
 *
 * Infiniband service records
 *
 */

/**
 * Create service record management transaction
 *
 * @v ibdev		Infiniband device
 * @v mi		Management interface
 * @v name		Service name
 * @v op		Management transaction operations
 * @ret madx		Management transaction, or NULL on error
 */
struct ib_mad_transaction *
ib_create_service_madx ( struct ib_device *ibdev,
			 struct ib_mad_interface *mi, const char *name,
			 struct ib_mad_transaction_operations *op ) {
	union ib_mad mad;
	struct ib_mad_sa *sa = &mad.sa;
	struct ib_service_record *svc = &sa->sa_data.service_record;

	/* Construct service record request */
	memset ( sa, 0, sizeof ( *sa ) );
	sa->mad_hdr.mgmt_class = IB_MGMT_CLASS_SUBN_ADM;
	sa->mad_hdr.class_version = IB_SA_CLASS_VERSION;
	sa->mad_hdr.method = IB_MGMT_METHOD_GET;
	sa->mad_hdr.attr_id = htons ( IB_SA_ATTR_SERVICE_REC );
	sa->sa_hdr.comp_mask[1] = htonl ( IB_SA_SERVICE_REC_NAME );
	snprintf ( svc->name, sizeof ( svc->name ), "%s", name );

	/* Create management transaction */
	return ib_create_madx ( ibdev, mi, &mad, NULL, op );
}

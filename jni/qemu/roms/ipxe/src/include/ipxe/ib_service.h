#ifndef _IPXE_IB_SERVICE_H
#define _IPXE_IB_SERVICE_H

/** @file
 *
 * Infiniband service records
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/infiniband.h>
#include <ipxe/ib_mi.h>

extern struct ib_mad_transaction *
ib_create_service_madx ( struct ib_device *ibdev,
			 struct ib_mad_interface *mi, const char *name,
			 struct ib_mad_transaction_operations *op );

#endif /* _IPXE_IB_SERVICE_H */

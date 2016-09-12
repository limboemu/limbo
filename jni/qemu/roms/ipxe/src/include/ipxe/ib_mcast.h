#ifndef _IPXE_IB_MCAST_H
#define _IPXE_IB_MCAST_H

/** @file
 *
 * Infiniband multicast groups
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/infiniband.h>

struct ib_mad_transaction;

/** An Infiniband multicast group membership */
struct ib_mc_membership {
	/** Queue pair */
	struct ib_queue_pair *qp;
	/** Address vector */
	struct ib_address_vector *av;
	/** Attached to multicast GID */
	int attached;
	/** Multicast group join transaction */
	struct ib_mad_transaction *madx;
	/** Handle join success/failure
	 *
	 * @v membership	Multicast group membership
	 * @v rc		Status code
	 */
	void ( * complete ) ( struct ib_mc_membership *membership, int rc );
};

extern int ib_mcast_join ( struct ib_device *ibdev, struct ib_queue_pair *qp,
			   struct ib_mc_membership *membership,
			   struct ib_address_vector *av, unsigned int mask,
			   void ( * joined ) ( struct ib_mc_membership *memb,
					       int rc ) );

extern void ib_mcast_leave ( struct ib_device *ibdev, struct ib_queue_pair *qp,
			     struct ib_mc_membership *membership );

#endif /* _IPXE_IB_MCAST_H */

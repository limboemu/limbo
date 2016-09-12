#ifndef _IPXE_EOIB_H
#define _IPXE_EOIB_H

/** @file
 *
 * Ethernet over Infiniband
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <byteswap.h>
#include <ipxe/netdevice.h>
#include <ipxe/infiniband.h>
#include <ipxe/ib_mcast.h>

/** An EoIB header */
struct eoib_header {
	/** Signature */
	uint16_t magic;
	/** Reserved */
	uint16_t reserved;
} __attribute__ (( packed ));

/** EoIB magic signature */
#define EOIB_MAGIC 0x8919

/** An EoIB device */
struct eoib_device {
	/** Name */
	const char *name;
	/** Network device */
	struct net_device *netdev;
	/** Underlying Infiniband device */
	struct ib_device *ibdev;
	/** List of EoIB devices */
	struct list_head list;
	/** Broadcast address */
	struct ib_address_vector broadcast;

	/** Completion queue */
	struct ib_completion_queue *cq;
	/** Queue pair */
	struct ib_queue_pair *qp;
	/** Broadcast group membership */
	struct ib_mc_membership membership;

	/** Peer cache */
	struct list_head peers;

	/** Send duplicate packet to gateway (or NULL)
	 *
	 * @v eoib		EoIB device
	 * @v original		Original I/O buffer
	 */
	void ( * duplicate ) ( struct eoib_device *eoib,
			       struct io_buffer *original );
	/** Gateway (if any) */
	struct ib_address_vector gateway;
	/** Multicast group additional component mask */
	unsigned int mask;
};

/**
 * Check if EoIB device uses a gateway
 *
 * @v eoib		EoIB device
 * @v has_gw		EoIB device uses a gateway
 */
static inline int eoib_has_gateway ( struct eoib_device *eoib ) {

	return ( eoib->duplicate != NULL );
}

/**
 * Force creation of multicast group
 *
 * @v eoib		EoIB device
 */
static inline void eoib_force_group_creation ( struct eoib_device *eoib ) {

	/* Some dubious EoIB implementations require each endpoint to
	 * force the creation of the multicast group.  Yes, this makes
	 * it impossible for the group parameters (e.g. SL) to ever be
	 * modified without breaking backwards compatiblity with every
	 * existing driver.
	 */
	eoib->mask = ( IB_SA_MCMEMBER_REC_PKEY | IB_SA_MCMEMBER_REC_QKEY |
		       IB_SA_MCMEMBER_REC_SL | IB_SA_MCMEMBER_REC_FLOW_LABEL |
		       IB_SA_MCMEMBER_REC_TRAFFIC_CLASS );
}

extern int eoib_create ( struct ib_device *ibdev, const uint8_t *hw_addr,
			 struct ib_address_vector *broadcast,
			 const char *name );
extern struct eoib_device * eoib_find ( struct ib_device *ibdev,
					const uint8_t *hw_addr );
extern void eoib_destroy ( struct eoib_device *eoib );
extern void eoib_set_gateway ( struct eoib_device *eoib,
			       struct ib_address_vector *av );

#endif /* _IPXE_EOIB_H */

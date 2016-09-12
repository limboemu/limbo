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
#include <errno.h>
#include <ipxe/errortab.h>
#include <ipxe/malloc.h>
#include <ipxe/iobuf.h>
#include <ipxe/if_ether.h>
#include <ipxe/netdevice.h>
#include <ipxe/ethernet.h>
#include <ipxe/infiniband.h>
#include <ipxe/ib_mcast.h>
#include <ipxe/ib_pathrec.h>
#include <ipxe/eoib.h>

/** @file
 *
 * Ethernet over Infiniband
 *
 */

/** Number of EoIB send work queue entries */
#define EOIB_NUM_SEND_WQES 8

/** Number of EoIB receive work queue entries */
#define EOIB_NUM_RECV_WQES 4

/** Number of EoIB completion queue entries */
#define EOIB_NUM_CQES 16

/** Link status for "broadcast join in progress" */
#define EINPROGRESS_JOINING __einfo_error ( EINFO_EINPROGRESS_JOINING )
#define EINFO_EINPROGRESS_JOINING __einfo_uniqify \
	( EINFO_EINPROGRESS, 0x01, "Joining" )

/** Human-readable message for the link status */
struct errortab eoib_errors[] __errortab = {
	__einfo_errortab ( EINFO_EINPROGRESS_JOINING ),
};

/** List of EoIB devices */
static LIST_HEAD ( eoib_devices );

static struct net_device_operations eoib_operations;

/****************************************************************************
 *
 * EoIB peer cache
 *
 ****************************************************************************
 */

/** An EoIB peer cache entry */
struct eoib_peer {
	/** List of EoIB peer cache entries */
	struct list_head list;
	/** Ethernet MAC */
	uint8_t mac[ETH_ALEN];
	/** Infiniband address vector */
	struct ib_address_vector av;
};

/**
 * Find EoIB peer cache entry
 *
 * @v eoib		EoIB device
 * @v mac		Ethernet MAC
 * @ret peer		EoIB peer, or NULL if not found
 */
static struct eoib_peer * eoib_find_peer ( struct eoib_device *eoib,
					   const uint8_t *mac ) {
	struct eoib_peer *peer;

	/* Find peer cache entry */
	list_for_each_entry ( peer, &eoib->peers, list ) {
		if ( memcmp ( mac, peer->mac, sizeof ( peer->mac ) ) == 0 ) {
			/* Move peer to start of list */
			list_del ( &peer->list );
			list_add ( &peer->list, &eoib->peers );
			return peer;
		}
	}

	return NULL;
}

/**
 * Create EoIB peer cache entry
 *
 * @v eoib		EoIB device
 * @v mac		Ethernet MAC
 * @ret peer		EoIB peer, or NULL on error
 */
static struct eoib_peer * eoib_create_peer ( struct eoib_device *eoib,
					     const uint8_t *mac ) {
	struct eoib_peer *peer;

	/* Allocate and initialise peer cache entry */
	peer = zalloc ( sizeof ( *peer ) );
	if ( peer ) {
		memcpy ( peer->mac, mac, sizeof ( peer->mac ) );
		list_add ( &peer->list, &eoib->peers );
	}
	return peer;
}

/**
 * Flush EoIB peer cache
 *
 * @v eoib		EoIB device
 */
static void eoib_flush_peers ( struct eoib_device *eoib ) {
	struct eoib_peer *peer;
	struct eoib_peer *tmp;

	list_for_each_entry_safe ( peer, tmp, &eoib->peers, list ) {
		list_del ( &peer->list );
		free ( peer );
	}
}

/**
 * Discard some entries from the peer cache
 *
 * @ret discarded	Number of cached items discarded
 */
static unsigned int eoib_discard ( void ) {
	struct net_device *netdev;
	struct eoib_device *eoib;
	struct eoib_peer *peer;
	unsigned int discarded = 0;

	/* Try to discard one cache entry for each EoIB device */
	for_each_netdev ( netdev ) {

		/* Skip non-EoIB devices */
		if ( netdev->op != &eoib_operations )
			continue;
		eoib = netdev->priv;

		/* Discard least recently used cache entry (if any) */
		list_for_each_entry_reverse ( peer, &eoib->peers, list ) {
			list_del ( &peer->list );
			free ( peer );
			discarded++;
			break;
		}
	}

	return discarded;
}

/** EoIB cache discarder */
struct cache_discarder eoib_discarder __cache_discarder ( CACHE_EXPENSIVE ) = {
	.discard = eoib_discard,
};

/**
 * Find destination address vector
 *
 * @v eoib		EoIB device
 * @v mac		Ethernet MAC
 * @ret av		Address vector, or NULL to send as broadcast
 */
static struct ib_address_vector * eoib_tx_av ( struct eoib_device *eoib,
					       const uint8_t *mac ) {
	struct ib_device *ibdev = eoib->ibdev;
	struct eoib_peer *peer;
	int rc;

	/* If this is a broadcast or multicast MAC address, then send
	 * this packet as a broadcast.
	 */
	if ( is_multicast_ether_addr ( mac ) ) {
		DBGCP ( eoib, "EoIB %s %s TX multicast\n",
			eoib->name, eth_ntoa ( mac ) );
		return NULL;
	}

	/* If we have no peer cache entry, then create one and send
	 * this packet as a broadcast.
	 */
	peer = eoib_find_peer ( eoib, mac );
	if ( ! peer ) {
		DBGC ( eoib, "EoIB %s %s TX unknown\n",
		       eoib->name, eth_ntoa ( mac ) );
		eoib_create_peer ( eoib, mac );
		return NULL;
	}

	/* If we have not yet recorded a received GID and QPN for this
	 * peer cache entry, then send this packet as a broadcast.
	 */
	if ( ! peer->av.gid_present ) {
		DBGCP ( eoib, "EoIB %s %s TX not yet recorded\n",
			eoib->name, eth_ntoa ( mac ) );
		return NULL;
	}

	/* If we have not yet resolved a path to this peer, then send
	 * this packet as a broadcast.
	 */
	if ( ( rc = ib_resolve_path ( ibdev, &peer->av ) ) != 0 ) {
		DBGCP ( eoib, "EoIB %s %s TX not yet resolved\n",
			eoib->name, eth_ntoa ( mac ) );
		return NULL;
	}

	/* Force use of GRH even for local destinations */
	peer->av.gid_present = 1;

	/* We have a fully resolved peer: send this packet as a
	 * unicast.
	 */
	DBGCP ( eoib, "EoIB %s %s TX " IB_GID_FMT " QPN %#lx\n", eoib->name,
		eth_ntoa ( mac ), IB_GID_ARGS ( &peer->av.gid ), peer->av.qpn );
	return &peer->av;
}

/**
 * Record source address vector
 *
 * @v eoib		EoIB device
 * @v mac		Ethernet MAC
 * @v lid		Infiniband LID
 */
static void eoib_rx_av ( struct eoib_device *eoib, const uint8_t *mac,
			 const struct ib_address_vector *av ) {
	const union ib_gid *gid = &av->gid;
	unsigned long qpn = av->qpn;
	struct eoib_peer *peer;

	/* Sanity checks */
	if ( ! av->gid_present ) {
		DBGC ( eoib, "EoIB %s %s RX with no GID\n",
		       eoib->name, eth_ntoa ( mac ) );
		return;
	}

	/* Find peer cache entry (if any) */
	peer = eoib_find_peer ( eoib, mac );
	if ( ! peer ) {
		DBGCP ( eoib, "EoIB %s %s RX " IB_GID_FMT " (ignored)\n",
			eoib->name, eth_ntoa ( mac ), IB_GID_ARGS ( gid ) );
		return;
	}

	/* Some dubious EoIB implementations utilise an Ethernet-to-
	 * EoIB gateway that will send packets from the wrong QPN.
	 */
	if ( eoib_has_gateway ( eoib ) &&
	     ( memcmp ( gid, &eoib->gateway.gid, sizeof ( *gid ) ) == 0 ) ) {
		qpn = eoib->gateway.qpn;
	}

	/* Do nothing if peer cache entry is complete and correct */
	if ( ( peer->av.lid == av->lid ) && ( peer->av.qpn == qpn ) ) {
		DBGCP ( eoib, "EoIB %s %s RX unchanged\n",
			eoib->name, eth_ntoa ( mac ) );
		return;
	}

	/* Update peer cache entry */
	peer->av.qpn = qpn;
	peer->av.qkey = eoib->broadcast.qkey;
	peer->av.gid_present = 1;
	memcpy ( &peer->av.gid, gid, sizeof ( peer->av.gid ) );
	DBGC ( eoib, "EoIB %s %s RX " IB_GID_FMT " QPN %#lx\n", eoib->name,
	       eth_ntoa ( mac ), IB_GID_ARGS ( &peer->av.gid ), peer->av.qpn );
}

/****************************************************************************
 *
 * EoIB network device
 *
 ****************************************************************************
 */

/**
 * Transmit packet via EoIB network device
 *
 * @v netdev		Network device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int eoib_transmit ( struct net_device *netdev,
			   struct io_buffer *iobuf ) {
	struct eoib_device *eoib = netdev->priv;
	struct eoib_header *eoib_hdr;
	struct ethhdr *ethhdr;
	struct ib_address_vector *av;
	size_t zlen;

	/* Sanity checks */
	assert ( iob_len ( iobuf ) >= sizeof ( *ethhdr ) );
	assert ( iob_headroom ( iobuf ) >= sizeof ( *eoib_hdr ) );

	/* Look up destination address vector */
	ethhdr = iobuf->data;
	av = eoib_tx_av ( eoib, ethhdr->h_dest );

	/* Prepend EoIB header */
	eoib_hdr = iob_push ( iobuf, sizeof ( *eoib_hdr ) );
	eoib_hdr->magic = htons ( EOIB_MAGIC );
	eoib_hdr->reserved = 0;

	/* Pad buffer to minimum Ethernet frame size */
	zlen = ( sizeof ( *eoib_hdr ) + ETH_ZLEN );
	assert ( zlen <= IOB_ZLEN );
	if ( iob_len ( iobuf ) < zlen )
		iob_pad ( iobuf, zlen );

	/* If we have no unicast address then send as a broadcast,
	 * with a duplicate sent to the gateway if applicable.
	 */
	if ( ! av ) {
		av = &eoib->broadcast;
		if ( eoib_has_gateway ( eoib ) )
			eoib->duplicate ( eoib, iobuf );
	}

	/* Post send work queue entry */
	return ib_post_send ( eoib->ibdev, eoib->qp, av, iobuf );
}

/**
 * Handle EoIB send completion
 *
 * @v ibdev		Infiniband device
 * @v qp		Queue pair
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void eoib_complete_send ( struct ib_device *ibdev __unused,
				 struct ib_queue_pair *qp,
				 struct io_buffer *iobuf, int rc ) {
	struct eoib_device *eoib = ib_qp_get_ownerdata ( qp );

	netdev_tx_complete_err ( eoib->netdev, iobuf, rc );
}

/**
 * Handle EoIB receive completion
 *
 * @v ibdev		Infiniband device
 * @v qp		Queue pair
 * @v dest		Destination address vector, or NULL
 * @v source		Source address vector, or NULL
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void eoib_complete_recv ( struct ib_device *ibdev __unused,
				 struct ib_queue_pair *qp,
				 struct ib_address_vector *dest __unused,
				 struct ib_address_vector *source,
				 struct io_buffer *iobuf, int rc ) {
	struct eoib_device *eoib = ib_qp_get_ownerdata ( qp );
	struct net_device *netdev = eoib->netdev;
	struct eoib_header *eoib_hdr;
	struct ethhdr *ethhdr;

	/* Record errors */
	if ( rc != 0 ) {
		netdev_rx_err ( netdev, iobuf, rc );
		return;
	}

	/* Sanity check */
	if ( iob_len ( iobuf ) < ( sizeof ( *eoib_hdr ) + sizeof ( *ethhdr ) )){
		DBGC ( eoib, "EoIB %s received packet too short to "
		       "contain EoIB and Ethernet headers\n", eoib->name );
		DBGC_HD ( eoib, iobuf->data, iob_len ( iobuf ) );
		netdev_rx_err ( netdev, iobuf, -EIO );
		return;
	}
	if ( ! source ) {
		DBGC ( eoib, "EoIB %s received packet without address "
		       "vector\n", eoib->name );
		netdev_rx_err ( netdev, iobuf, -ENOTTY );
		return;
	}

	/* Strip EoIB header */
	iob_pull ( iobuf, sizeof ( *eoib_hdr ) );

	/* Update neighbour cache entry, if any */
	ethhdr = iobuf->data;
	eoib_rx_av ( eoib, ethhdr->h_source, source );

	/* Hand off to network layer */
	netdev_rx ( netdev, iobuf );
}

/** EoIB completion operations */
static struct ib_completion_queue_operations eoib_cq_op = {
	.complete_send = eoib_complete_send,
	.complete_recv = eoib_complete_recv,
};

/** EoIB queue pair operations */
static struct ib_queue_pair_operations eoib_qp_op = {
	.alloc_iob = alloc_iob,
};

/**
 * Poll EoIB network device
 *
 * @v netdev		Network device
 */
static void eoib_poll ( struct net_device *netdev ) {
	struct eoib_device *eoib = netdev->priv;
	struct ib_device *ibdev = eoib->ibdev;

	/* Poll Infiniband device */
	ib_poll_eq ( ibdev );

	/* Poll the retry timers (required for EoIB multicast join) */
	retry_poll();
}

/**
 * Handle EoIB broadcast multicast group join completion
 *
 * @v membership	Multicast group membership
 * @v rc		Status code
 */
static void eoib_join_complete ( struct ib_mc_membership *membership, int rc ) {
	struct eoib_device *eoib =
		container_of ( membership, struct eoib_device, membership );

	/* Record join status as link status */
	netdev_link_err ( eoib->netdev, rc );
}

/**
 * Join EoIB broadcast multicast group
 *
 * @v eoib		EoIB device
 * @ret rc		Return status code
 */
static int eoib_join_broadcast_group ( struct eoib_device *eoib ) {
	int rc;

	/* Join multicast group */
	if ( ( rc = ib_mcast_join ( eoib->ibdev, eoib->qp,
				    &eoib->membership, &eoib->broadcast,
				    eoib->mask, eoib_join_complete ) ) != 0 ) {
		DBGC ( eoib, "EoIB %s could not join broadcast group: %s\n",
		       eoib->name, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Leave EoIB broadcast multicast group
 *
 * @v eoib		EoIB device
 */
static void eoib_leave_broadcast_group ( struct eoib_device *eoib ) {

	/* Leave multicast group */
	ib_mcast_leave ( eoib->ibdev, eoib->qp, &eoib->membership );
}

/**
 * Handle link status change
 *
 * @v eoib		EoIB device
 */
static void eoib_link_state_changed ( struct eoib_device *eoib ) {
	struct net_device *netdev = eoib->netdev;
	struct ib_device *ibdev = eoib->ibdev;
	int rc;

	/* Leave existing broadcast group */
	if ( eoib->qp )
		eoib_leave_broadcast_group ( eoib );

	/* Update broadcast GID based on potentially-new partition key */
	eoib->broadcast.gid.words[2] = htons ( ibdev->pkey | IB_PKEY_FULL );

	/* Set net device link state to reflect Infiniband link state */
	rc = ib_link_rc ( ibdev );
	netdev_link_err ( netdev, ( rc ? rc : -EINPROGRESS_JOINING ) );

	/* Join new broadcast group */
	if ( ib_is_open ( ibdev ) && ib_link_ok ( ibdev ) && eoib->qp &&
	     ( ( rc = eoib_join_broadcast_group ( eoib ) ) != 0 ) ) {
		DBGC ( eoib, "EoIB %s could not rejoin broadcast group: "
		       "%s\n", eoib->name, strerror ( rc ) );
		netdev_link_err ( netdev, rc );
		return;
	}
}

/**
 * Open EoIB network device
 *
 * @v netdev		Network device
 * @ret rc		Return status code
 */
static int eoib_open ( struct net_device *netdev ) {
	struct eoib_device *eoib = netdev->priv;
	struct ib_device *ibdev = eoib->ibdev;
	int rc;

	/* Open IB device */
	if ( ( rc = ib_open ( ibdev ) ) != 0 ) {
		DBGC ( eoib, "EoIB %s could not open %s: %s\n",
		       eoib->name, ibdev->name, strerror ( rc ) );
		goto err_ib_open;
	}

	/* Allocate completion queue */
	eoib->cq = ib_create_cq ( ibdev, EOIB_NUM_CQES, &eoib_cq_op );
	if ( ! eoib->cq ) {
		DBGC ( eoib, "EoIB %s could not allocate completion queue\n",
		       eoib->name );
		rc = -ENOMEM;
		goto err_create_cq;
	}

	/* Allocate queue pair */
	eoib->qp = ib_create_qp ( ibdev, IB_QPT_UD, EOIB_NUM_SEND_WQES,
				   eoib->cq, EOIB_NUM_RECV_WQES, eoib->cq,
				  &eoib_qp_op, netdev->name );
	if ( ! eoib->qp ) {
		DBGC ( eoib, "EoIB %s could not allocate queue pair\n",
		       eoib->name );
		rc = -ENOMEM;
		goto err_create_qp;
	}
	ib_qp_set_ownerdata ( eoib->qp, eoib );

	/* Fill receive rings */
	ib_refill_recv ( ibdev, eoib->qp );

	/* Fake a link status change to join the broadcast group */
	eoib_link_state_changed ( eoib );

	return 0;

	ib_destroy_qp ( ibdev, eoib->qp );
	eoib->qp = NULL;
 err_create_qp:
	ib_destroy_cq ( ibdev, eoib->cq );
	eoib->cq = NULL;
 err_create_cq:
	ib_close ( ibdev );
 err_ib_open:
	return rc;
}

/**
 * Close EoIB network device
 *
 * @v netdev		Network device
 */
static void eoib_close ( struct net_device *netdev ) {
	struct eoib_device *eoib = netdev->priv;
	struct ib_device *ibdev = eoib->ibdev;

	/* Flush peer cache */
	eoib_flush_peers ( eoib );

	/* Leave broadcast group */
	eoib_leave_broadcast_group ( eoib );

	/* Tear down the queues */
	ib_destroy_qp ( ibdev, eoib->qp );
	eoib->qp = NULL;
	ib_destroy_cq ( ibdev, eoib->cq );
	eoib->cq = NULL;

	/* Close IB device */
	ib_close ( ibdev );
}

/** EoIB network device operations */
static struct net_device_operations eoib_operations = {
	.open		= eoib_open,
	.close		= eoib_close,
	.transmit	= eoib_transmit,
	.poll		= eoib_poll,
};

/**
 * Create EoIB device
 *
 * @v ibdev		Infiniband device
 * @v hw_addr		Ethernet MAC
 * @v broadcast		Broadcast address vector
 * @v name		Interface name (or NULL to use default)
 * @ret rc		Return status code
 */
int eoib_create ( struct ib_device *ibdev, const uint8_t *hw_addr,
		  struct ib_address_vector *broadcast, const char *name ) {
	struct net_device *netdev;
	struct eoib_device *eoib;
	int rc;

	/* Allocate network device */
	netdev = alloc_etherdev ( sizeof ( *eoib ) );
	if ( ! netdev ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	netdev_init ( netdev, &eoib_operations );
	eoib = netdev->priv;
	netdev->dev = ibdev->dev;
	eoib->netdev = netdev;
	eoib->ibdev = ibdev_get ( ibdev );
	memcpy ( &eoib->broadcast, broadcast, sizeof ( eoib->broadcast ) );
	INIT_LIST_HEAD ( &eoib->peers );

	/* Set MAC address */
	memcpy ( netdev->hw_addr, hw_addr, ETH_ALEN );

	/* Set interface name, if applicable */
	if ( name )
		snprintf ( netdev->name, sizeof ( netdev->name ), "%s", name );
	eoib->name = netdev->name;

	/* Add to list of EoIB devices */
	list_add_tail ( &eoib->list, &eoib_devices );

	/* Register network device */
	if ( ( rc = register_netdev ( netdev ) ) != 0 )
		goto err_register;

	DBGC ( eoib, "EoIB %s created for %s MAC %s\n",
	       eoib->name, ibdev->name, eth_ntoa ( hw_addr ) );
	DBGC ( eoib, "EoIB %s broadcast GID " IB_GID_FMT "\n",
	       eoib->name, IB_GID_ARGS ( &broadcast->gid ) );
	return 0;

	unregister_netdev ( netdev );
 err_register:
	list_del ( &eoib->list );
	ibdev_put ( ibdev );
	netdev_nullify ( netdev );
	netdev_put ( netdev );
 err_alloc:
	return rc;
}

/**
 * Find EoIB device
 *
 * @v ibdev		Infiniband device
 * @v hw_addr		Original Ethernet MAC
 * @ret eoib		EoIB device
 */
struct eoib_device * eoib_find ( struct ib_device *ibdev,
				 const uint8_t *hw_addr ) {
	struct eoib_device *eoib;

	list_for_each_entry ( eoib, &eoib_devices, list ) {
		if ( ( eoib->ibdev == ibdev ) &&
		     ( memcmp ( eoib->netdev->hw_addr, hw_addr,
				ETH_ALEN ) == 0 ) )
			return eoib;
	}
	return NULL;
}

/**
 * Remove EoIB device
 *
 * @v eoib		EoIB device
 */
void eoib_destroy ( struct eoib_device *eoib ) {
	struct net_device *netdev = eoib->netdev;

	/* Unregister network device */
	unregister_netdev ( netdev );

	/* Remove from list of network devices */
	list_del ( &eoib->list );

	/* Drop reference to Infiniband device */
	ibdev_put ( eoib->ibdev );

	/* Free network device */
	DBGC ( eoib, "EoIB %s destroyed\n", eoib->name );
	netdev_nullify ( netdev );
	netdev_put ( netdev );
}

/**
 * Probe EoIB device
 *
 * @v ibdev		Infiniband device
 * @ret rc		Return status code
 */
static int eoib_probe ( struct ib_device *ibdev __unused ) {

	/* EoIB devices are not created automatically */
	return 0;
}

/**
 * Handle device or link status change
 *
 * @v ibdev		Infiniband device
 */
static void eoib_notify ( struct ib_device *ibdev ) {
	struct eoib_device *eoib;

	/* Handle link status change for any attached EoIB devices */
	list_for_each_entry ( eoib, &eoib_devices, list ) {
		if ( eoib->ibdev != ibdev )
			continue;
		eoib_link_state_changed ( eoib );
	}
}

/**
 * Remove EoIB device
 *
 * @v ibdev		Infiniband device
 */
static void eoib_remove ( struct ib_device *ibdev ) {
	struct eoib_device *eoib;
	struct eoib_device *tmp;

	/* Remove any attached EoIB devices */
	list_for_each_entry_safe ( eoib, tmp, &eoib_devices, list ) {
		if ( eoib->ibdev != ibdev )
			continue;
		eoib_destroy ( eoib );
	}
}

/** EoIB driver */
struct ib_driver eoib_driver __ib_driver = {
	.name = "EoIB",
	.probe = eoib_probe,
	.notify = eoib_notify,
	.remove = eoib_remove,
};

/****************************************************************************
 *
 * EoIB heartbeat packets
 *
 ****************************************************************************
 */

/**
 * Silently ignore incoming EoIB heartbeat packets
 *
 * @v iobuf		I/O buffer
 * @v netdev		Network device
 * @v ll_source		Link-layer source address
 * @v flags		Packet flags
 * @ret rc		Return status code
 */
static int eoib_heartbeat_rx ( struct io_buffer *iobuf,
			       struct net_device *netdev __unused,
			       const void *ll_dest __unused,
			       const void *ll_source __unused,
			       unsigned int flags __unused ) {
	free_iob ( iobuf );
	return 0;
}

/**
 * Transcribe EoIB heartbeat address
 *
 * @v net_addr		EoIB heartbeat address
 * @ret string		"<EoIB>"
 *
 * This operation is meaningless for the EoIB heartbeat protocol.
 */
static const char * eoib_heartbeat_ntoa ( const void *net_addr __unused ) {
	return "<EoIB>";
}

/** EoIB heartbeat network protocol */
struct net_protocol eoib_heartbeat_protocol __net_protocol = {
	.name = "EoIB",
	.net_proto = htons ( EOIB_MAGIC ),
	.rx = eoib_heartbeat_rx,
	.ntoa = eoib_heartbeat_ntoa,
};

/****************************************************************************
 *
 * EoIB gateway
 *
 ****************************************************************************
 *
 * Some dubious EoIB implementations require all broadcast traffic to
 * be sent twice: once to the actual broadcast group, and once as a
 * unicast to the EoIB-to-Ethernet gateway.  This somewhat curious
 * design arises since the EoIB-to-Ethernet gateway hardware lacks the
 * ability to attach a queue pair to a multicast GID (or LID), and so
 * cannot receive traffic sent to the broadcast group.
 *
 */

/**
 * Transmit duplicate packet to the EoIB gateway
 *
 * @v eoib		EoIB device
 * @v original		Original I/O buffer
 */
static void eoib_duplicate ( struct eoib_device *eoib,
			     struct io_buffer *original ) {
	struct net_device *netdev = eoib->netdev;
	struct ib_device *ibdev = eoib->ibdev;
	struct ib_address_vector *av = &eoib->gateway;
	size_t len = iob_len ( original );
	struct io_buffer *copy;
	int rc;

	/* Create copy of I/O buffer */
	copy = alloc_iob ( len );
	if ( ! copy ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	memcpy ( iob_put ( copy, len ), original->data, len );

	/* Append to network device's transmit queue */
	list_add_tail ( &copy->list, &original->list );

	/* Resolve path to gateway */
	if ( ( rc = ib_resolve_path ( ibdev, av ) ) != 0 ) {
		DBGC ( eoib, "EoIB %s no path to gateway: %s\n",
		       eoib->name, strerror ( rc ) );
		goto err_path;
	}

	/* Force use of GRH even for local destinations */
	av->gid_present = 1;

	/* Post send work queue entry */
	if ( ( rc = ib_post_send ( eoib->ibdev, eoib->qp, av, copy ) ) != 0 )
		goto err_post_send;

	return;

 err_post_send:
 err_path:
 err_alloc:
	netdev_tx_complete_err ( netdev, copy, rc );
}

/**
 * Set EoIB gateway
 *
 * @v eoib		EoIB device
 * @v av		Address vector, or NULL to clear gateway
 */
void eoib_set_gateway ( struct eoib_device *eoib,
			struct ib_address_vector *av ) {

	if ( av ) {
		DBGC ( eoib, "EoIB %s using gateway " IB_GID_FMT "\n",
		       eoib->name, IB_GID_ARGS ( &av->gid ) );
		memcpy ( &eoib->gateway, av, sizeof ( eoib->gateway ) );
		eoib->duplicate = eoib_duplicate;
	} else {
		DBGC ( eoib, "EoIB %s not using gateway\n", eoib->name );
		eoib->duplicate = NULL;
	}
}

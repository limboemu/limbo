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
#include <ipxe/version.h>
#include <ipxe/timer.h>
#include <ipxe/malloc.h>
#include <ipxe/iobuf.h>
#include <ipxe/retry.h>
#include <ipxe/process.h>
#include <ipxe/settings.h>
#include <ipxe/infiniband.h>
#include <ipxe/ib_service.h>
#include <ipxe/ib_cmrc.h>
#include <ipxe/if_ether.h>
#include <ipxe/ethernet.h>
#include <ipxe/eoib.h>
#include <ipxe/xsigo.h>

/** @file
 *
 * Xsigo virtual Ethernet devices
 *
 */

/** A Xsigo device */
struct xsigo_device {
	/** Reference count */
	struct refcnt refcnt;
	/** Underlying Infiniband device */
	struct ib_device *ibdev;
	/** List of Xsigo devices */
	struct list_head list;
	/** Device name */
	const char *name;

	/** Link opener timer */
	struct retry_timer opener;

	/** Discovery timer */
	struct retry_timer discovery;
	/** Discovery management transaction (if any) */
	struct ib_mad_transaction *madx;

	/** List of configuration managers */
	struct list_head managers;
};

/** A Xsigo configuration manager */
struct xsigo_manager {
	/** Reference count */
	struct refcnt refcnt;
	/** Xsigo device */
	struct xsigo_device *xdev;
	/** List of managers */
	struct list_head list;
	/** Device name */
	char name[16];
	/** Manager ID */
	struct xsigo_manager_id id;

	/** Data transfer interface */
	struct interface xfer;
	/** Connection timer */
	struct retry_timer reopen;
	/** Keepalive timer */
	struct retry_timer keepalive;
	/** Transmission process */
	struct process process;
	/** Pending transmissions */
	unsigned int pending;
	/** Transmit sequence number */
	uint32_t seq;

	/** List of virtual Ethernet devices */
	struct list_head nics;
};

/** Configuration manager pending transmissions */
enum xsigo_manager_pending {
	/** Send connection request */
	XCM_TX_CONNECT = 0x0001,
	/** Send registration message */
	XCM_TX_REGISTER = 0x0002,
};

/** A Xsigo virtual Ethernet device */
struct xsigo_nic {
	/** Configuration manager */
	struct xsigo_manager *xcm;
	/** List of virtual Ethernet devices */
	struct list_head list;
	/** Device name */
	char name[16];

	/** Resource identifier */
	union ib_guid resource;
	/** MAC address */
	uint8_t mac[ETH_ALEN];
	/** Network ID */
	unsigned long network;
};

/** Configuration manager service ID */
static union ib_guid xcm_service_id = {
	.bytes = XCM_SERVICE_ID,
};

/** List of all Xsigo devices */
static LIST_HEAD ( xsigo_devices );

/**
 * Free Xsigo device
 *
 * @v refcnt		Reference count
 */
static void xsigo_free ( struct refcnt *refcnt ) {
	struct xsigo_device *xdev =
		container_of ( refcnt, struct xsigo_device, refcnt );

	/* Sanity checks */
	assert ( ! timer_running ( &xdev->opener ) );
	assert ( ! timer_running ( &xdev->discovery ) );
	assert ( xdev->madx == NULL );
	assert ( list_empty ( &xdev->managers ) );

	/* Drop reference to Infiniband device */
	ibdev_put ( xdev->ibdev );

	/* Free device */
	free ( xdev );
}

/**
 * Free configuration manager
 *
 * @v refcnt		Reference count
 */
static void xcm_free ( struct refcnt *refcnt ) {
	struct xsigo_manager *xcm =
		container_of ( refcnt, struct xsigo_manager, refcnt );

	/* Sanity checks */
	assert ( ! timer_running ( &xcm->reopen ) );
	assert ( ! timer_running ( &xcm->keepalive ) );
	assert ( ! process_running ( &xcm->process ) );
	assert ( list_empty ( &xcm->nics ) );

	/* Drop reference to Xsigo device */
	ref_put ( &xcm->xdev->refcnt );

	/* Free manager */
	free ( xcm );
}

/****************************************************************************
 *
 * Virtual Ethernet (XVE) devices
 *
 ****************************************************************************
 */

/**
 * Create virtual Ethernet device
 *
 * @v xcm		Configuration manager
 * @v resource		Resource identifier
 * @v mac		Ethernet MAC
 * @v network		Network identifier
 * @v name		Device name
 * @ret rc		Return status code
 */
static int xve_create ( struct xsigo_manager *xcm, union ib_guid *resource,
			const uint8_t *mac, unsigned long network,
			unsigned long qkey, const char *name ) {
	struct xsigo_device *xdev = xcm->xdev;
	struct ib_device *ibdev = xdev->ibdev;
	struct xsigo_nic *xve;
	struct ib_address_vector broadcast;
	int rc;

	/* Allocate and initialise structure */
	xve = zalloc ( sizeof ( *xve ) );
	if ( ! xve ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	xve->xcm = xcm;
	snprintf ( xve->name, sizeof ( xve->name ), "%s", name );
	memcpy ( &xve->resource, resource, sizeof ( xve->resource ) );
	memcpy ( xve->mac, mac, ETH_ALEN );
	xve->network = network;
	DBGC ( xve, "XVE %s created for %s " IB_GUID_FMT "\n",
	       xve->name, xcm->name, IB_GUID_ARGS ( resource ) );
	DBGC ( xve, "XVE %s is MAC %s on network %ld\n",
	       xve->name, eth_ntoa ( mac ), network );

	/* Construct broadcast address vector */
	memset ( &broadcast, 0, sizeof ( broadcast ) );
	broadcast.qpn = IB_QPN_BROADCAST;
	broadcast.qkey = qkey;
	broadcast.gid_present = 1;
	broadcast.gid.dwords[0] = htonl ( XVE_PREFIX );
	broadcast.gid.words[2] = htons ( ibdev->pkey );
	broadcast.gid.dwords[3] = htonl ( network );

	/* Create EoIB device */
	if ( ( rc = eoib_create ( ibdev, xve->mac, &broadcast,
				  xve->name ) ) != 0 ) {
		DBGC ( xve, "XVE %s could not create EoIB device: %s\n",
		       xve->name, strerror ( rc ) );
		goto err_create;
	}

	/* Add to list of virtual Ethernet devices.  Do this only
	 * after creating the EoIB device, so that our net device
	 * notifier won't attempt to send an operational state update
	 * before we have acknowledged the installation.
	 */
	list_add ( &xve->list, &xcm->nics );

	return 0;

	list_del ( &xve->list );
 err_create:
	free ( xve );
 err_alloc:
	return rc;
}

/**
 * Find virtual Ethernet device
 *
 * @v xcm		Configuration manager
 * @v resource		Resource identifier
 * @ret xve		Virtual Ethernet device, or NULL
 */
static struct xsigo_nic * xve_find ( struct xsigo_manager *xcm,
				     union ib_guid *resource ) {
	struct xsigo_nic *xve;

	list_for_each_entry ( xve, &xcm->nics, list ) {
		if ( memcmp ( resource, &xve->resource,
			      sizeof ( *resource ) ) == 0 )
			return xve;
	}
	return NULL;
}

/**
 * Destroy virtual Ethernet device
 *
 * @v xve		Virtual Ethernet device
 */
static void xve_destroy ( struct xsigo_nic *xve ) {
	struct xsigo_manager *xcm = xve->xcm;
	struct xsigo_device *xdev = xcm->xdev;
	struct ib_device *ibdev = xdev->ibdev;
	struct eoib_device *eoib;

	/* Destroy corresponding EoIB device, if any */
	if ( ( eoib = eoib_find ( ibdev, xve->mac ) ) )
		eoib_destroy ( eoib );

	/* Remove from list of virtual Ethernet devices */
	list_del ( &xve->list );

	/* Free virtual Ethernet device */
	DBGC ( xve, "XVE %s destroyed\n", xve->name );
	free ( xve );
}

/**
 * Update virtual Ethernet device MTU
 *
 * @v xve		Virtual Ethernet device
 * @v eoib		EoIB device
 * @v mtu		New MTU (excluding Ethernet and EoIB headers)
 * @ret rc		Return status code
 */
static int xve_update_mtu ( struct xsigo_nic *xve, struct eoib_device *eoib,
			    size_t mtu ) {
	struct net_device *netdev = eoib->netdev;
	size_t max;

	/* Check that we can support this MTU */
	max = ( IB_MAX_PAYLOAD_SIZE - ( sizeof ( struct ethhdr ) +
					sizeof ( struct eoib_header ) ) );
	if ( mtu > max ) {
		DBGC ( xve, "XVE %s cannot support MTU %zd (max %zd)\n",
		       xve->name, mtu, max );
		return -ERANGE;
	}

	/* Update MTU.  No need to close/reopen the network device,
	 * since our Infiniband stack uses a fixed MTU anyway.  Note
	 * that the network device sees the Ethernet frame header but
	 * not the EoIB header.
	 */
	netdev->max_pkt_len = ( mtu + sizeof ( struct ethhdr ) );
	DBGC ( xve, "XVE %s has MTU %zd\n", xve->name, mtu );

	return 0;
}

/**
 * Open virtual Ethernet device
 *
 * @v xve		Virtual Ethernet device
 * @v eoib		EoIB device
 * @v open		New administrative state
 * @ret rc		Return status code
 */
static int xve_open ( struct xsigo_nic *xve, struct eoib_device *eoib ) {
	struct net_device *netdev = eoib->netdev;
	int rc;

	/* Do nothing if network device is already open */
	if ( netdev_is_open ( netdev ) )
		return 0;
	DBGC ( xve, "XVE %s opening network device\n", xve->name );

	/* Open network device */
	if ( ( rc = netdev_open ( netdev ) ) != 0 ) {
		DBGC ( xve, "XVE %s could not open: %s\n",
		       xve->name, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Close virtual Ethernet device
 *
 * @v xve		Virtual Ethernet device
 * @v eoib		EoIB device
 */
static void xve_close ( struct xsigo_nic *xve, struct eoib_device *eoib ) {
	struct net_device *netdev = eoib->netdev;

	/* Do nothing if network device is already closed */
	if ( ! netdev_is_open ( netdev ) )
		return;

	/* Close network device */
	netdev_close ( netdev );
	DBGC ( xve, "XVE %s closed network device\n", xve->name );
}

/**
 * Update virtual Ethernet device administrative state
 *
 * @v xve		Virtual Ethernet device
 * @v eoib		EoIB device
 * @v open		New administrative state
 * @ret rc		Return status code
 */
static int xve_update_state ( struct xsigo_nic *xve, struct eoib_device *eoib,
			      int open ) {

	/* Open or close device, as applicable */
	if ( open ) {
		return xve_open ( xve, eoib );
	} else {
		xve_close ( xve, eoib );
		return 0;
	}
}

/**
 * Update gateway (TCA)
 *
 * @v xve		Virtual Ethernet device
 * @v eoib		EoIB device
 * @v av		Address vector, or NULL if no gateway
 * @ret rc		Return status code
 */
static int xve_update_tca ( struct xsigo_nic *xve, struct eoib_device *eoib,
			    struct ib_address_vector *av ) {

	/* Update gateway address */
	eoib_set_gateway ( eoib, av );
	if ( av ) {
		DBGC ( xve, "XVE %s has TCA " IB_GID_FMT " data %#lx qkey "
		       "%#lx\n", xve->name, IB_GID_ARGS ( &av->gid ), av->qpn,
		       av->qkey );
	} else {
		DBGC ( xve, "XVE %s has no TCA\n", xve->name );
	}

	/* The Linux driver will modify the local device's link state
	 * to reflect the EoIB-to-Ethernet gateway's link state, but
	 * this seems philosophically incorrect since communication
	 * within the EoIB broadcast domain still works regardless of
	 * the state of the gateway.
	 */

	return 0;
}

/****************************************************************************
 *
 * Server management protocol (XSMP) session messages
 *
 ****************************************************************************
 */

/**
 * Get session message name (for debugging)
 *
 * @v type		Message type
 * @ret name		Message name
 */
static const char * xsmp_session_type ( unsigned int type ) {
	static char buf[16];

	switch ( type ) {
	case XSMP_SESSION_TYPE_HELLO:		return "HELLO";
	case XSMP_SESSION_TYPE_REGISTER:	return "REGISTER";
	case XSMP_SESSION_TYPE_CONFIRM:		return "CONFIRM";
	case XSMP_SESSION_TYPE_REJECT:		return "REJECT";
	case XSMP_SESSION_TYPE_SHUTDOWN:	return "SHUTDOWN";
	default:
		snprintf ( buf, sizeof ( buf ), "UNKNOWN<%d>", type );
		return buf;
	}
}

/**
 * Extract chassis name (for debugging)
 *
 * @v msg		Session message
 * @ret chassis		Chassis name
 */
static const char * xsmp_chassis_name ( struct xsmp_session_message *msg ) {
	static char chassis[ sizeof ( msg->chassis ) + 1 /* NUL */ ];

	memcpy ( chassis, msg->chassis, sizeof ( msg->chassis ) );
	return chassis;
}

/**
 * Extract session name (for debugging)
 *
 * @v msg		Session message
 * @ret session		Session name
 */
static const char * xsmp_session_name ( struct xsmp_session_message *msg ) {
	static char session[ sizeof ( msg->session ) + 1 /* NUL */ ];

	memcpy ( session, msg->session, sizeof ( msg->session ) );
	return session;
}

/**
 * Send session message
 *
 * @v xcm		Configuration manager
 * @v type		Message type
 * @ret rc		Return status code
 */
static int xsmp_tx_session ( struct xsigo_manager *xcm, unsigned int type ) {
	struct xsigo_device *xdev = xcm->xdev;
	struct ib_device *ibdev = xdev->ibdev;
	struct xsmp_session_message msg;
	int rc;

	/* Construct session message */
	memset ( &msg, 0, sizeof ( msg ) );
	msg.hdr.type = XSMP_TYPE_SESSION;
	msg.hdr.len = htons ( sizeof ( msg ) );
	msg.hdr.seq = htonl ( ++xcm->seq );
	memcpy ( &msg.hdr.src.guid, &ibdev->gid.s.guid,
		 sizeof ( msg.hdr.src.guid ) );
	memcpy ( &msg.hdr.dst.guid, &xcm->id.guid,
		 sizeof ( msg.hdr.dst.guid ) );
	msg.type = type;
	msg.len = htons ( sizeof ( msg ) - sizeof ( msg.hdr ) );
	msg.os_type = XSIGO_OS_TYPE_GENERIC;
	msg.resources = htons ( XSIGO_RESOURCE_XVE |
				XSIGO_RESOURCE_NO_HA );
	msg.boot = htonl ( XSMP_BOOT_PXE );
	DBGCP ( xcm, "XCM %s TX[%d] session %s\n", xcm->name,
		ntohl ( msg.hdr.seq ), xsmp_session_type ( msg.type ) );
	DBGCP_HDA ( xcm, 0, &msg, sizeof ( msg ) );

	/* Send session message */
	if ( ( rc = xfer_deliver_raw ( &xcm->xfer, &msg,
				       sizeof ( msg ) ) ) != 0 ) {
		DBGC ( xcm, "XCM %s TX session %s failed: %s\n", xcm->name,
		       xsmp_session_type ( msg.type ), strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Send registration message
 *
 * @v xcm		Configuration manager
 * @ret rc		Return status code
 */
static inline int xsmp_tx_session_register ( struct xsigo_manager *xcm ) {

	DBGC ( xcm, "XCM %s registering with " IB_GUID_FMT "\n",
	       xcm->name, IB_GUID_ARGS ( &xcm->id.guid ) );

	/* Send registration message */
	return xsmp_tx_session ( xcm, XSMP_SESSION_TYPE_REGISTER );
}

/**
 * Send keepalive message
 *
 * @v xcm		Configuration manager
 * @ret rc		Return status code
 */
static int xsmp_tx_session_hello ( struct xsigo_manager *xcm ) {

	/* Send keepalive message */
	return xsmp_tx_session ( xcm, XSMP_SESSION_TYPE_HELLO );
}

/**
 * Handle received keepalive message
 *
 * @v xcm		Configuration manager
 * @v msg		Keepalive message
 * @ret rc		Return status code
 */
static int xsmp_rx_session_hello ( struct xsigo_manager *xcm,
				   struct xsmp_session_message *msg __unused ) {

	/* Respond to keepalive message.  Note that the XCM doesn't
	 * seem to actually ever send these.
	 */
	return xsmp_tx_session_hello ( xcm );
}

/**
 * Handle received registration confirmation message
 *
 * @v xcm		Configuration manager
 * @v msg		Registration confirmation message
 * @ret rc		Return status code
 */
static int xsmp_rx_session_confirm ( struct xsigo_manager *xcm,
				     struct xsmp_session_message *msg ) {

	DBGC ( xcm, "XCM %s registered with \"%s\" as \"%s\"\n", xcm->name,
	       xsmp_chassis_name ( msg ), xsmp_session_name ( msg ) );

	return 0;
}

/**
 * Handle received registration rejection message
 *
 * @v xcm		Configuration manager
 * @v msg		Registration confirmation message
 * @ret rc		Return status code
 */
static int xsmp_rx_session_reject ( struct xsigo_manager *xcm,
				    struct xsmp_session_message *msg ) {

	DBGC ( xcm, "XCM %s rejected by \"%s\":\n",
	       xcm->name, xsmp_chassis_name ( msg ) );
	DBGC_HDA ( xcm, 0, msg, sizeof ( *msg ) );

	return -EPERM;
}

/**
 * Handle received shutdown message
 *
 * @v xcm		Configuration manager
 * @v msg		Registration confirmation message
 * @ret rc		Return status code
 */
static int xsmp_rx_session_shutdown ( struct xsigo_manager *xcm,
				      struct xsmp_session_message *msg ) {

	DBGC ( xcm, "XCM %s shut down by \"%s\":\n",
	       xcm->name, xsmp_chassis_name ( msg ) );
	DBGC_HDA ( xcm, 0, msg, sizeof ( *msg ) );

	return -ENOTCONN;
}

/**
 * Handle received session message
 *
 * @v xcm		Configuration manager
 * @v msg		Session message
 * @ret rc		Return status code
 */
static int xsmp_rx_session ( struct xsigo_manager *xcm,
			     struct xsmp_session_message *msg ) {

	DBGCP ( xcm, "XCM %s RX[%d] session %s\n", xcm->name,
		ntohl ( msg->hdr.seq ), xsmp_session_type ( msg->type ) );
	DBGCP_HDA ( xcm, 0, msg, sizeof ( *msg ) );

	/* Handle message according to type */
	switch ( msg->type ) {
	case XSMP_SESSION_TYPE_HELLO:
		return xsmp_rx_session_hello ( xcm, msg );
	case XSMP_SESSION_TYPE_CONFIRM:
		return xsmp_rx_session_confirm ( xcm, msg );
	case XSMP_SESSION_TYPE_REJECT:
		return xsmp_rx_session_reject ( xcm, msg );
	case XSMP_SESSION_TYPE_SHUTDOWN:
		return xsmp_rx_session_shutdown ( xcm, msg );
	default:
		DBGC ( xcm, "XCM %s RX[%d] session unexpected %s:\n", xcm->name,
		       ntohl ( msg->hdr.seq ), xsmp_session_type ( msg->type ));
		DBGC_HDA ( xcm, 0, msg, sizeof ( *msg ) );
		return -EPROTO;
	}
}

/****************************************************************************
 *
 * Server management protocol (XSMP) virtual Ethernet (XVE) messages
 *
 ****************************************************************************
 */

/**
 * Get virtual Ethernet message name (for debugging)
 *
 * @v type		Message type
 * @ret name		Message name
 */
static const char * xsmp_xve_type ( unsigned int type ) {
	static char buf[16];

	switch ( type ) {
	case XSMP_XVE_TYPE_INSTALL:		return "INSTALL";
	case XSMP_XVE_TYPE_DELETE:		return "DELETE";
	case XSMP_XVE_TYPE_UPDATE:		return "UPDATE";
	case XSMP_XVE_TYPE_OPER_UP:		return "OPER_UP";
	case XSMP_XVE_TYPE_OPER_DOWN:		return "OPER_DOWN";
	case XSMP_XVE_TYPE_OPER_REQ:		return "OPER_REQ";
	case XSMP_XVE_TYPE_READY:		return "READY";
	default:
		snprintf ( buf, sizeof ( buf ), "UNKNOWN<%d>", type );
		return buf;
	}
}

/**
 * Send virtual Ethernet message
 *
 * @v xcm		Configuration manager
 * @v msg		Partial message
 * @ret rc		Return status code
 */
static int xsmp_tx_xve ( struct xsigo_manager *xcm,
			 struct xsmp_xve_message *msg ) {
	struct xsigo_device *xdev = xcm->xdev;
	struct ib_device *ibdev = xdev->ibdev;
	int rc;

	/* Fill in common header fields */
	msg->hdr.type = XSMP_TYPE_XVE;
	msg->hdr.len = htons ( sizeof ( *msg ) );
	msg->hdr.seq = htonl ( ++xcm->seq );
	memcpy ( &msg->hdr.src.guid, &ibdev->gid.s.guid,
		 sizeof ( msg->hdr.src.guid ) );
	memcpy ( &msg->hdr.dst.guid, &xcm->id.guid,
		 sizeof ( msg->hdr.dst.guid ) );
	msg->len = htons ( sizeof ( *msg ) - sizeof ( msg->hdr ) );
	DBGCP ( xcm, "XCM %s TX[%d] xve %s code %#02x\n", xcm->name,
		ntohl ( msg->hdr.seq ), xsmp_xve_type ( msg->type ),
		msg->code );
	DBGCP_HDA ( xcm, 0, msg, sizeof ( *msg ) );

	/* Send virtual Ethernet message */
	if ( ( rc = xfer_deliver_raw ( &xcm->xfer, msg,
				       sizeof ( *msg ) ) ) != 0 ) {
		DBGC ( xcm, "XCM %s TX xve %s failed: %s\n", xcm->name,
		       xsmp_xve_type ( msg->type ), strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Send virtual Ethernet message including current device parameters
 *
 * @v xcm		Configuration manager
 * @v msg		Partial virtual Ethernet message
 * @v xve		Virtual Ethernet device
 * @v eoib		EoIB device
 * @ret rc		Return status code
 */
static int xsmp_tx_xve_params ( struct xsigo_manager *xcm,
				struct xsmp_xve_message *msg,
				struct xsigo_nic *xve,
				struct eoib_device *eoib ) {
	struct xsigo_device *xdev = xcm->xdev;
	struct ib_device *ibdev = xdev->ibdev;
	struct net_device *netdev = eoib->netdev;

	/* Set successful response code */
	msg->code = 0;

	/* Include network identifier, MTU, and current HCA parameters */
	msg->network = htonl ( xve->network );
	msg->mtu = htons ( netdev->max_pkt_len - sizeof ( struct ethhdr ) );
	msg->hca.prefix_le.qword = bswap_64 ( ibdev->gid.s.prefix.qword );
	msg->hca.pkey = htons ( ibdev->pkey );
	msg->hca.qkey = msg->tca.qkey;
	if ( eoib->qp ) {
		msg->hca.data = htonl ( eoib->qp->ext_qpn );
		msg->hca.qkey = htons ( eoib->qp->qkey );
	}

	/* The message type field is (ab)used to return the current
	 * operational status.
	 */
	if ( msg->type == XSMP_XVE_TYPE_OPER_REQ ) {
		msg->type = ( netdev_is_open ( netdev ) ?
			      XSMP_XVE_TYPE_OPER_UP : XSMP_XVE_TYPE_OPER_DOWN );
	}

	/* Send message */
	DBGC ( xve, "XVE %s network %d MTU %d ctrl %#x data %#x qkey %#04x "
	       "%s\n", xve->name, ntohl ( msg->network ), ntohs ( msg->mtu ),
	       ntohl ( msg->hca.ctrl ), ntohl ( msg->hca.data ),
	       ntohs ( msg->hca.qkey ), xsmp_xve_type ( msg->type ) );

	return xsmp_tx_xve ( xcm, msg );
}

/**
 * Send virtual Ethernet error response
 *
 * @v xcm		Configuration manager
 * @v msg		Partial virtual Ethernet message
 * @ret rc		Return status code
 */
static inline int xsmp_tx_xve_nack ( struct xsigo_manager *xcm,
				     struct xsmp_xve_message *msg ) {

	/* Set error response code.  (There aren't any meaningful
	 * detailed response codes defined by the wire protocol.)
	 */
	msg->code = XSMP_XVE_CODE_ERROR;

	/* Send message */
	return xsmp_tx_xve ( xcm, msg );
}

/**
 * Send virtual Ethernet notification
 *
 * @v xcm		Configuration manager
 * @v type		Message type
 * @v xve		Virtual Ethernet device
 * @v eoib		EoIB device
 * @ret rc		Return status code
 */
static int xsmp_tx_xve_notify ( struct xsigo_manager *xcm,
				unsigned int type,
				struct xsigo_nic *xve,
				struct eoib_device *eoib ) {
	struct xsmp_xve_message msg;

	/* Construct message */
	memset ( &msg, 0, sizeof ( msg ) );
	msg.type = type;
	memcpy ( &msg.resource, &xve->resource, sizeof ( msg.resource ) );

	/* Send message */
	return xsmp_tx_xve_params ( xcm, &msg, xve, eoib );
}

/**
 * Send virtual Ethernet current operational state
 *
 * @v xcm		Configuration manager
 * @v xve		Virtual Ethernet device
 * @v eoib		EoIB device
 * @ret rc		Return status code
 */
static inline int xsmp_tx_xve_oper ( struct xsigo_manager *xcm,
				     struct xsigo_nic *xve,
				     struct eoib_device *eoib ) {

	/* Send notification */
	return xsmp_tx_xve_notify ( xcm, XSMP_XVE_TYPE_OPER_REQ, xve, eoib );
}

/**
 * Handle received virtual Ethernet modification message
 *
 * @v xcm		Configuration manager
 * @v msg		Virtual Ethernet message
 * @v update		Update bitmask
 * @ret rc		Return status code
 */
static int xsmp_rx_xve_modify ( struct xsigo_manager *xcm,
				struct xsmp_xve_message *msg,
				unsigned int update ) {
	struct xsigo_device *xdev = xcm->xdev;
	struct ib_device *ibdev = xdev->ibdev;
	struct xsigo_nic *xve;
	struct eoib_device *eoib;
	struct ib_address_vector tca;
	size_t mtu;
	int rc;

	/* Avoid returning uninitialised HCA parameters in response */
	memset ( &msg->hca, 0, sizeof ( msg->hca ) );

	/* Find virtual Ethernet device */
	xve = xve_find ( xcm, &msg->resource );
	if ( ! xve ) {
		DBGC ( xcm, "XCM %s unrecognised resource " IB_GUID_FMT "\n",
		       xcm->name, IB_GUID_ARGS ( &msg->resource ) );
		rc = -ENOENT;
		goto err_no_xve;
	}

	/* Find corresponding EoIB device */
	eoib = eoib_find ( ibdev, xve->mac );
	if ( ! eoib ) {
		DBGC ( xve, "XVE %s has no EoIB device\n", xve->name );
		rc = -EPIPE;
		goto err_no_eoib;
	}

	/* The Xsigo management software fails to create the EoIB
	 * multicast group.  This is a fundamental design flaw.
	 */
	eoib_force_group_creation ( eoib );

	/* Extract modifiable parameters.  Note that the TCA GID is
	 * erroneously transmitted as little-endian.
	 */
	mtu = ntohs ( msg->mtu );
	tca.qpn = ntohl ( msg->tca.data );
	tca.qkey = ntohs ( msg->tca.qkey );
	tca.gid_present = 1;
	tca.gid.s.prefix.qword = bswap_64 ( msg->tca.prefix_le.qword );
	tca.gid.s.guid.qword = bswap_64 ( msg->guid_le.qword );

	/* Update MTU, if applicable */
	if ( ( update & XSMP_XVE_UPDATE_MTU ) &&
	     ( ( rc = xve_update_mtu ( xve, eoib, mtu ) ) != 0 ) )
		goto err_mtu;
	update &= ~XSMP_XVE_UPDATE_MTU;

	/* Update admin state, if applicable */
	if ( ( update & XSMP_XVE_UPDATE_STATE ) &&
	     ( ( rc = xve_update_state ( xve, eoib, msg->state ) ) != 0 ) )
		goto err_state;
	update &= ~XSMP_XVE_UPDATE_STATE;

	/* Remove gateway, if applicable */
	if ( ( update & XSMP_XVE_UPDATE_GW_DOWN ) &&
	     ( ( rc = xve_update_tca ( xve, eoib, NULL ) ) != 0 ) )
		goto err_gw_down;
	update &= ~XSMP_XVE_UPDATE_GW_DOWN;

	/* Update gateway, if applicable */
	if ( ( update & XSMP_XVE_UPDATE_GW_CHANGE ) &&
	     ( ( rc = xve_update_tca ( xve, eoib, &tca ) ) != 0 ) )
		goto err_gw_change;
	update &= ~XSMP_XVE_UPDATE_GW_CHANGE;

	/* Warn about unexpected updates */
	if ( update ) {
		DBGC ( xve, "XVE %s unrecognised update(s) %#08x\n",
		       xve->name, update );
	}

	xsmp_tx_xve_params ( xcm, msg, xve, eoib );
	return 0;

 err_gw_change:
 err_gw_down:
 err_state:
 err_mtu:
 err_no_eoib:
 err_no_xve:
	/* Send NACK */
	xsmp_tx_xve_nack ( xcm, msg );
	return rc;
}

/**
 * Handle received virtual Ethernet installation message
 *
 * @v xcm		Configuration manager
 * @v msg		Virtual Ethernet message
 * @ret rc		Return status code
 */
static int xsmp_rx_xve_install ( struct xsigo_manager *xcm,
				 struct xsmp_xve_message *msg ) {
	union {
		struct xsmp_xve_mac msg;
		uint8_t raw[ETH_ALEN];
	} mac;
	char name[ sizeof ( msg->name ) + 1 /* NUL */ ];
	unsigned long network;
	unsigned long qkey;
	unsigned int update;
	int rc;

	/* Demangle MAC address (which is erroneously transmitted as
	 * little-endian).
	 */
	mac.msg.high = bswap_16 ( msg->mac_le.high );
	mac.msg.low = bswap_32 ( msg->mac_le.low );

	/* Extract interface name (which may not be NUL-terminated) */
	memcpy ( name, msg->name, ( sizeof ( name ) - 1 /* NUL */ ) );
	name[ sizeof ( name ) - 1 /* NUL */ ] = '\0';

	/* Extract remaining message parameters */
	network = ntohl ( msg->network );
	qkey = ntohs ( msg->tca.qkey );
	DBGC2 ( xcm, "XCM %s " IB_GUID_FMT " install \"%s\" %s net %ld qkey "
		"%#lx\n", xcm->name, IB_GUID_ARGS ( &msg->resource ), name,
		eth_ntoa ( mac.raw ), network, qkey );

	/* Create virtual Ethernet device, if applicable */
	if ( ( xve_find ( xcm, &msg->resource ) == NULL ) &&
	     ( ( rc = xve_create ( xcm, &msg->resource, mac.raw, network,
				   qkey, name ) ) != 0 ) )
		goto err_create;

	/* Handle remaining parameters as for a modification message */
	update = XSMP_XVE_UPDATE_MTU;
	if ( msg->uplink == XSMP_XVE_UPLINK )
		update |= XSMP_XVE_UPDATE_GW_CHANGE;
	return xsmp_rx_xve_modify ( xcm, msg, update );

 err_create:
	/* Send NACK */
	xsmp_tx_xve_nack ( xcm, msg );
	return rc;
}

/**
 * Handle received virtual Ethernet deletion message
 *
 * @v xcm		Configuration manager
 * @v msg		Virtual Ethernet message
 * @ret rc		Return status code
 */
static int xsmp_rx_xve_delete ( struct xsigo_manager *xcm,
				struct xsmp_xve_message *msg ) {
	struct xsigo_nic *xve;

	DBGC2 ( xcm, "XCM %s " IB_GUID_FMT " delete\n",
		xcm->name, IB_GUID_ARGS ( &msg->resource ) );

	/* Destroy virtual Ethernet device (if any) */
	if ( ( xve = xve_find ( xcm, &msg->resource ) ) )
		xve_destroy ( xve );

	/* Send ACK */
	msg->code = 0;
	xsmp_tx_xve ( xcm, msg );

	return 0;
}

/**
 * Handle received virtual Ethernet update message
 *
 * @v xcm		Configuration manager
 * @v msg		Virtual Ethernet message
 * @ret rc		Return status code
 */
static int xsmp_rx_xve_update ( struct xsigo_manager *xcm,
				struct xsmp_xve_message *msg ) {
	unsigned int update = ntohl ( msg->update );

	DBGC2 ( xcm, "XCM %s " IB_GUID_FMT " update (%08x)\n",
		xcm->name, IB_GUID_ARGS ( &msg->resource ), update );

	/* Handle as a modification message */
	return xsmp_rx_xve_modify ( xcm, msg, update );
}

/**
 * Handle received virtual Ethernet operational request message
 *
 * @v xcm		Configuration manager
 * @v msg		Virtual Ethernet message
 * @ret rc		Return status code
 */
static int xsmp_rx_xve_oper_req ( struct xsigo_manager *xcm,
				  struct xsmp_xve_message *msg ) {

	DBGC2 ( xcm, "XCM %s " IB_GUID_FMT " operational request\n",
		xcm->name, IB_GUID_ARGS ( &msg->resource ) );

	/* Handle as a nullipotent modification message */
	return xsmp_rx_xve_modify ( xcm, msg, 0 );
}

/**
 * Handle received virtual Ethernet readiness message
 *
 * @v xcm		Configuration manager
 * @v msg		Virtual Ethernet message
 * @ret rc		Return status code
 */
static int xsmp_rx_xve_ready ( struct xsigo_manager *xcm,
			       struct xsmp_xve_message *msg ) {
	int rc;

	DBGC2 ( xcm, "XCM %s " IB_GUID_FMT " ready\n",
		xcm->name, IB_GUID_ARGS ( &msg->resource ) );

	/* Handle as a nullipotent modification message */
	if ( ( rc = xsmp_rx_xve_modify ( xcm, msg, 0 ) ) != 0 )
		return rc;

	/* Send an unsolicited operational state update, since there
	 * is no other way to convey the current operational state.
	 */
	msg->type = XSMP_XVE_TYPE_OPER_REQ;
	if ( ( rc = xsmp_rx_xve_modify ( xcm, msg, 0 ) ) != 0 )
		return rc;

	return 0;
}

/**
 * Handle received virtual Ethernet message
 *
 * @v xcm		Configuration manager
 * @v msg		Virtual Ethernet message
 * @ret rc		Return status code
 */
static int xsmp_rx_xve ( struct xsigo_manager *xcm,
			 struct xsmp_xve_message *msg ) {

	DBGCP ( xcm, "XCM %s RX[%d] xve %s\n", xcm->name,
		ntohl ( msg->hdr.seq ), xsmp_xve_type ( msg->type ) );
	DBGCP_HDA ( xcm, 0, msg, sizeof ( *msg ) );

	/* Handle message according to type */
	switch ( msg->type ) {
	case XSMP_XVE_TYPE_INSTALL:
		return xsmp_rx_xve_install ( xcm, msg );
	case XSMP_XVE_TYPE_DELETE:
		return xsmp_rx_xve_delete ( xcm, msg );
	case XSMP_XVE_TYPE_UPDATE:
		return xsmp_rx_xve_update ( xcm, msg );
	case XSMP_XVE_TYPE_OPER_REQ:
		return xsmp_rx_xve_oper_req ( xcm, msg );
	case XSMP_XVE_TYPE_READY:
		return xsmp_rx_xve_ready ( xcm, msg );
	default:
		DBGC ( xcm, "XCM %s RX[%d] xve unexpected %s:\n", xcm->name,
		       ntohl ( msg->hdr.seq ), xsmp_xve_type ( msg->type ) );
		DBGC_HDA ( xcm, 0, msg, sizeof ( *msg ) );
		return -EPROTO;
	}
}

/****************************************************************************
 *
 * Configuration managers (XCM)
 *
 ****************************************************************************
 */

/**
 * Close configuration manager connection
 *
 * @v xcm		Configuration manager
 * @v rc		Reason for close
 */
static void xcm_close ( struct xsigo_manager *xcm, int rc ) {

	DBGC ( xcm, "XCM %s closed: %s\n", xcm->name, strerror ( rc ) );

	/* Stop transmission process */
	process_del ( &xcm->process );

	/* Stop keepalive timer */
	stop_timer ( &xcm->keepalive );

	/* Restart data transfer interface */
	intf_restart ( &xcm->xfer, rc );

	/* Schedule reconnection attempt */
	start_timer ( &xcm->reopen );
}

/**
 * Send data to configuration manager
 *
 * @v xcm		Configuration manager
 */
static void xcm_step ( struct xsigo_manager *xcm ) {
	int rc;

	/* Do nothing unless we have something to send */
	if ( ! xcm->pending )
		return;

	/* Send (empty) connection request, if applicable */
	if ( xcm->pending & XCM_TX_CONNECT ) {
		if ( ( rc = xfer_deliver_raw ( &xcm->xfer, NULL, 0 ) ) != 0 ) {
			DBGC ( xcm, "XCM %s could not send connection request: "
			       "%s\n", xcm->name, strerror ( rc ) );
			goto err;
		}
		xcm->pending &= ~XCM_TX_CONNECT;
		return;
	}

	/* Wait until data transfer interface is connected */
	if ( ! xfer_window ( &xcm->xfer ) )
		return;

	/* Send registration message, if applicable */
	if ( xcm->pending & XCM_TX_REGISTER ) {
		if ( ( rc = xsmp_tx_session_register ( xcm ) ) != 0 )
			goto err;
		xcm->pending &= ~XCM_TX_REGISTER;
		return;
	}

	return;

 err:
	xcm_close ( xcm, rc );
}

/**
 * Receive data from configuration manager
 *
 * @v xcm		Configuration manager
 * @v iobuf		I/O buffer
 * @v meta		Data transfer metadata
 * @ret rc		Return status code
 */
static int xcm_deliver ( struct xsigo_manager *xcm, struct io_buffer *iobuf,
			 struct xfer_metadata *meta __unused ) {
	union xsmp_message *msg;
	size_t len = iob_len ( iobuf );
	int rc;

	/* Sanity check */
	if ( len < sizeof ( msg->hdr ) ) {
		DBGC ( xcm, "XCM %s underlength message:\n", xcm->name );
		DBGC_HDA ( xcm, 0, iobuf->data, iob_len ( iobuf ) );
		rc = -EPROTO;
		goto out;
	}
	msg = iobuf->data;

	/* Handle message according to type */
	if ( ! msg->hdr.type ) {

		/* Ignore unused communication manager private data blocks */
		rc = 0;

	} else if ( ( msg->hdr.type == XSMP_TYPE_SESSION ) &&
		    ( len >= sizeof ( msg->sess ) ) ) {

		/* Session message */
		rc = xsmp_rx_session ( xcm, &msg->sess );

	} else if ( ( msg->hdr.type == XSMP_TYPE_XVE ) &&
		    ( len >= sizeof ( msg->xve ) ) ) {

		/* Virtual Ethernet message */
		xsmp_rx_xve ( xcm, &msg->xve );

		/* Virtual Ethernet message errors are non-fatal */
		rc = 0;

	} else {

		/* Unknown message */
		DBGC ( xcm, "XCM %s unexpected message type %d:\n",
		       xcm->name, msg->hdr.type );
		DBGC_HDA ( xcm, 0, iobuf->data, iob_len ( iobuf ) );
		rc = -EPROTO;
	}

 out:
	free_iob ( iobuf );
	if ( rc != 0 )
		xcm_close ( xcm, rc );
	return rc;
}

/** Configuration manager data transfer interface operations */
static struct interface_operation xcm_xfer_op[] = {
	INTF_OP ( xfer_deliver, struct xsigo_manager *, xcm_deliver ),
	INTF_OP ( xfer_window_changed, struct xsigo_manager *, xcm_step ),
	INTF_OP ( intf_close, struct xsigo_manager *, xcm_close ),
};

/** Configuration manager data transfer interface descriptor */
static struct interface_descriptor xcm_xfer_desc =
	INTF_DESC ( struct xsigo_manager, xfer, xcm_xfer_op );

/** Configuration manager process descriptor */
static struct process_descriptor xcm_process_desc =
	PROC_DESC_ONCE ( struct xsigo_manager, process, xcm_step );

/**
 * Handle configuration manager connection timer expiry
 *
 * @v timer		Connection timer
 * @v fail		Failure indicator
 */
static void xcm_reopen ( struct retry_timer *timer, int fail __unused ) {
	struct xsigo_manager *xcm =
		container_of ( timer, struct xsigo_manager, reopen );
	struct xsigo_device *xdev = xcm->xdev;
	struct ib_device *ibdev = xdev->ibdev;
	union ib_gid gid;
	int rc;

	/* Stop transmission process */
	process_del ( &xcm->process );

	/* Stop keepalive timer */
	stop_timer ( &xcm->keepalive );

	/* Restart data transfer interface */
	intf_restart ( &xcm->xfer, -ECANCELED );

	/* Reset sequence number */
	xcm->seq = 0;

	/* Construct GID */
	memcpy ( &gid.s.prefix, &ibdev->gid.s.prefix, sizeof ( gid.s.prefix ) );
	memcpy ( &gid.s.guid, &xcm->id.guid, sizeof ( gid.s.guid ) );
	DBGC ( xcm, "XCM %s connecting to " IB_GID_FMT "\n",
	       xcm->name, IB_GID_ARGS ( &gid ) );

	/* Open CMRC connection */
	if ( ( rc = ib_cmrc_open ( &xcm->xfer, ibdev, &gid,
				   &xcm_service_id, xcm->name ) ) != 0 ) {
		DBGC ( xcm, "XCM %s could not open CMRC connection: %s\n",
		       xcm->name, strerror ( rc ) );
		start_timer ( &xcm->reopen );
		return;
	}

	/* Schedule transmissions */
	xcm->pending |= ( XCM_TX_CONNECT | XCM_TX_REGISTER );
	process_add ( &xcm->process );

	/* Start keepalive timer */
	start_timer_fixed ( &xcm->keepalive, XSIGO_KEEPALIVE_INTERVAL );

	return;
}

/**
 * Handle configuration manager keepalive timer expiry
 *
 * @v timer		Connection timer
 * @v fail		Failure indicator
 */
static void xcm_keepalive ( struct retry_timer *timer, int fail __unused ) {
	struct xsigo_manager *xcm =
		container_of ( timer, struct xsigo_manager, keepalive );
	int rc;

	/* Send keepalive message.  The server won't actually respond
	 * to these, but it gives the RC queue pair a chance to
	 * complain if it doesn't ever at least get an ACK.
	 */
	if ( ( rc = xsmp_tx_session_hello ( xcm ) ) != 0 ) {
		xcm_close ( xcm, rc );
		return;
	}

	/* Restart keepalive timer */
	start_timer_fixed ( &xcm->keepalive, XSIGO_KEEPALIVE_INTERVAL );
}

/**
 * Create configuration manager
 *
 * @v xsigo		Xsigo device
 * @v id		Configuration manager ID
 * @ret rc		Return status code
 */
static int xcm_create ( struct xsigo_device *xdev,
			struct xsigo_manager_id *id ) {
	struct xsigo_manager *xcm;

	/* Allocate and initialise structure */
	xcm = zalloc ( sizeof ( *xcm ) );
	if ( ! xcm )
		return -ENOMEM;
	ref_init ( &xcm->refcnt, xcm_free );
	xcm->xdev = xdev;
	ref_get ( &xcm->xdev->refcnt );
	snprintf ( xcm->name, sizeof ( xcm->name ), "%s:xcm-%d",
		   xdev->name, ntohs ( id->lid ) );
	memcpy ( &xcm->id, id, sizeof ( xcm->id ) );
	intf_init ( &xcm->xfer, &xcm_xfer_desc, &xcm->refcnt );
	timer_init ( &xcm->keepalive, xcm_keepalive, &xcm->refcnt );
	timer_init ( &xcm->reopen, xcm_reopen, &xcm->refcnt );
	process_init_stopped ( &xcm->process, &xcm_process_desc, &xcm->refcnt );
	INIT_LIST_HEAD ( &xcm->nics );

	/* Start timer to open connection */
	start_timer_nodelay ( &xcm->reopen );

	/* Add to list of managers and transfer reference to list */
	list_add ( &xcm->list, &xdev->managers );
	DBGC ( xcm, "XCM %s created for " IB_GUID_FMT " (LID %d)\n", xcm->name,
	       IB_GUID_ARGS ( &xcm->id.guid ), ntohs ( id->lid ) );
	return 0;
}

/**
 * Find configuration manager
 *
 * @v xsigo		Xsigo device
 * @v id		Configuration manager ID
 * @ret xcm		Configuration manager, or NULL
 */
static struct xsigo_manager * xcm_find ( struct xsigo_device *xdev,
					 struct xsigo_manager_id *id ) {
	struct xsigo_manager *xcm;
	union ib_guid *guid = &id->guid;

	/* Find configuration manager */
	list_for_each_entry ( xcm, &xdev->managers, list ) {
		if ( memcmp ( guid, &xcm->id.guid, sizeof ( *guid ) ) == 0 )
			return xcm;
	}
	return NULL;
}

/**
 * Destroy configuration manager
 *
 * @v xcm		Configuration manager
 */
static void xcm_destroy ( struct xsigo_manager *xcm ) {
	struct xsigo_nic *xve;

	/* Remove all EoIB NICs */
	while ( ( xve = list_first_entry ( &xcm->nics, struct xsigo_nic,
					   list ) ) ) {
		xve_destroy ( xve );
	}

	/* Stop transmission process */
	process_del ( &xcm->process );

	/* Stop timers */
	stop_timer ( &xcm->keepalive );
	stop_timer ( &xcm->reopen );

	/* Shut down data transfer interface */
	intf_shutdown ( &xcm->xfer, 0 );

	/* Remove from list of managers and drop list's reference */
	DBGC ( xcm, "XCM %s destroyed\n", xcm->name );
	list_del ( &xcm->list );
	ref_put ( &xcm->refcnt );
}

/**
 * Synchronise list of configuration managers
 *
 * @v xdev		Xsigo device
 * @v ids		List of manager IDs
 * @v count		Number of manager IDs
 * @ret rc		Return status code
 */
static int xcm_list ( struct xsigo_device *xdev, struct xsigo_manager_id *ids,
		      unsigned int count ) {
	struct xsigo_manager_id *id;
	struct xsigo_manager *xcm;
	struct xsigo_manager *tmp;
	struct list_head list;
	unsigned int i;
	int rc;

	/* Create list of managers to be retained */
	INIT_LIST_HEAD ( &list );
	for ( i = 0, id = ids ; i < count ; i++, id++ ) {
		if ( ( xcm = xcm_find ( xdev, id ) ) ) {
			list_del ( &xcm->list );
			list_add_tail ( &xcm->list, &list );
		}
	}

	/* Destroy any managers not in the list */
	list_for_each_entry_safe ( xcm, tmp, &xdev->managers, list )
		xcm_destroy ( xcm );
	list_splice ( &list, &xdev->managers );

	/* Create any new managers in the list, and force reconnection
	 * for any changed LIDs.
	 */
	for ( i = 0, id = ids ; i < count ; i++, id++ ) {
		if ( ( xcm = xcm_find ( xdev, id ) ) ) {
			if ( xcm->id.lid != id->lid )
				start_timer_nodelay ( &xcm->reopen );
			continue;
		}
		if ( ( rc = xcm_create ( xdev, id ) ) != 0 ) {
			DBGC ( xdev, "XDEV %s could not create manager: %s\n",
			       xdev->name, strerror ( rc ) );
			return rc;
		}
	}

	return 0;
}

/****************************************************************************
 *
 * Configuration manager discovery
 *
 ****************************************************************************
 */

/** A stage of discovery */
struct xsigo_discovery {
	/** Name */
	const char *name;
	/** Management transaction operations */
	struct ib_mad_transaction_operations op;
};

/**
 * Handle configuration manager lookup completion
 *
 * @v ibdev		Infiniband device
 * @v mi		Management interface
 * @v madx		Management transaction
 * @v rc		Status code
 * @v mad		Received MAD (or NULL on error)
 * @v av		Source address vector (or NULL on error)
 */
static void xsigo_xcm_complete ( struct ib_device *ibdev,
				 struct ib_mad_interface *mi __unused,
				 struct ib_mad_transaction *madx,
				 int rc, union ib_mad *mad,
				 struct ib_address_vector *av __unused ) {
	struct xsigo_device *xdev = ib_madx_get_ownerdata ( madx );
	union xsigo_mad *xsmad = container_of ( mad, union xsigo_mad, mad );
	struct xsigo_managers_reply *reply = &xsmad->reply;

	/* Check for failures */
	if ( ( rc == 0 ) && ( mad->hdr.status != htons ( IB_MGMT_STATUS_OK ) ) )
		rc = -ENODEV;
	if ( rc != 0 ) {
		DBGC ( xdev, "XDEV %s manager lookup failed: %s\n",
		       xdev->name, strerror ( rc ) );
		goto out;
	}

	/* Sanity checks */
	if ( reply->count > ( sizeof ( reply->manager ) /
			      sizeof ( reply->manager[0] ) ) ) {
		DBGC ( xdev, "XDEV %s has too many managers (%d)\n",
		       xdev->name, reply->count );
		goto out;
	}

	/* Synchronise list of managers */
	if ( ( rc = xcm_list ( xdev, reply->manager, reply->count ) ) != 0 )
		goto out;

	/* Report an empty list of managers */
	if ( reply->count == 0 )
		DBGC ( xdev, "XDEV %s has no managers\n", xdev->name );

	/* Delay next discovery attempt */
	start_timer_fixed ( &xdev->discovery, XSIGO_DISCOVERY_SUCCESS_DELAY );

out:
	/* Destroy the completed transaction */
	ib_destroy_madx ( ibdev, ibdev->gsi, madx );
	xdev->madx = NULL;
}

/** Configuration manager lookup discovery stage */
static struct xsigo_discovery xsigo_xcm_discovery = {
	.name = "manager",
	.op = {
		.complete = xsigo_xcm_complete,
	},
};

/**
 * Handle directory service lookup completion
 *
 * @v ibdev		Infiniband device
 * @v mi		Management interface
 * @v madx		Management transaction
 * @v rc		Status code
 * @v mad		Received MAD (or NULL on error)
 * @v av		Source address vector (or NULL on error)
 */
static void xsigo_xds_complete ( struct ib_device *ibdev,
				 struct ib_mad_interface *mi __unused,
				 struct ib_mad_transaction *madx,
				 int rc, union ib_mad *mad,
				 struct ib_address_vector *av __unused ) {
	struct xsigo_device *xdev = ib_madx_get_ownerdata ( madx );
	union xsigo_mad *xsmad = container_of ( mad, union xsigo_mad, mad );
	struct xsigo_managers_request *request = &xsmad->request;
	struct ib_service_record *svc;
	struct ib_address_vector dest;
	union ib_guid *guid;

	/* Allow for reuse of transaction pointer */
	xdev->madx = NULL;

	/* Check for failures */
	if ( ( rc == 0 ) && ( mad->hdr.status != htons ( IB_MGMT_STATUS_OK ) ) )
		rc = -ENODEV;
	if ( rc != 0 ) {
		DBGC ( xdev, "XDEV %s directory lookup failed: %s\n",
		       xdev->name, strerror ( rc ) );
		goto out;
	}

	/* Construct address vector */
	memset ( &dest, 0, sizeof ( dest ) );
	svc = &mad->sa.sa_data.service_record;
	dest.lid = ntohs ( svc->data16[0] );
	dest.sl = ibdev->sm_sl;
	dest.qpn = IB_QPN_GSI;
	dest.qkey = IB_QKEY_GSI;
	guid = ( ( union ib_guid * ) &svc->data64[0] );
	DBGC2 ( xdev, "XDEV %s found directory at LID %d GUID " IB_GUID_FMT
		"\n", xdev->name, dest.lid, IB_GUID_ARGS ( guid ) );

	/* Construct request (reusing MAD buffer) */
	memset ( request, 0, sizeof ( *request ) );
	request->mad_hdr.mgmt_class = XSIGO_MGMT_CLASS;
	request->mad_hdr.class_version = XSIGO_MGMT_CLASS_VERSION;
	request->mad_hdr.method = IB_MGMT_METHOD_GET;
	request->mad_hdr.attr_id = htons ( XSIGO_ATTR_XCM_REQUEST );
	memcpy ( &request->server.guid, &ibdev->gid.s.guid,
		 sizeof ( request->server.guid ) );
	snprintf ( request->os_version, sizeof ( request->os_version ),
		   "%s %s", product_short_name, product_version );
	snprintf ( request->arch, sizeof ( request->arch ), _S2 ( ARCH ) );
	request->os_type = XSIGO_OS_TYPE_GENERIC;
	request->resources = htons ( XSIGO_RESOURCES_PRESENT |
				     XSIGO_RESOURCE_XVE |
				     XSIGO_RESOURCE_NO_HA );

	/* The handling of this request on the server side is a
	 * textbook example of how not to design a wire protocol.  The
	 * server uses the _driver_ version number to determine which
	 * fields are present.
	 */
	request->driver_version = htonl ( 0x2a2a2a );

	/* The build version field is ignored unless it happens to
	 * contain the substring "xg-".
	 */
	snprintf ( request->build, sizeof ( request->build ),
		   "not-xg-%08lx", build_id );

	/* The server side user interface occasionally has no way to
	 * refer to an entry with an empty hostname.
	 */
	fetch_string_setting ( NULL, &hostname_setting, request->hostname,
			       sizeof ( request->hostname ) );
	if ( ! request->hostname[0] ) {
		snprintf ( request->hostname, sizeof ( request->hostname ),
			   "%s-" IB_GUID_FMT, product_short_name,
			   IB_GUID_ARGS ( &ibdev->gid.s.guid ) );
	}

	/* Start configuration manager lookup */
	xdev->madx = ib_create_madx ( ibdev, ibdev->gsi, mad, &dest,
				      &xsigo_xcm_discovery.op );
	if ( ! xdev->madx ) {
		DBGC ( xdev, "XDEV %s could not start manager lookup\n",
		       xdev->name );
		goto out;
	}
	ib_madx_set_ownerdata ( xdev->madx, xdev );

out:
	/* Destroy the completed transaction */
	ib_destroy_madx ( ibdev, ibdev->gsi, madx );
}

/** Directory service lookup discovery stage */
static struct xsigo_discovery xsigo_xds_discovery = {
	.name = "directory",
	.op = {
		.complete = xsigo_xds_complete,
	},
};

/**
 * Discover configuration managers
 *
 * @v timer		Retry timer
 * @v over		Failure indicator
 */
static void xsigo_discover ( struct retry_timer *timer, int over __unused ) {
	struct xsigo_device *xdev =
		container_of ( timer, struct xsigo_device, discovery );
	struct ib_device *ibdev = xdev->ibdev;
	struct xsigo_discovery *discovery;

	/* Restart timer */
	start_timer_fixed ( &xdev->discovery, XSIGO_DISCOVERY_FAILURE_DELAY );

	/* Cancel any pending discovery transaction */
	if ( xdev->madx ) {
		discovery = container_of ( xdev->madx->op,
					   struct xsigo_discovery, op );
		DBGC ( xdev, "XDEV %s timed out waiting for %s lookup\n",
		       xdev->name, discovery->name );
		ib_destroy_madx ( ibdev, ibdev->gsi, xdev->madx );
		xdev->madx = NULL;
	}

	/* Start directory service lookup */
	xdev->madx = ib_create_service_madx ( ibdev, ibdev->gsi,
					      XDS_SERVICE_NAME,
					      &xsigo_xds_discovery.op );
	if ( ! xdev->madx ) {
		DBGC ( xdev, "XDEV %s could not start directory lookup\n",
		       xdev->name );
		return;
	}
	ib_madx_set_ownerdata ( xdev->madx, xdev );
}

/****************************************************************************
 *
 * Infiniband device driver
 *
 ****************************************************************************
 */

/**
 * Open link and start discovery
 *
 * @v opener		Link opener
 * @v over		Failure indicator
 */
static void xsigo_ib_open ( struct retry_timer *opener, int over __unused ) {
	struct xsigo_device *xdev =
		container_of ( opener, struct xsigo_device, opener );
	struct ib_device *ibdev = xdev->ibdev;
	int rc;

	/* Open Infiniband device */
	if ( ( rc = ib_open ( ibdev ) ) != 0 ) {
		DBGC ( xdev, "XDEV %s could not open: %s\n",
		       xdev->name, strerror ( rc ) );
		/* Delay and try again */
		start_timer_fixed ( &xdev->opener, XSIGO_OPEN_RETRY_DELAY );
		return;
	}

	/* If link is already up, then start discovery */
	if ( ib_link_ok ( ibdev ) )
		start_timer_nodelay ( &xdev->discovery );
}

/**
 * Probe Xsigo device
 *
 * @v ibdev		Infiniband device
 * @ret rc		Return status code
 */
static int xsigo_ib_probe ( struct ib_device *ibdev ) {
	struct xsigo_device *xdev;

	/* Allocate and initialise structure */
	xdev = zalloc ( sizeof ( *xdev ) );
	if ( ! xdev )
		return -ENOMEM;
	ref_init ( &xdev->refcnt, xsigo_free );
	xdev->ibdev = ibdev_get ( ibdev );
	xdev->name = ibdev->name;
	timer_init ( &xdev->opener, xsigo_ib_open, &xdev->refcnt );
	timer_init ( &xdev->discovery, xsigo_discover, &xdev->refcnt );
	INIT_LIST_HEAD ( &xdev->managers );

	/* Start timer to open Infiniband device.  (We are currently
	 * within the Infiniband device probe callback list; opening
	 * the device here would have interesting side-effects.)
	 */
	start_timer_nodelay ( &xdev->opener );

	/* Add to list of devices and transfer reference to list */
	list_add_tail ( &xdev->list, &xsigo_devices );
	DBGC ( xdev, "XDEV %s created for " IB_GUID_FMT "\n",
	       xdev->name, IB_GUID_ARGS ( &ibdev->gid.s.guid ) );
	return 0;
}

/**
 * Handle device or link status change
 *
 * @v ibdev		Infiniband device
 */
static void xsigo_ib_notify ( struct ib_device *ibdev ) {
	struct xsigo_device *xdev;

	/* Stop/restart discovery on any attached devices */
	list_for_each_entry ( xdev, &xsigo_devices, list ) {

		/* Skip non-attached devices */
		if ( xdev->ibdev != ibdev )
			continue;

		/* Stop any ongoing discovery */
		if ( xdev->madx ) {
			ib_destroy_madx ( ibdev, ibdev->gsi, xdev->madx );
			xdev->madx = NULL;
		}
		stop_timer ( &xdev->discovery );

		/* If link is up, then start discovery */
		if ( ib_link_ok ( ibdev ) )
			start_timer_nodelay ( &xdev->discovery );
	}
}

/**
 * Remove Xsigo device
 *
 * @v ibdev		Infiniband device
 */
static void xsigo_ib_remove ( struct ib_device *ibdev ) {
	struct xsigo_device *xdev;
	struct xsigo_device *tmp;

	/* Remove any attached Xsigo devices */
	list_for_each_entry_safe ( xdev, tmp, &xsigo_devices, list ) {

		/* Skip non-attached devices */
		if ( xdev->ibdev != ibdev )
			continue;

		/* Stop any ongoing discovery */
		if ( xdev->madx ) {
			ib_destroy_madx ( ibdev, ibdev->gsi, xdev->madx );
			xdev->madx = NULL;
		}
		stop_timer ( &xdev->discovery );

		/* Destroy all configuration managers */
		xcm_list ( xdev, NULL, 0 );

		/* Close Infiniband device, if applicable */
		if ( ! timer_running ( &xdev->opener ) )
			ib_close ( xdev->ibdev );

		/* Stop link opener */
		stop_timer ( &xdev->opener );

		/* Remove from list of devices and drop list's reference */
		DBGC ( xdev, "XDEV %s destroyed\n", xdev->name );
		list_del ( &xdev->list );
		ref_put ( &xdev->refcnt );
	}
}

/** Xsigo Infiniband driver */
struct ib_driver xsigo_ib_driver __ib_driver = {
	.name = "Xsigo",
	.probe = xsigo_ib_probe,
	.notify = xsigo_ib_notify,
	.remove = xsigo_ib_remove,
};

/****************************************************************************
 *
 * Network device driver
 *
 ****************************************************************************
 */

/**
 * Handle device or link status change
 *
 * @v netdev		Network device
 */
static void xsigo_net_notify ( struct net_device *netdev ) {
	struct xsigo_device *xdev;
	struct ib_device *ibdev;
	struct xsigo_manager *xcm;
	struct xsigo_nic *xve;
	struct eoib_device *eoib;

	/* Send current operational state to XCM, if applicable */
	list_for_each_entry ( xdev, &xsigo_devices, list ) {
		ibdev = xdev->ibdev;
		list_for_each_entry ( xcm, &xdev->managers, list ) {
			list_for_each_entry ( xve, &xcm->nics, list ) {
				eoib = eoib_find ( ibdev, xve->mac );
				if ( ! eoib )
					continue;
				if ( eoib->netdev != netdev )
					continue;
				xsmp_tx_xve_oper ( xcm, xve, eoib );
			}
		}
	}
}

/** Xsigo network driver */
struct net_driver xsigo_net_driver __net_driver = {
	.name = "Xsigo",
	.notify = xsigo_net_notify,
};

/*
 * Copyright (C) 2015 Mellanox Technologies Ltd.
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
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <byteswap.h>
#include <ipxe/pci.h>
#include <ipxe/malloc.h>
#include <ipxe/umalloc.h>
#include <ipxe/if_ether.h>
#include <ipxe/ethernet.h>
#include <ipxe/vlan.h>
#include <ipxe/io.h>
#include "flexboot_nodnic.h"
#include "mlx_utils/mlx_lib/mlx_nvconfig/mlx_nvconfig.h"
#include "mlx_utils/mlx_lib/mlx_nvconfig/mlx_nvconfig_defaults.h"
#include "mlx_utils/include/public/mlx_pci_gw.h"
#include "mlx_utils/mlx_lib/mlx_vmac/mlx_vmac.h"
#include "mlx_utils/include/public/mlx_types.h"
#include "mlx_utils/include/public/mlx_utils.h"
#include "mlx_utils/include/public/mlx_bail.h"
#include "mlx_nodnic/include/mlx_cmd.h"
#include "mlx_utils/include/public/mlx_memory.h"
#include "mlx_utils/include/public/mlx_pci.h"
#include "mlx_nodnic/include/mlx_device.h"
#include "mlx_nodnic/include/mlx_port.h"

/***************************************************************************
 *
 * Completion queue operations
 *
 ***************************************************************************
 */
static int flexboot_nodnic_arm_cq ( struct flexboot_nodnic_port *port ) {
#ifndef DEVICE_CX3
	mlx_uint32 val = ( port->eth_cq->next_idx & 0xffff );
	if ( nodnic_port_set ( & port->port_priv, nodnic_port_option_arm_cq, val ) ) {
		MLX_DEBUG_ERROR( port->port_priv.device, "Failed to arm the CQ\n" );
		return MLX_FAILED;
	}
#else
	mlx_utils *utils = port->port_priv.device->utils;
	nodnic_port_data_flow_gw *ptr = port->port_priv.data_flow_gw;
	mlx_uint32 data = 0;
	mlx_uint32 val = 0;

	if ( port->port_priv.device->device_cap.crspace_doorbells == 0 ) {
		val = ( port->eth_cq->next_idx & 0xffff );
		if ( nodnic_port_set ( & port->port_priv, nodnic_port_option_arm_cq, val ) ) {
			MLX_DEBUG_ERROR( port->port_priv.device, "Failed to arm the CQ\n" );
			return MLX_FAILED;
		}
	} else {
		/* Arming the CQ with CQ CI should be with this format -
		 * 16 bit - CQ CI - same endianness as the FW (don't swap bytes)
		 * 15 bit - reserved
		 *  1 bit - arm CQ - must correct the endianness with the reserved above */
		data = ( ( ( port->eth_cq->next_idx & 0xffff ) << 16 ) | 0x0080 );
		/* Write the new index and update FW that new data was submitted */
		mlx_pci_mem_write ( utils, MlxPciWidthUint32, 0,
				( mlx_uint64 ) & ( ptr->armcq_cq_ci_dword ), 1, &data );
	}
#endif
	return 0;
}

/**
 * Create completion queue
 *
 * @v ibdev		Infiniband device
 * @v cq		Completion queue
 * @ret rc		Return status code
 */
static int flexboot_nodnic_create_cq ( struct ib_device *ibdev ,
			      struct ib_completion_queue *cq ) {
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	struct flexboot_nodnic_port *port = &flexboot_nodnic->port[ibdev->port - 1];
	struct flexboot_nodnic_completion_queue *flexboot_nodnic_cq;
	mlx_status status = MLX_SUCCESS;

	flexboot_nodnic_cq = (struct flexboot_nodnic_completion_queue *)
			zalloc(sizeof(*flexboot_nodnic_cq));
	if ( flexboot_nodnic_cq == NULL ) {
		status = MLX_OUT_OF_RESOURCES;
		goto qp_alloc_err;
	}

	status = nodnic_port_create_cq(&port->port_priv,
			cq->num_cqes *
			flexboot_nodnic->callbacks->get_cqe_size(),
			&flexboot_nodnic_cq->nodnic_completion_queue
			);
	MLX_FATAL_CHECK_STATUS(status, create_err,
				"nodnic_port_create_cq failed");
	flexboot_nodnic->callbacks->cqe_set_owner(
			flexboot_nodnic_cq->nodnic_completion_queue->cq_virt,
			cq->num_cqes);


	ib_cq_set_drvdata ( cq, flexboot_nodnic_cq );
	return status;
create_err:
	free(flexboot_nodnic_cq);
qp_alloc_err:
	return status;
}

/**
 * Destroy completion queue
 *
 * @v ibdev		Infiniband device
 * @v cq		Completion queue
 */
static void flexboot_nodnic_destroy_cq ( struct ib_device *ibdev ,
				struct ib_completion_queue *cq ) {
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	struct flexboot_nodnic_port *port = &flexboot_nodnic->port[ibdev->port - 1];
	struct flexboot_nodnic_completion_queue *flexboot_nodnic_cq = ib_cq_get_drvdata ( cq );

	nodnic_port_destroy_cq(&port->port_priv,
			flexboot_nodnic_cq->nodnic_completion_queue);

	free(flexboot_nodnic_cq);
}

static
struct ib_work_queue * flexboot_nodnic_find_wq ( struct ib_device *ibdev ,
							struct ib_completion_queue *cq,
							unsigned long qpn, int is_send ) {
	struct ib_work_queue *wq;
	struct flexboot_nodnic_queue_pair *flexboot_nodnic_qp;
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	struct flexboot_nodnic_port *port = &flexboot_nodnic->port[ibdev->port - 1];
	struct nodnic_ring  *ring;
	mlx_uint32 out_qpn;
	list_for_each_entry ( wq, &cq->work_queues, list ) {
		flexboot_nodnic_qp = ib_qp_get_drvdata ( wq->qp );
		if( wq->is_send == is_send && wq->is_send == TRUE ) {
			ring = &flexboot_nodnic_qp->nodnic_queue_pair->send.nodnic_ring;
		} else if( wq->is_send == is_send && wq->is_send == FALSE ) {
			ring = &flexboot_nodnic_qp->nodnic_queue_pair->receive.nodnic_ring;
		} else {
			continue;
		}
		nodnic_port_get_qpn(&port->port_priv, ring, &out_qpn);
		if ( out_qpn == qpn )
			return wq;
	}
	return NULL;
}

/**
 * Handle completion
 *
 * @v ibdev		Infiniband device
 * @v cq		Completion queue
 * @v cqe		Hardware completion queue entry
 * @ret rc		Return status code
 */
static int flexboot_nodnic_complete ( struct ib_device *ibdev,
			     struct ib_completion_queue *cq,
				 struct cqe_data *cqe_data ) {
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	struct ib_work_queue *wq;
	struct ib_queue_pair *qp;
	struct io_buffer *iobuf;
	struct ib_address_vector recv_dest;
	struct ib_address_vector recv_source;
	unsigned long qpn;
	unsigned long wqe_idx;
	unsigned long wqe_idx_mask;
	size_t len;
	int rc = 0;

	/* Parse completion */
	qpn = cqe_data->qpn;

	if ( cqe_data->is_error == TRUE ) {
		DBGC ( flexboot_nodnic, "flexboot_nodnic %p CQN %#lx syndrome %x vendor %x\n",
				flexboot_nodnic, cq->cqn, cqe_data->syndrome,
				cqe_data->vendor_err_syndrome );
		rc = -EIO;
		/* Don't return immediately; propagate error to completer */
	}

	/* Identify work queue */
	wq = flexboot_nodnic_find_wq( ibdev, cq, qpn, cqe_data->is_send );
	if ( wq == NULL ) {
		DBGC ( flexboot_nodnic,
				"flexboot_nodnic %p CQN %#lx unknown %s QPN %#lx\n",
				flexboot_nodnic, cq->cqn,
				( cqe_data->is_send ? "send" : "recv" ), qpn );
		return -EIO;
	}
	qp = wq->qp;

	/* Identify work queue entry */
	wqe_idx = cqe_data->wqe_counter;
	wqe_idx_mask = ( wq->num_wqes - 1 );
	DBGCP ( flexboot_nodnic,
			"NODNIC %p CQN %#lx QPN %#lx %s WQE %#lx completed:\n",
			flexboot_nodnic, cq->cqn, qp->qpn,
			( cqe_data->is_send ? "send" : "recv" ),
		wqe_idx );

	/* Identify I/O buffer */
	iobuf = wq->iobufs[wqe_idx & wqe_idx_mask];
	if ( iobuf == NULL ) {
		DBGC ( flexboot_nodnic,
				"NODNIC %p CQN %#lx QPN %#lx empty %s WQE %#lx\n",
				flexboot_nodnic, cq->cqn, qp->qpn,
		       ( cqe_data->is_send ? "send" : "recv" ), wqe_idx );
		return -EIO;
	}
	wq->iobufs[wqe_idx & wqe_idx_mask] = NULL;

	if ( cqe_data->is_send == TRUE ) {
		/* Hand off to completion handler */
		ib_complete_send ( ibdev, qp, iobuf, rc );
	} else if ( rc != 0 ) {
		/* Propagate error to receive completion handler */
		ib_complete_recv ( ibdev, qp, NULL, NULL, iobuf, rc );
	} else {
		/* Set received length */
		len = cqe_data->byte_cnt;
		assert ( len <= iob_tailroom ( iobuf ) );
		iob_put ( iobuf, len );
		memset ( &recv_dest, 0, sizeof ( recv_dest ) );
		recv_dest.qpn = qpn;
		memset ( &recv_source, 0, sizeof ( recv_source ) );
		switch ( qp->type ) {
		case IB_QPT_SMI:
		case IB_QPT_GSI:
		case IB_QPT_UD:
		case IB_QPT_RC:
			break;
		case IB_QPT_ETH:
			break;
		default:
			assert ( 0 );
			return -EINVAL;
		}
		/* Hand off to completion handler */
		ib_complete_recv ( ibdev, qp, &recv_dest,
				&recv_source, iobuf, rc );
	}

	return rc;
}
/**
 * Poll completion queue
 *
 * @v ibdev		Infiniband device
 * @v cq		Completion queues
 */
static void flexboot_nodnic_poll_cq ( struct ib_device *ibdev,
			     struct ib_completion_queue *cq) {
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	struct flexboot_nodnic_completion_queue *flexboot_nodnic_cq = ib_cq_get_drvdata ( cq );
	void *cqe;
	mlx_size cqe_size;
	struct cqe_data cqe_data;
	unsigned int cqe_idx_mask;
	int rc;

	cqe_size = flexboot_nodnic->callbacks->get_cqe_size();
	while ( TRUE ) {
		/* Look for completion entry */
		cqe_idx_mask = ( cq->num_cqes - 1 );
		cqe = ((uint8_t *)flexboot_nodnic_cq->nodnic_completion_queue->cq_virt) +
				cqe_size * (cq->next_idx & cqe_idx_mask);

		/* TODO: check fill_completion */
		flexboot_nodnic->callbacks->fill_completion(cqe, &cqe_data);
		if ( cqe_data.owner ^
				( ( cq->next_idx & cq->num_cqes ) ? 1 : 0 ) ) {
			/* Entry still owned by hardware; end of poll */
			break;
		}
		/* Handle completion */
		rc = flexboot_nodnic_complete ( ibdev, cq, &cqe_data );
		if ( rc != 0 ) {
			DBGC ( flexboot_nodnic, "flexboot_nodnic %p CQN %#lx failed to complete: %s\n",
					flexboot_nodnic, cq->cqn, strerror ( rc ) );
			DBGC_HDA ( flexboot_nodnic, virt_to_phys ( cqe ),
				   cqe, sizeof ( *cqe ) );
		}

		/* Update completion queue's index */
		cq->next_idx++;
	}
}
/***************************************************************************
 *
 * Queue pair operations
 *
 ***************************************************************************
 */


/**
 * Create queue pair
 *
 * @v ibdev		Infiniband device
 * @v qp		Queue pair
 * @ret rc		Return status code
 */
static int flexboot_nodnic_create_qp ( struct ib_device *ibdev,
			      struct ib_queue_pair *qp ) {
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	struct flexboot_nodnic_port *port = &flexboot_nodnic->port[ibdev->port - 1];
	struct flexboot_nodnic_queue_pair *flexboot_nodnic_qp;
	mlx_status status = MLX_SUCCESS;

	flexboot_nodnic_qp = (struct flexboot_nodnic_queue_pair *)zalloc(sizeof(*flexboot_nodnic_qp));
	if ( flexboot_nodnic_qp == NULL ) {
		status = MLX_OUT_OF_RESOURCES;
		goto qp_alloc_err;
	}

	status = nodnic_port_create_qp(&port->port_priv, qp->type,
			qp->send.num_wqes * sizeof(struct nodnic_send_wqbb),
			qp->send.num_wqes,
			qp->recv.num_wqes * sizeof(struct nodnic_recv_wqe),
			qp->recv.num_wqes,
			&flexboot_nodnic_qp->nodnic_queue_pair);
	MLX_FATAL_CHECK_STATUS(status, create_err,
			"nodnic_port_create_qp failed");
	ib_qp_set_drvdata ( qp, flexboot_nodnic_qp );
	return status;
create_err:
	free(flexboot_nodnic_qp);
qp_alloc_err:
	return status;
}

/**
 * Modify queue pair
 *
 * @v ibdev		Infiniband device
 * @v qp		Queue pair
 * @ret rc		Return status code
 */
static int flexboot_nodnic_modify_qp ( struct ib_device *ibdev __unused,
			      struct ib_queue_pair *qp __unused) {
	/*not needed*/
	return 0;
}

/**
 * Destroy queue pair
 *
 * @v ibdev		Infiniband device
 * @v qp		Queue pair
 */
static void flexboot_nodnic_destroy_qp ( struct ib_device *ibdev,
				struct ib_queue_pair *qp ) {
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	struct flexboot_nodnic_port *port = &flexboot_nodnic->port[ibdev->port - 1];
	struct flexboot_nodnic_queue_pair *flexboot_nodnic_qp = ib_qp_get_drvdata ( qp );

	nodnic_port_destroy_qp(&port->port_priv, qp->type,
			flexboot_nodnic_qp->nodnic_queue_pair);

	free(flexboot_nodnic_qp);
}

/***************************************************************************
 *
 * Work request operations
 *
 ***************************************************************************
 */

/**
 * Post send work queue entry
 *
 * @v ibdev		Infiniband device
 * @v qp		Queue pair
 * @v av		Address vector
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int flexboot_nodnic_post_send ( struct ib_device *ibdev,
			      struct ib_queue_pair *qp,
			      struct ib_address_vector *av,
			      struct io_buffer *iobuf) {

	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	struct flexboot_nodnic_queue_pair *flexboot_nodnic_qp = ib_qp_get_drvdata ( qp );
	struct flexboot_nodnic_port *port = &flexboot_nodnic->port[ibdev->port - 1];
	struct ib_work_queue *wq = &qp->send;
	struct nodnic_send_wqbb *wqbb;
	nodnic_qp *nodnic_qp = flexboot_nodnic_qp->nodnic_queue_pair;
	struct nodnic_send_ring *send_ring = &nodnic_qp->send;
	mlx_status status = MLX_SUCCESS;
	unsigned int wqe_idx_mask;
	unsigned long wqe_idx;

	if ( ( port->port_priv.dma_state == FALSE ) ||
		 ( port->port_priv.port_state & NODNIC_PORT_DISABLING_DMA ) ) {
		DBGC ( flexboot_nodnic, "flexboot_nodnic DMA disabled\n");
		status = -ENETDOWN;
		goto post_send_done;
	}

	/* Allocate work queue entry */
	wqe_idx = wq->next_idx;
	wqe_idx_mask = ( wq->num_wqes - 1 );
	if ( wq->iobufs[wqe_idx & wqe_idx_mask] ) {
		DBGC ( flexboot_nodnic, "flexboot_nodnic %p QPN %#lx send queue full\n",
				flexboot_nodnic, qp->qpn );
		status = -ENOBUFS;
		goto post_send_done;
	}
	wqbb = &send_ring->wqe_virt[wqe_idx & wqe_idx_mask];
	wq->iobufs[wqe_idx & wqe_idx_mask] = iobuf;

	assert ( flexboot_nodnic->callbacks->
			fill_send_wqe[qp->type] != NULL );
	status = flexboot_nodnic->callbacks->
			fill_send_wqe[qp->type] ( ibdev, qp, av, iobuf,
					wqbb, wqe_idx );
	if ( status != 0 ) {
		DBGC ( flexboot_nodnic, "flexboot_nodnic %p QPN %#lx fill send wqe failed\n",
				flexboot_nodnic, qp->qpn );
		goto post_send_done;
	}

	wq->next_idx++;

	status = port->port_priv.send_doorbell ( &port->port_priv,
				&send_ring->nodnic_ring, ( mlx_uint16 ) wq->next_idx );
	if ( status != 0 ) {
		DBGC ( flexboot_nodnic, "flexboot_nodnic %p ring send doorbell failed\n", flexboot_nodnic );
	}

post_send_done:
	return status;
}

/**
 * Post receive work queue entry
 *
 * @v ibdev		Infiniband device
 * @v qp		Queue pair
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int flexboot_nodnic_post_recv ( struct ib_device *ibdev,
			      struct ib_queue_pair *qp,
			      struct io_buffer *iobuf ) {
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	struct flexboot_nodnic_queue_pair *flexboot_nodnic_qp = ib_qp_get_drvdata ( qp );
	struct flexboot_nodnic_port *port = &flexboot_nodnic->port[ibdev->port - 1];
	struct ib_work_queue *wq = &qp->recv;
	nodnic_qp *nodnic_qp = flexboot_nodnic_qp->nodnic_queue_pair;
	struct nodnic_recv_ring *recv_ring = &nodnic_qp->receive;
	struct nodnic_recv_wqe *wqe;
	unsigned int wqe_idx_mask;
	mlx_status status = MLX_SUCCESS;

	/* Allocate work queue entry */
	wqe_idx_mask = ( wq->num_wqes - 1 );
	if ( wq->iobufs[wq->next_idx & wqe_idx_mask] ) {
		DBGC ( flexboot_nodnic,
				"flexboot_nodnic %p QPN %#lx receive queue full\n",
				flexboot_nodnic, qp->qpn );
		status = -ENOBUFS;
		goto post_recv_done;
	}
	wq->iobufs[wq->next_idx & wqe_idx_mask] = iobuf;
	wqe = &((struct nodnic_recv_wqe*)recv_ring->wqe_virt)[wq->next_idx & wqe_idx_mask];

	MLX_FILL_1 ( &wqe->data[0], 0, byte_count, iob_tailroom ( iobuf ) );
	MLX_FILL_1 ( &wqe->data[0], 1, l_key, flexboot_nodnic->device_priv.lkey );
	MLX_FILL_H ( &wqe->data[0], 2,
			 local_address_h, virt_to_bus ( iobuf->data ) );
	MLX_FILL_1 ( &wqe->data[0], 3,
			 local_address_l, virt_to_bus ( iobuf->data ) );

	wq->next_idx++;

	status = port->port_priv.recv_doorbell ( &port->port_priv,
				&recv_ring->nodnic_ring, ( mlx_uint16 ) wq->next_idx );
	if ( status != 0 ) {
		DBGC ( flexboot_nodnic, "flexboot_nodnic %p ring receive doorbell failed\n", flexboot_nodnic );
	}
post_recv_done:
	return status;
}

/***************************************************************************
 *
 * Event queues
 *
 ***************************************************************************
 */

static void flexboot_nodnic_poll_eq ( struct ib_device *ibdev ) {
	struct flexboot_nodnic *flexboot_nodnic;
	struct flexboot_nodnic_port *port;
	struct net_device *netdev;
	nodnic_port_state state = 0;
	mlx_status status;

	if ( ! ibdev ) {
		DBG ( "%s: ibdev = NULL!!!\n", __FUNCTION__ );
		return;
	}

	flexboot_nodnic = ib_get_drvdata ( ibdev );
	port = &flexboot_nodnic->port[ibdev->port - 1];
	netdev = port->netdev;

	if ( ! netdev_is_open ( netdev ) ) {
		DBG2( "%s: port %d is closed\n", __FUNCTION__, port->ibdev->port );
		return;
	}

	/* we don't poll EQ. Just poll link status if it's not active */
	if ( ! netdev_link_ok ( netdev ) ) {
		status = nodnic_port_get_state ( &port->port_priv, &state );
		MLX_FATAL_CHECK_STATUS(status, state_err, "nodnic_port_get_state failed");

		if ( state == nodnic_port_state_active ) {
			DBG( "%s: port %d physical link is up\n", __FUNCTION__,
					port->ibdev->port );
			port->type->state_change ( flexboot_nodnic, port, 1 );
		}
	}
state_err:
	return;
}

/***************************************************************************
 *
 * Multicast group operations
 *
 ***************************************************************************
 */
static int flexboot_nodnic_mcast_attach ( struct ib_device *ibdev,
				 struct ib_queue_pair *qp,
				 union ib_gid *gid) {
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	struct flexboot_nodnic_port *port = &flexboot_nodnic->port[ibdev->port - 1];
	mlx_mac_address mac;
	mlx_status status = MLX_SUCCESS;

	switch (qp->type) {
	case IB_QPT_ETH:
		memcpy(&mac, &gid, sizeof(mac));
		status = nodnic_port_add_mac_filter(&port->port_priv, mac);
		MLX_CHECK_STATUS(flexboot_nodnic->device_priv, status, mac_err,
				"nodnic_port_add_mac_filter failed");
		break;
	default:
		break;
	}
mac_err:
	return status;
}
static void flexboot_nodnic_mcast_detach ( struct ib_device *ibdev,
				  struct ib_queue_pair *qp,
				  union ib_gid *gid ) {
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	struct flexboot_nodnic_port *port = &flexboot_nodnic->port[ibdev->port - 1];
	mlx_mac_address mac;
	mlx_status status = MLX_SUCCESS;

	switch (qp->type) {
	case IB_QPT_ETH:
		memcpy(&mac, &gid, sizeof(mac));
		status = nodnic_port_remove_mac_filter(&port->port_priv, mac);
		MLX_CHECK_STATUS(flexboot_nodnic->device_priv, status, mac_err,
				"nodnic_port_remove_mac_filter failed");
		break;
	default:
		break;
	}
mac_err:
	return;
}
/***************************************************************************
 *
 * Infiniband link-layer operations
 *
 ***************************************************************************
 */

/**
 * Initialise Infiniband link
 *
 * @v ibdev		Infiniband device
 * @ret rc		Return status code
 */
static int flexboot_nodnic_ib_open ( struct ib_device *ibdev __unused) {
	int rc = 0;

	/*TODO: add implementation*/
	return rc;
}

/**
 * Close Infiniband link
 *
 * @v ibdev		Infiniband device
 */
static void flexboot_nodnic_ib_close ( struct ib_device *ibdev __unused) {
	/*TODO: add implementation*/
}

/**
 * Inform embedded subnet management agent of a received MAD
 *
 * @v ibdev		Infiniband device
 * @v mad		MAD
 * @ret rc		Return status code
 */
static int flexboot_nodnic_inform_sma ( struct ib_device *ibdev __unused,
			       union ib_mad *mad __unused) {
	/*TODO: add implementation*/
	return 0;
}

/** flexboot_nodnic Infiniband operations */
static struct ib_device_operations flexboot_nodnic_ib_operations = {
	.create_cq	= flexboot_nodnic_create_cq,
	.destroy_cq	= flexboot_nodnic_destroy_cq,
	.create_qp	= flexboot_nodnic_create_qp,
	.modify_qp	= flexboot_nodnic_modify_qp,
	.destroy_qp	= flexboot_nodnic_destroy_qp,
	.post_send	= flexboot_nodnic_post_send,
	.post_recv	= flexboot_nodnic_post_recv,
	.poll_cq	= flexboot_nodnic_poll_cq,
	.poll_eq	= flexboot_nodnic_poll_eq,
	.open		= flexboot_nodnic_ib_open,
	.close		= flexboot_nodnic_ib_close,
	.mcast_attach	= flexboot_nodnic_mcast_attach,
	.mcast_detach	= flexboot_nodnic_mcast_detach,
	.set_port_info	= flexboot_nodnic_inform_sma,
	.set_pkey_table	= flexboot_nodnic_inform_sma,
};
/***************************************************************************
 *
 *
 *
 ***************************************************************************
 */

#define FLEX_NODNIC_TX_POLL_TOUT	500000
#define FLEX_NODNIC_TX_POLL_USLEEP	10

static void flexboot_nodnic_complete_all_tx ( struct flexboot_nodnic_port *port ) {
	struct ib_device *ibdev = port->ibdev;
	struct ib_completion_queue *cq;
	struct ib_work_queue *wq;
	int keep_polling = 0;
	int timeout = FLEX_NODNIC_TX_POLL_TOUT;

	list_for_each_entry ( cq, &ibdev->cqs, list ) {
		do {
			ib_poll_cq ( ibdev, cq );
			keep_polling = 0;
			list_for_each_entry ( wq, &cq->work_queues, list ) {
				if ( wq->is_send )
					keep_polling += ( wq->fill > 0 );
			}
			udelay ( FLEX_NODNIC_TX_POLL_USLEEP );
		} while ( keep_polling && ( timeout-- > 0 ) );
	}
}

static void flexboot_nodnic_port_disable_dma ( struct flexboot_nodnic_port *port ) {
	nodnic_port_priv *port_priv = & ( port->port_priv );
	mlx_status status;

	if ( ! ( port_priv->port_state & NODNIC_PORT_OPENED ) )
		return;

	port_priv->port_state |= NODNIC_PORT_DISABLING_DMA;
	flexboot_nodnic_complete_all_tx ( port );
	if ( ( status = nodnic_port_disable_dma ( port_priv ) ) ) {
		MLX_DEBUG_WARN ( port, "Failed to disable DMA %d\n", status );
	}

	port_priv->port_state &= ~NODNIC_PORT_DISABLING_DMA;
}

/***************************************************************************
 *
 * Ethernet operation
 *
 ***************************************************************************
 */

/** Number of flexboot_nodnic Ethernet send work queue entries */
#define FLEXBOOT_NODNIC_ETH_NUM_SEND_WQES 64

/** Number of flexboot_nodnic Ethernet receive work queue entries */
#define FLEXBOOT_NODNIC_ETH_NUM_RECV_WQES 64
/** flexboot nodnic Ethernet queue pair operations */
static struct ib_queue_pair_operations flexboot_nodnic_eth_qp_op = {
	.alloc_iob = alloc_iob,
};

/**
 * Transmit packet via flexboot_nodnic Ethernet device
 *
 * @v netdev		Network device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int flexboot_nodnic_eth_transmit ( struct net_device *netdev,
				 struct io_buffer *iobuf) {
	struct flexboot_nodnic_port *port = netdev->priv;
	struct ib_device *ibdev = port->ibdev;
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	int rc;

	rc = ib_post_send ( ibdev, port->eth_qp, NULL, iobuf);
	/* Transmit packet */
	if ( rc != 0) {
		DBGC ( flexboot_nodnic, "NODNIC %p port %d could not transmit: %s\n",
				flexboot_nodnic, ibdev->port, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Handle flexboot_nodnic Ethernet device send completion
 *
 * @v ibdev		Infiniband device
 * @v qp		Queue pair
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void flexboot_nodnic_eth_complete_send ( struct ib_device *ibdev __unused,
				       struct ib_queue_pair *qp,
				       struct io_buffer *iobuf,
					   int rc) {
	struct net_device *netdev = ib_qp_get_ownerdata ( qp );

	netdev_tx_complete_err ( netdev, iobuf, rc );
}

/**
 * Handle flexboot_nodnic Ethernet device receive completion
 *
 * @v ibdev		Infiniband device
 * @v qp		Queue pair
 * @v av		Address vector, or NULL
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void flexboot_nodnic_eth_complete_recv ( struct ib_device *ibdev __unused,
				struct ib_queue_pair *qp,
				struct ib_address_vector *dest __unused,
				struct ib_address_vector *source,
				 struct io_buffer *iobuf,
				int rc) {
	struct net_device *netdev = ib_qp_get_ownerdata ( qp );

	if ( rc != 0 ) {
		DBG ( "Received packet with error\n" );
		netdev_rx_err ( netdev, iobuf, rc );
		return;
	}

	if ( source == NULL ) {
		DBG ( "Received packet without address vector\n" );
		netdev_rx_err ( netdev, iobuf, -ENOTTY );
		return;
	}
	netdev_rx ( netdev, iobuf );
}

/** flexboot_nodnic Ethernet device completion operations */
static struct ib_completion_queue_operations flexboot_nodnic_eth_cq_op = {
	.complete_send = flexboot_nodnic_eth_complete_send,
	.complete_recv = flexboot_nodnic_eth_complete_recv,
};

/**
 * Poll flexboot_nodnic Ethernet device
 *
 * @v netdev		Network device
 */
static void flexboot_nodnic_eth_poll ( struct net_device *netdev) {
	struct flexboot_nodnic_port *port = netdev->priv;
	struct ib_device *ibdev = port->ibdev;

	ib_poll_eq ( ibdev );
}

/**
 * Open flexboot_nodnic Ethernet device
 *
 * @v netdev		Network device
 * @ret rc		Return status code
 */
static int flexboot_nodnic_eth_open ( struct net_device *netdev ) {
	struct flexboot_nodnic_port *port = netdev->priv;
	struct ib_device *ibdev = port->ibdev;
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	mlx_status status = MLX_SUCCESS;
	struct ib_completion_queue *dummy_cq = NULL;
	struct flexboot_nodnic_queue_pair *flexboot_nodnic_qp = NULL;
	mlx_uint64	cq_size = 0;
	mlx_uint32	qpn = 0;
	nodnic_port_state state = nodnic_port_state_down;

	if ( port->port_priv.port_state & NODNIC_PORT_OPENED ) {
		DBGC ( flexboot_nodnic, "%s: port %d is already opened\n",
				__FUNCTION__, port->ibdev->port );
		return 0;
	}

	port->port_priv.port_state |= NODNIC_PORT_OPENED;

	dummy_cq = zalloc ( sizeof ( struct ib_completion_queue ) );
	if ( dummy_cq == NULL ) {
		DBGC ( flexboot_nodnic, "%s: Failed to allocate dummy CQ\n", __FUNCTION__ );
		status = MLX_OUT_OF_RESOURCES;
		goto err_create_dummy_cq;
	}
	INIT_LIST_HEAD ( &dummy_cq->work_queues );

	port->eth_qp = ib_create_qp ( ibdev, IB_QPT_ETH,
					FLEXBOOT_NODNIC_ETH_NUM_SEND_WQES, dummy_cq,
					FLEXBOOT_NODNIC_ETH_NUM_RECV_WQES, dummy_cq,
					&flexboot_nodnic_eth_qp_op, netdev->name );
	if ( !port->eth_qp ) {
		DBGC ( flexboot_nodnic, "flexboot_nodnic %p port %d could not create queue pair\n",
				 flexboot_nodnic, ibdev->port );
		status = MLX_OUT_OF_RESOURCES;
		goto err_create_qp;
	}

	ib_qp_set_ownerdata ( port->eth_qp, netdev );

	status = nodnic_port_get_cq_size(&port->port_priv, &cq_size);
	MLX_FATAL_CHECK_STATUS(status, get_cq_size_err,
			"nodnic_port_get_cq_size failed");

	port->eth_cq = ib_create_cq ( ibdev, cq_size,
			&flexboot_nodnic_eth_cq_op );
	if ( !port->eth_cq ) {
		DBGC ( flexboot_nodnic,
			"flexboot_nodnic %p port %d could not create completion queue\n",
			flexboot_nodnic, ibdev->port );
		status = MLX_OUT_OF_RESOURCES;
		goto err_create_cq;
	}
	port->eth_qp->send.cq = port->eth_cq;
	list_del(&port->eth_qp->send.list);
	list_add ( &port->eth_qp->send.list, &port->eth_cq->work_queues );
	port->eth_qp->recv.cq = port->eth_cq;
	list_del(&port->eth_qp->recv.list);
	list_add ( &port->eth_qp->recv.list, &port->eth_cq->work_queues );

	status = nodnic_port_allocate_eq(&port->port_priv,
		flexboot_nodnic->device_priv.device_cap.log_working_buffer_size);
	MLX_FATAL_CHECK_STATUS(status, eq_alloc_err,
				"nodnic_port_allocate_eq failed");

	status = nodnic_port_init(&port->port_priv);
	MLX_FATAL_CHECK_STATUS(status, init_err,
					"nodnic_port_init failed");

	/* update qp - qpn */
	flexboot_nodnic_qp = ib_qp_get_drvdata ( port->eth_qp );
	status = nodnic_port_get_qpn(&port->port_priv,
			&flexboot_nodnic_qp->nodnic_queue_pair->send.nodnic_ring,
			&qpn);
	MLX_FATAL_CHECK_STATUS(status, qpn_err,
						"nodnic_port_get_qpn failed");
	port->eth_qp->qpn = qpn;

	/* Fill receive rings */
	ib_refill_recv ( ibdev, port->eth_qp );

	status = nodnic_port_enable_dma(&port->port_priv);
	MLX_FATAL_CHECK_STATUS(status, dma_err,
					"nodnic_port_enable_dma failed");

	if (flexboot_nodnic->device_priv.device_cap.support_promisc_filter) {
		status = nodnic_port_set_promisc(&port->port_priv, TRUE);
		MLX_FATAL_CHECK_STATUS(status, promisc_err,
							"nodnic_port_set_promisc failed");
	}

	status = nodnic_port_get_state(&port->port_priv, &state);
	MLX_FATAL_CHECK_STATUS(status, state_err,
						"nodnic_port_get_state failed");

	port->type->state_change (
			flexboot_nodnic, port, state == nodnic_port_state_active );

	DBGC ( flexboot_nodnic, "%s: port %d opened (link is %s)\n",
			__FUNCTION__, port->ibdev->port,
			( ( state == nodnic_port_state_active ) ? "Up" : "Down" ) );

	free(dummy_cq);
	return 0;
state_err:
promisc_err:
dma_err:
qpn_err:
	nodnic_port_close(&port->port_priv);
init_err:
	nodnic_port_free_eq(&port->port_priv);
eq_alloc_err:
err_create_cq:
get_cq_size_err:
	ib_destroy_qp(ibdev, port->eth_qp );
err_create_qp:
	free(dummy_cq);
err_create_dummy_cq:
	port->port_priv.port_state &= ~NODNIC_PORT_OPENED;
	return status;
}

/**
 * Close flexboot_nodnic Ethernet device
 *
 * @v netdev		Network device
 */
static void flexboot_nodnic_eth_close ( struct net_device *netdev) {
	struct flexboot_nodnic_port *port = netdev->priv;
	struct ib_device *ibdev = port->ibdev;
	struct flexboot_nodnic *flexboot_nodnic = ib_get_drvdata ( ibdev );
	mlx_status status = MLX_SUCCESS;

	if ( ! ( port->port_priv.port_state & NODNIC_PORT_OPENED ) ) {
		DBGC ( flexboot_nodnic, "%s: port %d is already closed\n",
				__FUNCTION__, port->ibdev->port );
		return;
	}

	if (flexboot_nodnic->device_priv.device_cap.support_promisc_filter) {
		if ( ( status = nodnic_port_set_promisc( &port->port_priv, FALSE ) ) ) {
			DBGC ( flexboot_nodnic,
					"nodnic_port_set_promisc failed (status = %d)\n", status );
		}
	}

	flexboot_nodnic_port_disable_dma ( port );

	port->port_priv.port_state &= ~NODNIC_PORT_OPENED;

	port->type->state_change ( flexboot_nodnic, port, FALSE );

	/* Close port */
	status = nodnic_port_close(&port->port_priv);
	if ( status != MLX_SUCCESS ) {
		DBGC ( flexboot_nodnic, "flexboot_nodnic %p port %d could not close port: %s\n",
				flexboot_nodnic, ibdev->port, strerror ( status ) );
		/* Nothing we can do about this */
	}

	ib_destroy_qp ( ibdev, port->eth_qp );
	port->eth_qp = NULL;
	ib_destroy_cq ( ibdev, port->eth_cq );
	port->eth_cq = NULL;

	nodnic_port_free_eq(&port->port_priv);

	DBGC ( flexboot_nodnic, "%s: port %d closed\n", __FUNCTION__, port->ibdev->port );
}

void flexboot_nodnic_eth_irq ( struct net_device *netdev, int enable ) {
	struct flexboot_nodnic_port *port = netdev->priv;

	if ( enable ) {
		if ( ( port->port_priv.port_state & NODNIC_PORT_OPENED ) &&
			 ! ( port->port_priv.port_state & NODNIC_PORT_DISABLING_DMA ) ) {
			flexboot_nodnic_arm_cq ( port );
		} else {
			/* do nothing */
		}
	} else {
		nodnic_device_clear_int( port->port_priv.device );
	}
}

/** flexboot_nodnic Ethernet network device operations */
static struct net_device_operations flexboot_nodnic_eth_operations = {
	.open		= flexboot_nodnic_eth_open,
	.close		= flexboot_nodnic_eth_close,
	.transmit	= flexboot_nodnic_eth_transmit,
	.poll		= flexboot_nodnic_eth_poll,
};

/**
 * Register flexboot_nodnic Ethernet device
 */
static int flexboot_nodnic_register_netdev ( struct flexboot_nodnic *flexboot_nodnic,
				    struct flexboot_nodnic_port *port) {
	mlx_status status = MLX_SUCCESS;
	struct net_device	*netdev;
	struct ib_device	*ibdev = port->ibdev;
	union {
		uint8_t bytes[8];
		uint32_t dwords[2];
	} mac;

	/* Allocate network devices */
	netdev = alloc_etherdev ( 0 );
	if ( netdev == NULL ) {
		DBGC ( flexboot_nodnic, "flexboot_nodnic %p port %d could not allocate net device\n",
				flexboot_nodnic, ibdev->port );
		status = MLX_OUT_OF_RESOURCES;
		goto alloc_err;
	}
	port->netdev = netdev;
	netdev_init ( netdev, &flexboot_nodnic_eth_operations );
	netdev->dev = ibdev->dev;
	netdev->priv = port;

	status = nodnic_port_query(&port->port_priv,
			nodnic_port_option_mac_high,
			&mac.dwords[0]);
	MLX_FATAL_CHECK_STATUS(status, mac_err,
			"failed to query mac high");
	status = nodnic_port_query(&port->port_priv,
			nodnic_port_option_mac_low,
			&mac.dwords[1]);
	MLX_FATAL_CHECK_STATUS(status, mac_err,
				"failed to query mac low");
	mac.dwords[0] = htonl(mac.dwords[0]);
	mac.dwords[1] = htonl(mac.dwords[1]);
	memcpy ( netdev->hw_addr,
			 &mac.bytes[2], ETH_ALEN);
	/* Register network device */
	status = register_netdev ( netdev );
	if ( status != MLX_SUCCESS ) {
		DBGC ( flexboot_nodnic,
			"flexboot_nodnic %p port %d could not register network device: %s\n",
			flexboot_nodnic, ibdev->port, strerror ( status ) );
		goto reg_err;
	}
	return status;
reg_err:
mac_err:
	netdev_put ( netdev );
alloc_err:
	return status;
}

/**
 * Handle flexboot_nodnic Ethernet device port state change
 */
static void flexboot_nodnic_state_change_netdev ( struct flexboot_nodnic *flexboot_nodnic __unused,
					 struct flexboot_nodnic_port *port,
					 int link_up ) {
	struct net_device *netdev = port->netdev;

	if ( link_up )
		netdev_link_up ( netdev );
	else
		netdev_link_down ( netdev );

}

/**
 * Unregister flexboot_nodnic Ethernet device
 */
static void flexboot_nodnic_unregister_netdev ( struct flexboot_nodnic *flexboot_nodnic __unused,
				       struct flexboot_nodnic_port *port ) {
	struct net_device *netdev = port->netdev;
	unregister_netdev ( netdev );
	netdev_nullify ( netdev );
	netdev_put ( netdev );
}

/** flexboot_nodnic Ethernet port type */
static struct flexboot_nodnic_port_type flexboot_nodnic_port_type_eth = {
	.register_dev = flexboot_nodnic_register_netdev,
	.state_change = flexboot_nodnic_state_change_netdev,
	.unregister_dev = flexboot_nodnic_unregister_netdev,
};

/***************************************************************************
 *
 * PCI interface helper functions
 *
 ***************************************************************************
 */
static
mlx_status
flexboot_nodnic_allocate_infiniband_devices( struct flexboot_nodnic *flexboot_nodnic_priv ) {
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv *device_priv = &flexboot_nodnic_priv->device_priv;
	struct pci_device *pci = flexboot_nodnic_priv->pci;
	struct ib_device *ibdev = NULL;
	unsigned int i = 0;

	/* Allocate Infiniband devices */
	for (; i < device_priv->device_cap.num_ports; i++) {
		if ( ! ( flexboot_nodnic_priv->port_mask & ( i + 1 ) ) )
			continue;
		ibdev = alloc_ibdev(0);
		if (ibdev == NULL) {
			status = MLX_OUT_OF_RESOURCES;
			goto err_alloc_ibdev;
		}
		flexboot_nodnic_priv->port[i].ibdev = ibdev;
		ibdev->op = &flexboot_nodnic_ib_operations;
		ibdev->dev = &pci->dev;
		ibdev->port = ( FLEXBOOT_NODNIC_PORT_BASE + i);
		ib_set_drvdata(ibdev, flexboot_nodnic_priv);
	}
	return status;
err_alloc_ibdev:
	for ( i-- ; ( signed int ) i >= 0 ; i-- )
		ibdev_put ( flexboot_nodnic_priv->port[i].ibdev );
	return status;
}

static
mlx_status
flexboot_nodnic_thin_init_ports( struct flexboot_nodnic *flexboot_nodnic_priv ) {
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv *device_priv = &flexboot_nodnic_priv->device_priv;
	nodnic_port_priv *port_priv = NULL;
	unsigned int i = 0;

	for ( i = 0; i < device_priv->device_cap.num_ports; i++ ) {
		if ( ! ( flexboot_nodnic_priv->port_mask & ( i + 1 ) ) )
			continue;
		port_priv = &flexboot_nodnic_priv->port[i].port_priv;
		status = nodnic_port_thin_init( device_priv, port_priv, i );
		MLX_FATAL_CHECK_STATUS(status, thin_init_err,
				"flexboot_nodnic_thin_init_ports failed");
	}
thin_init_err:
	return status;
}


static
mlx_status
flexboot_nodnic_set_ports_type ( struct flexboot_nodnic *flexboot_nodnic_priv ) {
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv	*device_priv = &flexboot_nodnic_priv->device_priv;
	nodnic_port_priv	*port_priv = NULL;
	nodnic_port_type	type = NODNIC_PORT_TYPE_UNKNOWN;
	unsigned int i = 0;

	for ( i = 0 ; i < device_priv->device_cap.num_ports ; i++ ) {
		if ( ! ( flexboot_nodnic_priv->port_mask & ( i + 1 ) ) )
			continue;
		port_priv = &flexboot_nodnic_priv->port[i].port_priv;
		status = nodnic_port_get_type(port_priv, &type);
		MLX_FATAL_CHECK_STATUS(status, type_err,
				"nodnic_port_get_type failed");
		switch ( type ) {
		case NODNIC_PORT_TYPE_ETH:
			DBGC ( flexboot_nodnic_priv, "Port %d type is Ethernet\n", i );
			flexboot_nodnic_priv->port[i].type = &flexboot_nodnic_port_type_eth;
			break;
		case NODNIC_PORT_TYPE_IB:
			DBGC ( flexboot_nodnic_priv, "Port %d type is Infiniband\n", i );
			status = MLX_UNSUPPORTED;
			goto type_err;
		default:
			DBGC ( flexboot_nodnic_priv, "Port %d type is unknown\n", i );
			status = MLX_UNSUPPORTED;
			goto type_err;
		}
	}
type_err:
	return status;
}

static
mlx_status
flexboot_nodnic_ports_register_dev( struct flexboot_nodnic *flexboot_nodnic_priv ) {
	mlx_status status = MLX_SUCCESS;
	nodnic_device_priv *device_priv = &flexboot_nodnic_priv->device_priv;
	struct flexboot_nodnic_port *port = NULL;
	unsigned int i = 0;

	for (; i < device_priv->device_cap.num_ports; i++) {
		if ( ! ( flexboot_nodnic_priv->port_mask & ( i + 1 ) ) )
			continue;
		port = &flexboot_nodnic_priv->port[i];
		status = port->type->register_dev ( flexboot_nodnic_priv, port );
		MLX_FATAL_CHECK_STATUS(status, reg_err,
				"port register_dev failed");
	}
reg_err:
	return status;
}

static
mlx_status
flexboot_nodnic_ports_unregister_dev ( struct flexboot_nodnic *flexboot_nodnic_priv ) {
	struct flexboot_nodnic_port *port;
	nodnic_device_priv	*device_priv = &flexboot_nodnic_priv->device_priv;
	int i = (device_priv->device_cap.num_ports - 1);

	for (; i >= 0; i--) {
		if ( ! ( flexboot_nodnic_priv->port_mask & ( i + 1 ) ) )
			continue;
		port = &flexboot_nodnic_priv->port[i];
		port->type->unregister_dev(flexboot_nodnic_priv, port);
		ibdev_put(flexboot_nodnic_priv->port[i].ibdev);
	}
	return MLX_SUCCESS;
}

/***************************************************************************
 *
 * flexboot nodnic interface
 *
 ***************************************************************************
 */
__unused static void flexboot_nodnic_enable_dma ( struct flexboot_nodnic *nodnic ) {
	nodnic_port_priv *port_priv;
	mlx_status status;
	int i;

	for ( i = 0; i < nodnic->device_priv.device_cap.num_ports; i++ ) {
		if ( ! ( nodnic->port_mask & ( i + 1 ) ) )
			continue;
		port_priv = & ( nodnic->port[i].port_priv );
		if ( ! ( port_priv->port_state & NODNIC_PORT_OPENED ) )
			continue;

		if ( ( status = nodnic_port_enable_dma ( port_priv ) ) ) {
			MLX_DEBUG_WARN ( nodnic, "Failed to enable DMA %d\n", status );
		}
	}
}

__unused static void flexboot_nodnic_disable_dma ( struct flexboot_nodnic *nodnic ) {
	int i;

	for ( i = 0; i < nodnic->device_priv.device_cap.num_ports; i++ ) {
		if ( ! ( nodnic->port_mask & ( i + 1 ) ) )
			continue;
		flexboot_nodnic_port_disable_dma ( & ( nodnic->port[i] ) );
	}
}

int flexboot_nodnic_is_supported ( struct pci_device *pci ) {
	mlx_utils utils;
	mlx_pci_gw_buffer buffer;
	mlx_status status;
	int is_supported = 0;

	DBG ( "%s: start\n", __FUNCTION__ );

	memset ( &utils, 0, sizeof ( utils ) );

	status = mlx_utils_init ( &utils, pci );
	MLX_CHECK_STATUS ( pci, status, utils_init_err, "mlx_utils_init failed" );

	status = mlx_pci_gw_init ( &utils );
	MLX_CHECK_STATUS ( pci, status, pci_gw_init_err, "mlx_pci_gw_init failed" );

	status = mlx_pci_gw_read ( &utils, PCI_GW_SPACE_NODNIC,
			NODNIC_NIC_INTERFACE_SUPPORTED_OFFSET, &buffer );

	if ( status == MLX_SUCCESS ) {
		buffer >>= NODNIC_NIC_INTERFACE_SUPPORTED_BIT;
		is_supported = ( buffer & 0x1 );
	}

	mlx_pci_gw_teardown( &utils );

pci_gw_init_err:
utils_init_err:
	DBG ( "%s: NODNIC is %s supported (status = %d)\n",
			__FUNCTION__, ( is_supported ? "": "not" ), status );
	return is_supported;
}

void flexboot_nodnic_copy_mac ( uint8_t mac_addr[], uint32_t low_byte,
		uint16_t high_byte ) {
	union mac_addr {
		struct {
			uint32_t low_byte;
			uint16_t high_byte;
		};
		uint8_t mac_addr[ETH_ALEN];
	} mac_addr_aux;

	mac_addr_aux.high_byte = high_byte;
	mac_addr_aux.low_byte = low_byte;

	mac_addr[0] = mac_addr_aux.mac_addr[5];
	mac_addr[1] = mac_addr_aux.mac_addr[4];
	mac_addr[2] = mac_addr_aux.mac_addr[3];
	mac_addr[3] = mac_addr_aux.mac_addr[2];
	mac_addr[4] = mac_addr_aux.mac_addr[1];
	mac_addr[5] = mac_addr_aux.mac_addr[0];
}

static mlx_status flexboot_nodnic_get_factory_mac (
		struct flexboot_nodnic *flexboot_nodnic_priv, uint8_t port __unused ) {
	struct mlx_vmac_query_virt_mac virt_mac;
	mlx_status status;

	memset ( & virt_mac, 0, sizeof ( virt_mac ) );
	status = mlx_vmac_query_virt_mac ( flexboot_nodnic_priv->device_priv.utils,
			&virt_mac );
	if ( ! status ) {
		DBGC ( flexboot_nodnic_priv, "NODNIC %p Failed to set the virtual MAC\n",
			flexboot_nodnic_priv );
	}

	return status;
}

/**
 * Set port masking
 *
 * @v flexboot_nodnic		nodnic device
 * @ret rc		Return status code
 */
static int flexboot_nodnic_set_port_masking ( struct flexboot_nodnic *flexboot_nodnic ) {
	unsigned int i;
	nodnic_device_priv *device_priv = &flexboot_nodnic->device_priv;

	flexboot_nodnic->port_mask = 0;
	for ( i = 0; i < device_priv->device_cap.num_ports; i++ ) {
		flexboot_nodnic->port_mask |= (i + 1);
	}

	if ( ! flexboot_nodnic->port_mask ) {
		/* No port was enabled */
		DBGC ( flexboot_nodnic, "NODNIC %p No port was enabled for "
				"booting\n", flexboot_nodnic );
		return -ENETUNREACH;
	}

	return 0;
}

int flexboot_nodnic_probe ( struct pci_device *pci,
		struct flexboot_nodnic_callbacks *callbacks,
		void *drv_priv __unused ) {
	mlx_status status = MLX_SUCCESS;
	struct flexboot_nodnic *flexboot_nodnic_priv = NULL;
	nodnic_device_priv *device_priv = NULL;
	int i = 0;

	if ( ( pci == NULL ) || ( callbacks == NULL ) ) {
		DBGC ( flexboot_nodnic_priv, "%s: Bad Parameter\n", __FUNCTION__ );
		return -EINVAL;
	}

	flexboot_nodnic_priv = zalloc( sizeof ( *flexboot_nodnic_priv ) );
	if ( flexboot_nodnic_priv == NULL ) {
		DBGC ( flexboot_nodnic_priv, "%s: Failed to allocate priv data\n", __FUNCTION__ );
		status = MLX_OUT_OF_RESOURCES;
		goto device_err_alloc;
	}

	/* Register settings
	 * Note that pci->priv will be the device private data */
	flexboot_nodnic_priv->pci = pci;
	flexboot_nodnic_priv->callbacks = callbacks;
	pci_set_drvdata ( pci, flexboot_nodnic_priv );

	device_priv = &flexboot_nodnic_priv->device_priv;
	device_priv->utils = (mlx_utils *)zalloc( sizeof ( mlx_utils ) );
	if ( device_priv->utils == NULL ) {
		DBGC ( flexboot_nodnic_priv, "%s: Failed to allocate utils\n", __FUNCTION__ );
		status = MLX_OUT_OF_RESOURCES;
		goto utils_err_alloc;
	}

	status = mlx_utils_init( device_priv->utils, pci );
	MLX_FATAL_CHECK_STATUS(status, utils_init_err,
			"mlx_utils_init failed");

	/* nodnic init*/
	status = mlx_pci_gw_init( device_priv->utils );
	MLX_FATAL_CHECK_STATUS(status, cmd_init_err,
			"mlx_pci_gw_init failed");

	/* init device */
	status = nodnic_device_init( device_priv );
	MLX_FATAL_CHECK_STATUS(status, device_init_err,
				"nodnic_device_init failed");

	status = nodnic_device_get_cap( device_priv );
	MLX_FATAL_CHECK_STATUS(status, get_cap_err,
					"nodnic_device_get_cap failed");

	status =  flexboot_nodnic_set_port_masking ( flexboot_nodnic_priv );
	MLX_FATAL_CHECK_STATUS(status, err_set_masking,
						"flexboot_nodnic_set_port_masking failed");

	status = flexboot_nodnic_allocate_infiniband_devices( flexboot_nodnic_priv );
	MLX_FATAL_CHECK_STATUS(status, err_alloc_ibdev,
					"flexboot_nodnic_allocate_infiniband_devices failed");

	/* port init */
	status = flexboot_nodnic_thin_init_ports( flexboot_nodnic_priv );
	MLX_FATAL_CHECK_STATUS(status, err_thin_init_ports,
						"flexboot_nodnic_thin_init_ports failed");

	/* device reg */
	status = flexboot_nodnic_set_ports_type( flexboot_nodnic_priv );
	MLX_CHECK_STATUS( flexboot_nodnic_priv, status, err_set_ports_types,
						"flexboot_nodnic_set_ports_type failed");

	status = flexboot_nodnic_ports_register_dev( flexboot_nodnic_priv );
	MLX_FATAL_CHECK_STATUS(status, reg_err,
					"flexboot_nodnic_ports_register_dev failed");

	for ( i = 0; i < device_priv->device_cap.num_ports; i++ ) {
		if ( ! ( flexboot_nodnic_priv->port_mask & ( i + 1 ) ) )
			continue;
		flexboot_nodnic_get_factory_mac ( flexboot_nodnic_priv, i );
	}

	/* Update ETH operations with IRQ function if supported */
	DBGC ( flexboot_nodnic_priv, "%s: %s IRQ function\n",
			__FUNCTION__, ( callbacks->irq ? "Valid" : "No" ) );
	flexboot_nodnic_eth_operations.irq = callbacks->irq;
	return 0;

	flexboot_nodnic_ports_unregister_dev ( flexboot_nodnic_priv );
reg_err:
err_set_ports_types:
err_thin_init_ports:
err_alloc_ibdev:
err_set_masking:
get_cap_err:
	nodnic_device_teardown ( device_priv );
device_init_err:
	mlx_pci_gw_teardown ( device_priv->utils );
cmd_init_err:
utils_init_err:
	free ( device_priv->utils );
utils_err_alloc:
	free ( flexboot_nodnic_priv );
device_err_alloc:
	return status;
}

void flexboot_nodnic_remove ( struct pci_device *pci )
{
	struct flexboot_nodnic *flexboot_nodnic_priv = pci_get_drvdata ( pci );
	nodnic_device_priv *device_priv = & ( flexboot_nodnic_priv->device_priv );

	flexboot_nodnic_ports_unregister_dev ( flexboot_nodnic_priv );
	nodnic_device_teardown( device_priv );
	mlx_pci_gw_teardown( device_priv->utils );
	free( device_priv->utils );
	free( flexboot_nodnic_priv );
}

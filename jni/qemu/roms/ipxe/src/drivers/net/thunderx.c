/*
 * Copyright (C) 2016 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <byteswap.h>
#include <ipxe/netdevice.h>
#include <ipxe/ethernet.h>
#include <ipxe/if_ether.h>
#include <ipxe/iobuf.h>
#include <ipxe/malloc.h>
#include <ipxe/pci.h>
#include <ipxe/pciea.h>
#include <ipxe/umalloc.h>
#include "thunderx.h"
#include "thunderxcfg.h"

/** @file
 *
 * Cavium ThunderX Ethernet driver
 *
 */

/** List of BGX Ethernet interfaces */
static LIST_HEAD ( txnic_bgxs );

/** List of physical functions */
static LIST_HEAD ( txnic_pfs );

/** Debug colour for physical function and BGX messages */
#define TXNICCOL(x) ( &txnic_pfs + (x)->node )

/** Board configuration protocol */
static EFI_THUNDER_CONFIG_PROTOCOL *txcfg;
EFI_REQUEST_PROTOCOL ( EFI_THUNDER_CONFIG_PROTOCOL, &txcfg );

/******************************************************************************
 *
 * Diagnostics
 *
 ******************************************************************************
 */

/**
 * Show virtual NIC diagnostics (for debugging)
 *
 * @v vnic		Virtual NIC
 */
static __attribute__ (( unused )) void txnic_diag ( struct txnic *vnic ) {

	DBGC ( vnic, "TXNIC %s SQ %05zx(%05llx)/%05zx(%05llx) %08llx\n",
	       vnic->name,
	       ( ( vnic->sq.prod % TXNIC_SQES ) * TXNIC_SQ_STRIDE ),
	       readq ( vnic->regs + TXNIC_QS_SQ_TAIL(0) ),
	       ( ( vnic->sq.cons % TXNIC_SQES ) * TXNIC_SQ_STRIDE ),
	       readq ( vnic->regs + TXNIC_QS_SQ_HEAD(0) ),
	       readq ( vnic->regs + TXNIC_QS_SQ_STATUS(0) ) );
	DBGC ( vnic, "TXNIC %s RQ %05zx(%05llx)/%05zx(%05llx) %016llx\n",
	       vnic->name,
	       ( ( vnic->rq.prod % TXNIC_RQES ) * TXNIC_RQ_STRIDE ),
	       readq ( vnic->regs + TXNIC_QS_RBDR_TAIL(0) ),
	       ( ( vnic->rq.cons % TXNIC_RQES ) * TXNIC_RQ_STRIDE ),
	       readq ( vnic->regs + TXNIC_QS_RBDR_HEAD(0) ),
	       readq ( vnic->regs + TXNIC_QS_RBDR_STATUS0(0) ) );
	DBGC ( vnic, "TXNIC %s CQ xxxxx(%05llx)/%05x(%05llx) %08llx:%08llx\n",
	       vnic->name, readq ( vnic->regs + TXNIC_QS_CQ_TAIL(0) ),
	       ( ( vnic->cq.cons % TXNIC_CQES ) * TXNIC_CQ_STRIDE ),
	       readq ( vnic->regs + TXNIC_QS_CQ_HEAD(0) ),
	       readq ( vnic->regs + TXNIC_QS_CQ_STATUS(0) ),
	       readq ( vnic->regs + TXNIC_QS_CQ_STATUS2(0) ) );
}

/******************************************************************************
 *
 * Send queue
 *
 ******************************************************************************
 */

/**
 * Create send queue
 *
 * @v vnic		Virtual NIC
 * @ret rc		Return status code
 */
static int txnic_create_sq ( struct txnic *vnic ) {

	/* Reset send queue */
	vnic->sq.prod = 0;
	vnic->sq.cons = 0;
	writeq ( TXNIC_QS_SQ_CFG_RESET, ( vnic->regs + TXNIC_QS_SQ_CFG(0) ) );

	/* Configure and enable send queue */
	writeq ( user_to_phys ( vnic->sq.sqe, 0 ),
		 ( vnic->regs + TXNIC_QS_SQ_BASE(0) ) );
	writeq ( ( TXNIC_QS_SQ_CFG_ENA | TXNIC_QS_SQ_CFG_QSIZE_1K ),
		 ( vnic->regs + TXNIC_QS_SQ_CFG(0) ) );

	DBGC ( vnic, "TXNIC %s SQ at [%08lx,%08lx)\n",
	       vnic->name, user_to_phys ( vnic->sq.sqe, 0 ),
	       user_to_phys ( vnic->sq.sqe, TXNIC_SQ_SIZE ) );
	return 0;
}

/**
 * Disable send queue
 *
 * @v vnic		Virtual NIC
 * @ret rc		Return status code
 */
static int txnic_disable_sq ( struct txnic *vnic ) {
	uint64_t status;
	unsigned int i;

	/* Disable send queue */
	writeq ( 0, ( vnic->regs + TXNIC_QS_SQ_CFG(0) ) );

	/* Wait for send queue to be stopped */
	for ( i = 0 ; i < TXNIC_SQ_STOP_MAX_WAIT_MS ; i++ ) {

		/* Check if send queue is stopped */
		status = readq ( vnic->regs + TXNIC_QS_SQ_STATUS(0) );
		if ( status & TXNIC_QS_SQ_STATUS_STOPPED )
			return 0;

		/* Delay */
		mdelay ( 1 );
	}

	DBGC ( vnic, "TXNIC %s SQ disable timed out\n", vnic->name );
	return -ETIMEDOUT;
}

/**
 * Destroy send queue
 *
 * @v vnic		Virtual NIC
 */
static void txnic_destroy_sq ( struct txnic *vnic ) {
	int rc;

	/* Disable send queue */
	if ( ( rc = txnic_disable_sq ( vnic ) ) != 0 ) {
		/* Nothing else we can do */
		return;
	}

	/* Reset send queue */
	writeq ( TXNIC_QS_SQ_CFG_RESET, ( vnic->regs + TXNIC_QS_SQ_CFG(0) ) );
}

/**
 * Send packet
 *
 * @v vnic		Virtual NIC
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int txnic_send ( struct txnic *vnic, struct io_buffer *iobuf ) {
	struct txnic_sqe sqe;
	unsigned int sq_idx;
	size_t offset;
	size_t len;

	/* Get next send queue entry */
	if ( ( vnic->sq.prod - vnic->sq.cons ) >= TXNIC_SQ_FILL ) {
		DBGC ( vnic, "TXNIC %s out of send queue entries\n",
		       vnic->name );
		return -ENOBUFS;
	}
	sq_idx = ( vnic->sq.prod++ % TXNIC_SQES );
	offset = ( sq_idx * TXNIC_SQ_STRIDE );

	/* Populate send descriptor */
	len = iob_len ( iobuf );
	memset ( &sqe, 0, sizeof ( sqe ) );
	sqe.hdr.total = cpu_to_le32 ( ( len >= ETH_ZLEN ) ? len : ETH_ZLEN );
	sqe.hdr.subdcnt = ( TXNIC_SQE_SUBDESCS - 1 );
	sqe.hdr.flags = TXNIC_SEND_HDR_FLAGS;
	sqe.gather.size = cpu_to_le16 ( len );
	sqe.gather.flags = TXNIC_SEND_GATHER_FLAGS;
	sqe.gather.addr = cpu_to_le64 ( virt_to_bus ( iobuf->data ) );
	DBGC2 ( vnic, "TXNIC %s SQE %#03x is [%08lx,%08lx)\n",
		vnic->name, sq_idx, virt_to_bus ( iobuf->data ),
		( virt_to_bus ( iobuf->data ) + len ) );

	/* Copy send descriptor to ring */
	copy_to_user ( vnic->sq.sqe, offset, &sqe, sizeof ( sqe ) );

	/* Ring doorbell */
	wmb();
	writeq ( TXNIC_SQE_SUBDESCS, ( vnic->regs + TXNIC_QS_SQ_DOOR(0) ) );

	return 0;
}

/**
 * Complete send queue entry
 *
 * @v vnic		Virtual NIC
 * @v cqe		Send completion queue entry
 */
static void txnic_complete_sqe ( struct txnic *vnic,
				 struct txnic_cqe_send *cqe ) {
	struct net_device *netdev = vnic->netdev;
	unsigned int sq_idx;
	unsigned int status;

	/* Parse completion */
	sq_idx = ( le16_to_cpu ( cqe->sqe_ptr ) / TXNIC_SQE_SUBDESCS );
	status = cqe->send_status;

	/* Sanity check */
	assert ( sq_idx == ( vnic->sq.cons % TXNIC_SQES ) );

	/* Free send queue entry */
	vnic->sq.cons++;

	/* Complete transmission */
	if ( status ) {
		DBGC ( vnic, "TXNIC %s SQE %#03x complete (status %#02x)\n",
		       vnic->name, sq_idx, status );
		netdev_tx_complete_next_err ( netdev, -EIO );
	} else {
		DBGC2 ( vnic, "TXNIC %s SQE %#03x complete\n",
			vnic->name, sq_idx );
		netdev_tx_complete_next ( netdev );
	}
}

/******************************************************************************
 *
 * Receive queue
 *
 ******************************************************************************
 */

/**
 * Create receive queue
 *
 * @v vnic		Virtual NIC
 * @ret rc		Return status code
 */
static int txnic_create_rq ( struct txnic *vnic ) {

	/* Reset receive buffer descriptor ring */
	vnic->rq.prod = 0;
	vnic->rq.cons = 0;
	writeq ( TXNIC_QS_RBDR_CFG_RESET,
		 ( vnic->regs + TXNIC_QS_RBDR_CFG(0) ) );

	/* Configure and enable receive buffer descriptor ring */
	writeq ( user_to_phys ( vnic->rq.rqe, 0 ),
		 ( vnic->regs + TXNIC_QS_RBDR_BASE(0) ) );
	writeq ( ( TXNIC_QS_RBDR_CFG_ENA | TXNIC_QS_RBDR_CFG_QSIZE_8K |
		   TXNIC_QS_RBDR_CFG_LINES ( TXNIC_RQE_SIZE /
					     TXNIC_LINE_SIZE ) ),
		 ( vnic->regs + TXNIC_QS_RBDR_CFG(0) ) );

	/* Enable receive queue */
	writeq ( TXNIC_QS_RQ_CFG_ENA, ( vnic->regs + TXNIC_QS_RQ_CFG(0) ) );

	DBGC ( vnic, "TXNIC %s RQ at [%08lx,%08lx)\n",
	       vnic->name, user_to_phys ( vnic->rq.rqe, 0 ),
	       user_to_phys ( vnic->rq.rqe, TXNIC_RQ_SIZE ) );
	return 0;
}

/**
 * Disable receive queue
 *
 * @v vnic		Virtual NIC
 * @ret rc		Return status code
 */
static int txnic_disable_rq ( struct txnic *vnic ) {
	uint64_t cfg;
	unsigned int i;

	/* Disable receive queue */
	writeq ( 0, ( vnic->regs + TXNIC_QS_RQ_CFG(0) ) );

	/* Wait for receive queue to be disabled */
	for ( i = 0 ; i < TXNIC_RQ_DISABLE_MAX_WAIT_MS ; i++ ) {

		/* Check if receive queue is disabled */
		cfg = readq ( vnic->regs + TXNIC_QS_RQ_CFG(0) );
		if ( ! ( cfg & TXNIC_QS_RQ_CFG_ENA ) )
			return 0;

		/* Delay */
		mdelay ( 1 );
	}

	DBGC ( vnic, "TXNIC %s RQ disable timed out\n", vnic->name );
	return -ETIMEDOUT;
}

/**
 * Destroy receive queue
 *
 * @v vnic		Virtual NIC
 */
static void txnic_destroy_rq ( struct txnic *vnic ) {
	unsigned int i;
	int rc;

	/* Disable receive queue */
	if ( ( rc = txnic_disable_rq ( vnic ) ) != 0 ) {
		/* Leak memory; there's nothing else we can do */
		return;
	}

	/* Disable receive buffer descriptor ring */
	writeq ( 0, ( vnic->regs + TXNIC_QS_RBDR_CFG(0) ) );

	/* Reset receive buffer descriptor ring */
	writeq ( TXNIC_QS_RBDR_CFG_RESET,
		 ( vnic->regs + TXNIC_QS_RBDR_CFG(0) ) );

	/* Free any unused I/O buffers */
	for ( i = 0 ; i < TXNIC_RQ_FILL ; i++ ) {
		if ( vnic->rq.iobuf[i] )
			free_iob ( vnic->rq.iobuf[i] );
		vnic->rq.iobuf[i] = NULL;
	}
}

/**
 * Refill receive queue
 *
 * @v vnic		Virtual NIC
 */
static void txnic_refill_rq ( struct txnic *vnic ) {
	struct io_buffer *iobuf;
	struct txnic_rqe rqe;
	unsigned int rq_idx;
	unsigned int rq_iobuf_idx;
	unsigned int refilled = 0;
	size_t offset;

	/* Refill ring */
	while ( ( vnic->rq.prod - vnic->rq.cons ) < TXNIC_RQ_FILL ) {

		/* Allocate I/O buffer */
		iobuf = alloc_iob ( TXNIC_RQE_SIZE );
		if ( ! iobuf ) {
			/* Wait for next refill */
			break;
		}

		/* Get next receive descriptor */
		rq_idx = ( vnic->rq.prod++ % TXNIC_RQES );
		offset = ( rq_idx * TXNIC_RQ_STRIDE );

		/* Populate receive descriptor */
		rqe.rbdre.addr = cpu_to_le64 ( virt_to_bus ( iobuf->data ) );
		DBGC2 ( vnic, "TXNIC %s RQE %#03x is [%08lx,%08lx)\n",
			vnic->name, rq_idx, virt_to_bus ( iobuf->data ),
			( virt_to_bus ( iobuf->data ) + TXNIC_RQE_SIZE ) );

		/* Copy receive descriptor to ring */
		copy_to_user ( vnic->rq.rqe, offset, &rqe, sizeof ( rqe ) );
		refilled++;

		/* Record I/O buffer */
		rq_iobuf_idx = ( rq_idx % TXNIC_RQ_FILL );
		assert ( vnic->rq.iobuf[rq_iobuf_idx] == NULL );
		vnic->rq.iobuf[rq_iobuf_idx] = iobuf;
	}

	/* Ring doorbell */
	wmb();
	writeq ( refilled, ( vnic->regs + TXNIC_QS_RBDR_DOOR(0) ) );
}

/**
 * Complete receive queue entry
 *
 * @v vnic		Virtual NIC
 * @v cqe		Receive completion queue entry
 */
static void txnic_complete_rqe ( struct txnic *vnic,
				 struct txnic_cqe_rx *cqe ) {
	struct net_device *netdev = vnic->netdev;
	struct io_buffer *iobuf;
	unsigned int errop;
	unsigned int rq_idx;
	unsigned int rq_iobuf_idx;
	size_t apad_len;
	size_t len;

	/* Parse completion */
	errop = cqe->errop;
	apad_len = TXNIC_CQE_RX_APAD_LEN ( cqe->apad );
	len = le16_to_cpu ( cqe->len );

	/* Get next receive I/O buffer */
	rq_idx = ( vnic->rq.cons++ % TXNIC_RQES );
	rq_iobuf_idx = ( rq_idx % TXNIC_RQ_FILL );
	iobuf = vnic->rq.iobuf[rq_iobuf_idx];
	vnic->rq.iobuf[rq_iobuf_idx] = NULL;

	/* Populate I/O buffer */
	iob_reserve ( iobuf, apad_len );
	iob_put ( iobuf, len );

	/* Hand off to network stack */
	if ( errop ) {
		DBGC ( vnic, "TXNIC %s RQE %#03x error (length %zd, errop "
		       "%#02x)\n", vnic->name, rq_idx, len, errop );
		netdev_rx_err ( netdev, iobuf, -EIO );
	} else {
		DBGC2 ( vnic, "TXNIC %s RQE %#03x complete (length %zd)\n",
			vnic->name, rq_idx, len );
		netdev_rx ( netdev, iobuf );
	}
}

/******************************************************************************
 *
 * Completion queue
 *
 ******************************************************************************
 */

/**
 * Create completion queue
 *
 * @v vnic		Virtual NIC
 * @ret rc		Return status code
 */
static int txnic_create_cq ( struct txnic *vnic ) {

	/* Reset completion queue */
	vnic->cq.cons = 0;
	writeq ( TXNIC_QS_CQ_CFG_RESET, ( vnic->regs + TXNIC_QS_CQ_CFG(0) ) );

	/* Configure and enable completion queue */
	writeq ( user_to_phys ( vnic->cq.cqe, 0 ),
		 ( vnic->regs + TXNIC_QS_CQ_BASE(0) ) );
	writeq ( ( TXNIC_QS_CQ_CFG_ENA | TXNIC_QS_CQ_CFG_QSIZE_256 ),
		 ( vnic->regs + TXNIC_QS_CQ_CFG(0) ) );

	DBGC ( vnic, "TXNIC %s CQ at [%08lx,%08lx)\n",
	       vnic->name, user_to_phys ( vnic->cq.cqe, 0 ),
	       user_to_phys ( vnic->cq.cqe, TXNIC_CQ_SIZE ) );
	return 0;
}

/**
 * Disable completion queue
 *
 * @v vnic		Virtual NIC
 * @ret rc		Return status code
 */
static int txnic_disable_cq ( struct txnic *vnic ) {
	uint64_t cfg;
	unsigned int i;

	/* Disable completion queue */
	writeq ( 0, ( vnic->regs + TXNIC_QS_CQ_CFG(0) ) );

	/* Wait for completion queue to be disabled */
	for ( i = 0 ; i < TXNIC_CQ_DISABLE_MAX_WAIT_MS ; i++ ) {

		/* Check if completion queue is disabled */
		cfg = readq ( vnic->regs + TXNIC_QS_CQ_CFG(0) );
		if ( ! ( cfg & TXNIC_QS_CQ_CFG_ENA ) )
			return 0;

		/* Delay */
		mdelay ( 1 );
	}

	DBGC ( vnic, "TXNIC %s CQ disable timed out\n", vnic->name );
	return -ETIMEDOUT;
}

/**
 * Destroy completion queue
 *
 * @v vnic		Virtual NIC
 */
static void txnic_destroy_cq ( struct txnic *vnic ) {
	int rc;

	/* Disable completion queue */
	if ( ( rc = txnic_disable_cq ( vnic ) ) != 0 ) {
		/* Leak memory; there's nothing else we can do */
		return;
	}

	/* Reset completion queue */
	writeq ( TXNIC_QS_CQ_CFG_RESET, ( vnic->regs + TXNIC_QS_CQ_CFG(0) ) );
}

/**
 * Poll completion queue
 *
 * @v vnic		Virtual NIC
 */
static void txnic_poll_cq ( struct txnic *vnic ) {
	union txnic_cqe cqe;
	uint64_t status;
	size_t offset;
	unsigned int qcount;
	unsigned int cq_idx;
	unsigned int i;

	/* Get number of completions */
	status = readq ( vnic->regs + TXNIC_QS_CQ_STATUS(0) );
	qcount = TXNIC_QS_CQ_STATUS_QCOUNT ( status );
	if ( ! qcount )
		return;

	/* Process completion queue entries */
	for ( i = 0 ; i < qcount ; i++ ) {

		/* Get completion queue entry */
		cq_idx = ( vnic->cq.cons++ % TXNIC_CQES );
		offset = ( cq_idx * TXNIC_CQ_STRIDE );
		copy_from_user ( &cqe, vnic->cq.cqe, offset, sizeof ( cqe ) );

		/* Process completion queue entry */
		switch ( cqe.common.cqe_type ) {
		case TXNIC_CQE_TYPE_SEND:
			txnic_complete_sqe ( vnic, &cqe.send );
			break;
		case TXNIC_CQE_TYPE_RX:
			txnic_complete_rqe ( vnic, &cqe.rx );
			break;
		default:
			DBGC ( vnic, "TXNIC %s unknown completion type %d\n",
			       vnic->name, cqe.common.cqe_type );
			DBGC_HDA ( vnic, user_to_phys ( vnic->cq.cqe, offset ),
				   &cqe, sizeof ( cqe ) );
			break;
		}
	}

	/* Ring doorbell */
	writeq ( qcount, ( vnic->regs + TXNIC_QS_CQ_DOOR(0) ) );
}

/******************************************************************************
 *
 * Virtual NIC
 *
 ******************************************************************************
 */

/**
 * Open virtual NIC
 *
 * @v vnic		Virtual NIC
 * @ret rc		Return status code
 */
static int txnic_open ( struct txnic *vnic ) {
	int rc;

	/* Create completion queue */
	if ( ( rc = txnic_create_cq ( vnic ) ) != 0 )
		goto err_create_cq;

	/* Create send queue */
	if ( ( rc = txnic_create_sq ( vnic ) ) != 0 )
		goto err_create_sq;

	/* Create receive queue */
	if ( ( rc = txnic_create_rq ( vnic ) ) != 0 )
		goto err_create_rq;

	/* Refill receive queue */
	txnic_refill_rq ( vnic );

	return 0;

	txnic_destroy_rq ( vnic );
 err_create_rq:
	txnic_destroy_sq ( vnic );
 err_create_sq:
	txnic_destroy_cq ( vnic );
 err_create_cq:
	return rc;
}

/**
 * Close virtual NIC
 *
 * @v vnic		Virtual NIC
 */
static void txnic_close ( struct txnic *vnic ) {

	/* Destroy receive queue */
	txnic_destroy_rq ( vnic );

	/* Destroy send queue */
	txnic_destroy_sq ( vnic );

	/* Destroy completion queue */
	txnic_destroy_cq ( vnic );
}

/**
 * Poll virtual NIC
 *
 * @v vnic		Virtual NIC
 */
static void txnic_poll ( struct txnic *vnic ) {

	/* Poll completion queue */
	txnic_poll_cq ( vnic );

	/* Refill receive queue */
	txnic_refill_rq ( vnic );
}

/**
 * Allocate virtual NIC
 *
 * @v dev		Underlying device
 * @v membase		Register base address
 * @ret vnic		Virtual NIC, or NULL on failure
 */
static struct txnic * txnic_alloc ( struct device *dev,
				    unsigned long membase ) {
	struct net_device *netdev;
	struct txnic *vnic;

	/* Allocate network device */
	netdev = alloc_etherdev ( sizeof ( *vnic ) );
	if ( ! netdev )
		goto err_alloc_netdev;
	netdev->dev = dev;
	vnic = netdev->priv;
	vnic->netdev = netdev;
	vnic->name = dev->name;

	/* Allow caller to reuse netdev->priv.  (The generic virtual
	 * NIC code never assumes that netdev->priv==vnic.)
	 */
	netdev->priv = NULL;

	/* Allocate completion queue */
	vnic->cq.cqe = umalloc ( TXNIC_CQ_SIZE );
	if ( ! vnic->cq.cqe )
		goto err_alloc_cq;

	/* Allocate send queue */
	vnic->sq.sqe = umalloc ( TXNIC_SQ_SIZE );
	if ( ! vnic->sq.sqe )
		goto err_alloc_sq;

	/* Allocate receive queue */
	vnic->rq.rqe = umalloc ( TXNIC_RQ_SIZE );
	if ( ! vnic->rq.rqe )
		goto err_alloc_rq;

	/* Map registers */
	vnic->regs = ioremap ( membase, TXNIC_VF_BAR_SIZE );
	if ( ! vnic->regs )
		goto err_ioremap;

	return vnic;

	iounmap ( vnic->regs );
 err_ioremap:
	ufree ( vnic->rq.rqe );
 err_alloc_rq:
	ufree ( vnic->sq.sqe );
 err_alloc_sq:
	ufree ( vnic->cq.cqe );
 err_alloc_cq:
	netdev_nullify ( netdev );
	netdev_put ( netdev );
 err_alloc_netdev:
	return NULL;
}

/**
 * Free virtual NIC
 *
 * @v vnic		Virtual NIC
 */
static void txnic_free ( struct txnic *vnic ) {
	struct net_device *netdev = vnic->netdev;

	/* Unmap registers */
	iounmap ( vnic->regs );

	/* Free receive queue */
	ufree ( vnic->rq.rqe );

	/* Free send queue */
	ufree ( vnic->sq.sqe );

	/* Free completion queue */
	ufree ( vnic->cq.cqe );

	/* Free network device */
	netdev_nullify ( netdev );
	netdev_put ( netdev );
}

/******************************************************************************
 *
 * Logical MAC virtual NICs
 *
 ******************************************************************************
 */

/**
 * Show LMAC diagnostics (for debugging)
 *
 * @v lmac		Logical MAC
 */
static __attribute__ (( unused )) void
txnic_lmac_diag ( struct txnic_lmac *lmac ) {
	struct txnic *vnic = lmac->vnic;
	uint64_t status1;
	uint64_t status2;
	uint64_t br_status1;
	uint64_t br_status2;
	uint64_t br_algn_status;
	uint64_t br_pmd_status;
	uint64_t an_status;

	/* Read status (clearing latching bits) */
	writeq ( BGX_SPU_STATUS1_RCV_LNK, ( lmac->regs + BGX_SPU_STATUS1 ) );
	writeq ( BGX_SPU_STATUS2_RCVFLT, ( lmac->regs + BGX_SPU_STATUS2 ) );
	status1 = readq ( lmac->regs + BGX_SPU_STATUS1 );
	status2 = readq ( lmac->regs + BGX_SPU_STATUS2 );
	DBGC ( vnic, "TXNIC %s SPU %02llx:%04llx%s%s%s\n",
	       vnic->name, status1, status2,
	       ( ( status1 & BGX_SPU_STATUS1_FLT ) ? " FLT" : "" ),
	       ( ( status1 & BGX_SPU_STATUS1_RCV_LNK ) ? " RCV_LNK" : "" ),
	       ( ( status2 & BGX_SPU_STATUS2_RCVFLT ) ? " RCVFLT" : "" ) );

	/* Read BASE-R status (clearing latching bits) */
	writeq ( ( BGX_SPU_BR_STATUS2_LATCHED_LOCK |
		   BGX_SPU_BR_STATUS2_LATCHED_BER ),
		 ( lmac->regs + BGX_SPU_BR_STATUS2 ) );
	br_status1 = readq ( lmac->regs + BGX_SPU_BR_STATUS1 );
	br_status2 = readq ( lmac->regs + BGX_SPU_BR_STATUS2 );
	DBGC ( vnic, "TXNIC %s BR %04llx:%04llx%s%s%s%s%s\n",
	       vnic->name, br_status2, br_status2,
	       ( ( br_status1 & BGX_SPU_BR_STATUS1_RCV_LNK ) ? " RCV_LNK" : ""),
	       ( ( br_status1 & BGX_SPU_BR_STATUS1_HI_BER ) ? " HI_BER" : "" ),
	       ( ( br_status1 & BGX_SPU_BR_STATUS1_BLK_LOCK ) ?
		 " BLK_LOCK" : "" ),
	       ( ( br_status2 & BGX_SPU_BR_STATUS2_LATCHED_LOCK ) ?
		 " LATCHED_LOCK" : "" ),
	       ( ( br_status2 & BGX_SPU_BR_STATUS2_LATCHED_BER ) ?
		 " LATCHED_BER" : "" ) );

	/* Read BASE-R alignment status */
	br_algn_status = readq ( lmac->regs + BGX_SPU_BR_ALGN_STATUS );
	DBGC ( vnic, "TXNIC %s BR ALGN %016llx%s\n", vnic->name, br_algn_status,
	       ( ( br_algn_status & BGX_SPU_BR_ALGN_STATUS_ALIGND ) ?
		 " ALIGND" : "" ) );

	/* Read BASE-R link training status */
	br_pmd_status = readq ( lmac->regs + BGX_SPU_BR_PMD_STATUS );
	DBGC ( vnic, "TXNIC %s BR PMD %04llx\n", vnic->name, br_pmd_status );

	/* Read autonegotiation status (clearing latching bits) */
	writeq ( ( BGX_SPU_AN_STATUS_PAGE_RX | BGX_SPU_AN_STATUS_LINK_STATUS ),
		 ( lmac->regs + BGX_SPU_AN_STATUS ) );
	an_status = readq ( lmac->regs + BGX_SPU_AN_STATUS );
	DBGC ( vnic, "TXNIC %s BR AN %04llx%s%s%s%s%s\n", vnic->name, an_status,
	       ( ( an_status & BGX_SPU_AN_STATUS_XNP_STAT ) ? " XNP_STAT" : ""),
	       ( ( an_status & BGX_SPU_AN_STATUS_PAGE_RX ) ? " PAGE_RX" : "" ),
	       ( ( an_status & BGX_SPU_AN_STATUS_AN_COMPLETE ) ?
		 " AN_COMPLETE" : "" ),
	       ( ( an_status & BGX_SPU_AN_STATUS_LINK_STATUS ) ?
		 " LINK_STATUS" : "" ),
	       ( ( an_status & BGX_SPU_AN_STATUS_LP_AN_ABLE ) ?
		 " LP_AN_ABLE" : "" ) );

	/* Read transmit statistics */
	DBGC ( vnic, "TXNIC %s TXF xc %#llx xd %#llx mc %#llx sc %#llx ok "
	       "%#llx bc %#llx mc %#llx un %#llx pa %#llx\n", vnic->name,
	       readq ( lmac->regs + BGX_CMR_TX_STAT0 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT1 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT2 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT3 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT5 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT14 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT15 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT16 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT17 ) );
	DBGC ( vnic, "TXNIC %s TXB ok %#llx hist %#llx:%#llx:%#llx:%#llx:"
	       "%#llx:%#llx:%#llx:%#llx\n", vnic->name,
	       readq ( lmac->regs + BGX_CMR_TX_STAT4 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT6 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT7 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT8 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT9 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT10 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT11 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT12 ),
	       readq ( lmac->regs + BGX_CMR_TX_STAT13 ) );

	/* Read receive statistics */
	DBGC ( vnic, "TXNIC %s RXF ok %#llx pa %#llx nm %#llx ov %#llx er "
	       "%#llx nc %#llx\n", vnic->name,
	       readq ( lmac->regs + BGX_CMR_RX_STAT0 ),
	       readq ( lmac->regs + BGX_CMR_RX_STAT2 ),
	       readq ( lmac->regs + BGX_CMR_RX_STAT4 ),
	       readq ( lmac->regs + BGX_CMR_RX_STAT6 ),
	       readq ( lmac->regs + BGX_CMR_RX_STAT8 ),
	       readq ( lmac->regs + BGX_CMR_RX_STAT9 ) );
	DBGC ( vnic, "TXNIC %s RXB ok %#llx pa %#llx nm %#llx ov %#llx nc "
	       "%#llx\n", vnic->name,
	       readq ( lmac->regs + BGX_CMR_RX_STAT1 ),
	       readq ( lmac->regs + BGX_CMR_RX_STAT3 ),
	       readq ( lmac->regs + BGX_CMR_RX_STAT5 ),
	       readq ( lmac->regs + BGX_CMR_RX_STAT7 ),
	       readq ( lmac->regs + BGX_CMR_RX_STAT10 ) );
}

/**
 * Update LMAC link state
 *
 * @v lmac		Logical MAC
 */
static void txnic_lmac_update_link ( struct txnic_lmac *lmac ) {
	struct txnic *vnic = lmac->vnic;
	struct net_device *netdev = vnic->netdev;
	uint64_t status1;

	/* Read status (clearing latching bits) */
	writeq ( BGX_SPU_STATUS1_RCV_LNK, ( lmac->regs + BGX_SPU_STATUS1 ) );
	status1 = readq ( lmac->regs + BGX_SPU_STATUS1 );

	/* Report link status */
	if ( status1 & BGX_SPU_STATUS1_RCV_LNK ) {
		netdev_link_up ( netdev );
	} else {
		netdev_link_down ( netdev );
	}
}

/**
 * Poll LMAC link state
 *
 * @v lmac		Logical MAC
 */
static void txnic_lmac_poll_link ( struct txnic_lmac *lmac ) {
	struct txnic *vnic = lmac->vnic;
	uint64_t intr;

	/* Get interrupt status */
	intr = readq ( lmac->regs + BGX_SPU_INT );
	if ( ! intr )
		return;
	DBGC ( vnic, "TXNIC %s INT %04llx%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
	       vnic->name, intr,
	       ( ( intr & BGX_SPU_INT_TRAINING_FAIL ) ? " TRAINING_FAIL" : "" ),
	       ( ( intr & BGX_SPU_INT_TRAINING_DONE ) ? " TRAINING_DONE" : "" ),
	       ( ( intr & BGX_SPU_INT_AN_COMPLETE ) ? " AN_COMPLETE" : "" ),
	       ( ( intr & BGX_SPU_INT_AN_LINK_GOOD ) ? " AN_LINK_GOOD" : "" ),
	       ( ( intr & BGX_SPU_INT_AN_PAGE_RX ) ? " AN_PAGE_RX" : "" ),
	       ( ( intr & BGX_SPU_INT_FEC_UNCORR ) ? " FEC_UNCORR" : "" ),
	       ( ( intr & BGX_SPU_INT_FEC_CORR ) ? " FEC_CORR" : "" ),
	       ( ( intr & BGX_SPU_INT_BIP_ERR ) ? " BIP_ERR" : "" ),
	       ( ( intr & BGX_SPU_INT_DBG_SYNC ) ? " DBG_SYNC" : "" ),
	       ( ( intr & BGX_SPU_INT_ALGNLOS ) ? " ALGNLOS" : "" ),
	       ( ( intr & BGX_SPU_INT_SYNLOS ) ? " SYNLOS" : "" ),
	       ( ( intr & BGX_SPU_INT_BITLCKLS ) ? " BITLCKLS" : "" ),
	       ( ( intr & BGX_SPU_INT_ERR_BLK ) ? " ERR_BLK" : "" ),
	       ( ( intr & BGX_SPU_INT_RX_LINK_DOWN ) ? " RX_LINK_DOWN" : "" ),
	       ( ( intr & BGX_SPU_INT_RX_LINK_UP ) ? " RX_LINK_UP" : "" ) );

	/* Clear interrupt status */
	writeq ( intr, ( lmac->regs + BGX_SPU_INT ) );

	/* Update link state */
	txnic_lmac_update_link ( lmac );
}

/**
 * Reset LMAC
 *
 * @v lmac		Logical MAC
 */
static void txnic_lmac_reset ( struct txnic_lmac *lmac ) {
	struct txnic_bgx *bgx = lmac->bgx;
	struct txnic_pf *pf = bgx->pf;
	void *qsregs = ( pf->regs + TXNIC_PF_QS ( lmac->idx ) );

	/* There is no reset available for the physical function
	 * aspects of a virtual NIC; we have to explicitly reload a
	 * sensible set of default values.
	 */
	writeq ( 0, ( qsregs + TXNIC_PF_QS_CFG ) );
	writeq ( 0, ( qsregs + TXNIC_PF_QS_RQ_CFG(0) ) );
	writeq ( 0, ( qsregs + TXNIC_PF_QS_RQ_DROP_CFG(0) ) );
	writeq ( 0, ( qsregs + TXNIC_PF_QS_RQ_BP_CFG(0) ) );
	writeq ( 0, ( qsregs + TXNIC_PF_QS_SQ_CFG(0) ) );
}

/**
 * Open network device
 *
 * @v netdev		Network device
 * @ret rc		Return status code
 */
static int txnic_lmac_open ( struct net_device *netdev ) {
	struct txnic_lmac *lmac = netdev->priv;
	struct txnic_bgx *bgx = lmac->bgx;
	struct txnic_pf *pf = bgx->pf;
	struct txnic *vnic = lmac->vnic;
	unsigned int vnic_idx = lmac->idx;
	unsigned int chan_idx = TXNIC_CHAN_IDX ( vnic_idx );
	unsigned int tl4_idx = TXNIC_TL4_IDX ( vnic_idx );
	unsigned int tl3_idx = TXNIC_TL3_IDX ( vnic_idx );
	unsigned int tl2_idx = TXNIC_TL2_IDX ( vnic_idx );
	void *lmregs = ( pf->regs + TXNIC_PF_LMAC ( vnic_idx ) );
	void *chregs = ( pf->regs + TXNIC_PF_CHAN ( chan_idx ) );
	void *qsregs = ( pf->regs + TXNIC_PF_QS ( vnic_idx ) );
	size_t max_pkt_size;
	int rc;

	/* Configure channel/match parse indices */
	writeq ( ( TXNIC_PF_MPI_CFG_VNIC ( vnic_idx ) |
		   TXNIC_PF_MPI_CFG_RSSI_BASE ( vnic_idx ) ),
		 ( TXNIC_PF_MPI_CFG ( vnic_idx ) + pf->regs ) );
	writeq ( ( TXNIC_PF_RSSI_RQ_RQ_QS ( vnic_idx ) ),
		 ( TXNIC_PF_RSSI_RQ ( vnic_idx ) + pf->regs ) );

	/* Configure LMAC */
	max_pkt_size = ( netdev->max_pkt_len + 4 /* possible VLAN */ );
	writeq ( ( TXNIC_PF_LMAC_CFG_ADJUST_DEFAULT |
		   TXNIC_PF_LMAC_CFG_MIN_PKT_SIZE ( ETH_ZLEN ) ),
		 ( TXNIC_PF_LMAC_CFG + lmregs ) );
	writeq ( ( TXNIC_PF_LMAC_CFG2_MAX_PKT_SIZE ( max_pkt_size ) ),
		 ( TXNIC_PF_LMAC_CFG2 + lmregs ) );
	writeq ( ( TXNIC_PF_LMAC_CREDIT_CC_UNIT_CNT_DEFAULT |
		   TXNIC_PF_LMAC_CREDIT_CC_PACKET_CNT_DEFAULT |
		   TXNIC_PF_LMAC_CREDIT_CC_ENABLE ),
		 ( TXNIC_PF_LMAC_CREDIT + lmregs ) );

	/* Configure channels */
	writeq ( ( TXNIC_PF_CHAN_TX_CFG_BP_ENA ),
		 ( TXNIC_PF_CHAN_TX_CFG + chregs ) );
	writeq ( ( TXNIC_PF_CHAN_RX_CFG_CPI_BASE ( vnic_idx ) ),
		 ( TXNIC_PF_CHAN_RX_CFG + chregs ) );
	writeq ( ( TXNIC_PF_CHAN_RX_BP_CFG_ENA |
		   TXNIC_PF_CHAN_RX_BP_CFG_BPID ( vnic_idx ) ),
		 ( TXNIC_PF_CHAN_RX_BP_CFG + chregs ) );

	/* Configure traffic limiters */
	writeq ( ( TXNIC_PF_TL2_CFG_RR_QUANTUM_DEFAULT ),
		 ( TXNIC_PF_TL2_CFG ( tl2_idx ) + pf->regs ) );
	writeq ( ( TXNIC_PF_TL3_CFG_RR_QUANTUM_DEFAULT ),
		 ( TXNIC_PF_TL3_CFG ( tl3_idx ) + pf->regs ) );
	writeq ( ( TXNIC_PF_TL3_CHAN_CHAN ( chan_idx ) ),
		 ( TXNIC_PF_TL3_CHAN ( tl3_idx ) + pf->regs ) );
	writeq ( ( TXNIC_PF_TL4_CFG_SQ_QS ( vnic_idx ) |
		   TXNIC_PF_TL4_CFG_RR_QUANTUM_DEFAULT ),
		 ( TXNIC_PF_TL4_CFG ( tl4_idx ) + pf->regs ) );

	/* Configure send queue */
	writeq ( ( TXNIC_PF_QS_SQ_CFG_CQ_QS ( vnic_idx ) ),
		 ( TXNIC_PF_QS_SQ_CFG(0) + qsregs ) );
	writeq ( ( TXNIC_PF_QS_SQ_CFG2_TL4 ( tl4_idx ) ),
		 ( TXNIC_PF_QS_SQ_CFG2(0) + qsregs ) );

	/* Configure receive queue */
	writeq ( ( TXNIC_PF_QS_RQ_CFG_CACHING_ALL |
		   TXNIC_PF_QS_RQ_CFG_CQ_QS ( vnic_idx ) |
		   TXNIC_PF_QS_RQ_CFG_RBDR_CONT_QS ( vnic_idx ) |
		   TXNIC_PF_QS_RQ_CFG_RBDR_STRT_QS ( vnic_idx ) ),
		 ( TXNIC_PF_QS_RQ_CFG(0) + qsregs ) );
	writeq ( ( TXNIC_PF_QS_RQ_BP_CFG_RBDR_BP_ENA |
		   TXNIC_PF_QS_RQ_BP_CFG_CQ_BP_ENA |
		   TXNIC_PF_QS_RQ_BP_CFG_BPID ( vnic_idx ) ),
		 ( TXNIC_PF_QS_RQ_BP_CFG(0) + qsregs ) );

	/* Enable queue set */
	writeq ( ( TXNIC_PF_QS_CFG_ENA | TXNIC_PF_QS_CFG_VNIC ( vnic_idx ) ),
		 ( TXNIC_PF_QS_CFG + qsregs ) );

	/* Open virtual NIC */
	if ( ( rc = txnic_open ( vnic ) ) != 0 )
		goto err_open;

	/* Update link state */
	txnic_lmac_update_link ( lmac );

	return 0;

	txnic_close ( vnic );
 err_open:
	writeq ( 0, ( qsregs + TXNIC_PF_QS_CFG ) );
	return rc;
}

/**
 * Close network device
 *
 * @v netdev		Network device
 */
static void txnic_lmac_close ( struct net_device *netdev ) {
	struct txnic_lmac *lmac = netdev->priv;
	struct txnic_bgx *bgx = lmac->bgx;
	struct txnic_pf *pf = bgx->pf;
	struct txnic *vnic = lmac->vnic;
	void *qsregs = ( pf->regs + TXNIC_PF_QS ( lmac->idx ) );

	/* Close virtual NIC */
	txnic_close ( vnic );

	/* Disable queue set */
	writeq ( 0, ( qsregs + TXNIC_PF_QS_CFG ) );
}

/**
 * Transmit packet
 *
 * @v netdev		Network device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int txnic_lmac_transmit ( struct net_device *netdev,
				 struct io_buffer *iobuf ) {
	struct txnic_lmac *lmac = netdev->priv;
	struct txnic *vnic = lmac->vnic;

	return txnic_send ( vnic, iobuf );
}

/**
 * Poll network device
 *
 * @v netdev		Network device
 */
static void txnic_lmac_poll ( struct net_device *netdev ) {
	struct txnic_lmac *lmac = netdev->priv;
	struct txnic *vnic = lmac->vnic;

	/* Poll virtual NIC */
	txnic_poll ( vnic );

	/* Poll link state */
	txnic_lmac_poll_link ( lmac );
}

/** Network device operations */
static struct net_device_operations txnic_lmac_operations = {
	.open = txnic_lmac_open,
	.close = txnic_lmac_close,
	.transmit = txnic_lmac_transmit,
	.poll = txnic_lmac_poll,
};

/**
 * Probe logical MAC virtual NIC
 *
 * @v lmac		Logical MAC
 * @ret rc		Return status code
 */
static int txnic_lmac_probe ( struct txnic_lmac *lmac ) {
	struct txnic_bgx *bgx = lmac->bgx;
	struct txnic_pf *pf = bgx->pf;
	struct txnic *vnic;
	struct net_device *netdev;
	unsigned long membase;
	int rc;

	/* Sanity check */
	assert ( lmac->vnic == NULL );

	/* Calculate register base address */
	membase = ( pf->vf_membase + ( lmac->idx * pf->vf_stride ) );

	/* Allocate and initialise network device */
	vnic = txnic_alloc ( &bgx->pci->dev, membase );
	if ( ! vnic ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	netdev = vnic->netdev;
	netdev_init ( netdev, &txnic_lmac_operations );
	netdev->priv = lmac;
	lmac->vnic = vnic;

	/* Reset device */
	txnic_lmac_reset ( lmac );

	/* Set MAC address */
	memcpy ( netdev->hw_addr, lmac->mac.raw, ETH_ALEN );

	/* Register network device */
	if ( ( rc = register_netdev ( netdev ) ) != 0 )
		goto err_register;
	vnic->name = netdev->name;
	DBGC ( TXNICCOL ( pf ), "TXNIC %d/%d/%d is %s (%s)\n", pf->node,
	       bgx->idx, lmac->idx, vnic->name, eth_ntoa ( lmac->mac.raw ) );

	/* Update link state */
	txnic_lmac_update_link ( lmac );

	return 0;

	unregister_netdev ( netdev );
 err_register:
	txnic_lmac_reset ( lmac );
	txnic_free ( vnic );
	lmac->vnic = NULL;
 err_alloc:
	return rc;
}

/**
 * Remove logical MAC virtual NIC
 *
 * @v lmac		Logical MAC
 */
static void txnic_lmac_remove ( struct txnic_lmac *lmac ) {

	/* Sanity check */
	assert ( lmac->vnic != NULL );

	/* Unregister network device */
	unregister_netdev ( lmac->vnic->netdev );

	/* Reset device */
	txnic_lmac_reset ( lmac );

	/* Free virtual NIC */
	txnic_free ( lmac->vnic );
	lmac->vnic = NULL;
}

/**
 * Probe all LMACs on a BGX Ethernet interface
 *
 * @v pf		Physical function
 * @v bgx		BGX Ethernet interface
 * @ret rc		Return status code
 */
static int txnic_lmac_probe_all ( struct txnic_pf *pf, struct txnic_bgx *bgx ) {
	unsigned int bgx_idx;
	int lmac_idx;
	int count;
	int rc;

	/* Sanity checks */
	bgx_idx = bgx->idx;
	assert ( pf->node == bgx->node );
	assert ( pf->bgx[bgx_idx] == NULL );
	assert ( bgx->pf == NULL );

	/* Associate BGX with physical function */
	pf->bgx[bgx_idx] = bgx;
	bgx->pf = pf;

	/* Probe all LMACs */
	count = bgx->count;
	for ( lmac_idx = 0 ; lmac_idx < count ; lmac_idx++ ) {
		if ( ( rc = txnic_lmac_probe ( &bgx->lmac[lmac_idx] ) ) != 0 )
			goto err_probe;
	}

	return 0;

	lmac_idx = count;
 err_probe:
	for ( lmac_idx-- ; lmac_idx >= 0 ; lmac_idx-- )
		txnic_lmac_remove ( &bgx->lmac[lmac_idx] );
	pf->bgx[bgx_idx] = NULL;
	bgx->pf = NULL;
	return rc;
}

/**
 * Remove all LMACs on a BGX Ethernet interface
 *
 * @v pf		Physical function
 * @v bgx		BGX Ethernet interface
 */
static void txnic_lmac_remove_all ( struct txnic_pf *pf,
				    struct txnic_bgx *bgx ) {
	unsigned int lmac_idx;

	/* Sanity checks */
	assert ( pf->bgx[bgx->idx] == bgx );
	assert ( bgx->pf == pf );

	/* Remove all LMACs */
	for ( lmac_idx = 0 ; lmac_idx < bgx->count ; lmac_idx++ )
		txnic_lmac_remove ( &bgx->lmac[lmac_idx] );

	/* Disassociate BGX from physical function */
	pf->bgx[bgx->idx] = NULL;
	bgx->pf = NULL;
}

/******************************************************************************
 *
 * NIC physical function interface
 *
 ******************************************************************************
 */

/**
 * Probe PCI device
 *
 * @v pci		PCI device
 * @ret rc		Return status code
 */
static int txnic_pf_probe ( struct pci_device *pci ) {
	struct txnic_pf *pf;
	struct txnic_bgx *bgx;
	unsigned long membase;
	unsigned int i;
	int rc;

	/* Allocate and initialise structure */
	pf = zalloc ( sizeof ( *pf ) );
	if ( ! pf ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	pf->pci = pci;
	pci_set_drvdata ( pci, pf );

	/* Get base addresses */
	membase = pciea_bar_start ( pci, PCIEA_BEI_BAR_0 );
	pf->vf_membase = pciea_bar_start ( pci, PCIEA_BEI_VF_BAR_0 );
	pf->vf_stride = pciea_bar_size ( pci, PCIEA_BEI_VF_BAR_0 );

	/* Calculate node ID */
	pf->node = txnic_address_node ( membase );
	DBGC ( TXNICCOL ( pf ), "TXNIC %d/*/* PF %s at %#lx (VF %#lx+%#lx)\n",
	       pf->node, pci->dev.name, membase, pf->vf_membase, pf->vf_stride);

	/* Fix up PCI device */
	adjust_pci_device ( pci );

	/* Map registers */
	pf->regs = ioremap ( membase, TXNIC_PF_BAR_SIZE );
	if ( ! pf->regs ) {
		rc = -ENODEV;
		goto err_ioremap;
	}

	/* Configure physical function */
	writeq ( TXNIC_PF_CFG_ENA, ( pf->regs + TXNIC_PF_CFG ) );
	writeq ( ( TXNIC_PF_BP_CFG_BP_POLL_ENA |
		   TXNIC_PF_BP_CFG_BP_POLL_DLY_DEFAULT ),
		 ( pf->regs + TXNIC_PF_BP_CFG ) );
	for ( i = 0 ; i < TXNIC_NUM_BGX ; i++ ) {
		writeq ( ( TXNIC_PF_INTF_SEND_CFG_BLOCK_BGX |
			   TXNIC_PF_INTF_SEND_CFG_BLOCK ( i ) ),
			 ( pf->regs + TXNIC_PF_INTF_SEND_CFG ( i ) ) );
		writeq ( ( TXNIC_PF_INTF_BP_CFG_BP_ENA |
			   TXNIC_PF_INTF_BP_CFG_BP_ID_BGX |
			   TXNIC_PF_INTF_BP_CFG_BP_ID ( i ) ),
			 ( pf->regs + TXNIC_PF_INTF_BP_CFG ( i ) ) );
	}
	writeq ( ( TXNIC_PF_PKIND_CFG_LENERR_EN |
		   TXNIC_PF_PKIND_CFG_MAXLEN_DISABLE |
		   TXNIC_PF_PKIND_CFG_MINLEN_DISABLE ),
		 ( pf->regs + TXNIC_PF_PKIND_CFG(0) ) );

	/* Add to list of physical functions */
	list_add_tail ( &pf->list, &txnic_pfs );

	/* Probe all LMACs, if applicable */
	list_for_each_entry ( bgx, &txnic_bgxs, list ) {
		if ( bgx->node != pf->node )
			continue;
		if ( ( rc = txnic_lmac_probe_all ( pf, bgx ) ) != 0 )
			goto err_probe;
	}

	return 0;

 err_probe:
	for ( i = 0 ; i < TXNIC_NUM_BGX ; i++ ) {
		if ( pf->bgx[i] )
			txnic_lmac_remove_all ( pf, pf->bgx[i] );
	}
	list_del ( &pf->list );
	writeq ( 0, ( pf->regs + TXNIC_PF_CFG ) );
	iounmap ( pf->regs );
 err_ioremap:
	free ( pf );
 err_alloc:
	return rc;
}

/**
 * Remove PCI device
 *
 * @v pci		PCI device
 */
static void txnic_pf_remove ( struct pci_device *pci ) {
	struct txnic_pf *pf = pci_get_drvdata ( pci );
	unsigned int i;

	/* Remove all LMACs, if applicable */
	for ( i = 0 ; i < TXNIC_NUM_BGX ; i++ ) {
		if ( pf->bgx[i] )
			txnic_lmac_remove_all ( pf, pf->bgx[i] );
	}

	/* Remove from list of physical functions */
	list_del ( &pf->list );

	/* Disable physical function */
	writeq ( 0, ( pf->regs + TXNIC_PF_CFG ) );

	/* Unmap registers */
	iounmap ( pf->regs );

	/* Free physical function */
	free ( pf );
}

/** NIC physical function PCI device IDs */
static struct pci_device_id txnic_pf_ids[] = {
	PCI_ROM ( 0x177d, 0xa01e, "thunder-pf", "ThunderX NIC PF", 0 ),
};

/** NIC physical function PCI driver */
struct pci_driver txnic_pf_driver __pci_driver = {
	.ids = txnic_pf_ids,
	.id_count = ( sizeof ( txnic_pf_ids ) / sizeof ( txnic_pf_ids[0] ) ),
	.probe = txnic_pf_probe,
	.remove = txnic_pf_remove,
};

/******************************************************************************
 *
 * BGX interface
 *
 ******************************************************************************
 */

/** LMAC types */
static struct txnic_lmac_type txnic_lmac_types[] = {
	[TXNIC_LMAC_XAUI] = {
		.name = "XAUI",
		.count = 1,
		.lane_to_sds = 0xe4,
	},
	[TXNIC_LMAC_RXAUI] = {
		.name = "RXAUI",
		.count = 2,
		.lane_to_sds = 0x0e04,
	},
	[TXNIC_LMAC_10G_R] = {
		.name = "10GBASE-R",
		.count = 4,
		.lane_to_sds = 0x00000000,
	},
	[TXNIC_LMAC_40G_R] = {
		.name = "40GBASE-R",
		.count = 1,
		.lane_to_sds = 0xe4,
	},
};

/**
 * Detect BGX Ethernet interface LMAC type
 *
 * @v bgx		BGX Ethernet interface
 * @ret type		LMAC type, or negative error
 */
static int txnic_bgx_detect ( struct txnic_bgx *bgx ) {
	uint64_t config;
	uint64_t br_pmd_control;
	uint64_t rx_lmacs;
	unsigned int type;

	/* We assume that the early (pre-UEFI) firmware will have
	 * configured at least the LMAC 0 type and use of link
	 * training, and may have overridden the number of LMACs.
	 */

	/* Determine type from LMAC 0 */
	config = readq ( bgx->regs + BGX_CMR_CONFIG );
	type = BGX_CMR_CONFIG_LMAC_TYPE_GET ( config );
	if ( ( type >= ( sizeof ( txnic_lmac_types ) /
			 sizeof ( txnic_lmac_types[0] ) ) ) ||
	     ( txnic_lmac_types[type].count == 0 ) ) {
		DBGC ( TXNICCOL ( bgx ), "TXNIC %d/%d/* BGX unknown type %d\n",
		       bgx->node, bgx->idx, type );
		return -ENOTTY;
	}
	bgx->type = &txnic_lmac_types[type];

	/* Check whether link training is required */
	br_pmd_control = readq ( bgx->regs + BGX_SPU_BR_PMD_CONTROL );
	bgx->training =
		( !! ( br_pmd_control & BGX_SPU_BR_PMD_CONTROL_TRAIN_EN ) );

	/* Determine number of LMACs */
	rx_lmacs = readq ( bgx->regs + BGX_CMR_RX_LMACS );
	bgx->count = BGX_CMR_RX_LMACS_LMACS_GET ( rx_lmacs );
	if ( ( bgx->count == TXNIC_NUM_LMAC ) &&
	     ( bgx->type->count != TXNIC_NUM_LMAC ) ) {
		DBGC ( TXNICCOL ( bgx ), "TXNIC %d/%d/* assuming %d LMACs\n",
		       bgx->node, bgx->idx, bgx->type->count );
		bgx->count = bgx->type->count;
	}

	return type;
}

/**
 * Initialise BGX Ethernet interface
 *
 * @v bgx		BGX Ethernet interface
 * @v type		LMAC type
 */
static void txnic_bgx_init ( struct txnic_bgx *bgx, unsigned int type ) {
	uint64_t global_config;
	uint32_t lane_to_sds;
	unsigned int i;

	/* Set number of LMACs */
	writeq ( BGX_CMR_RX_LMACS_LMACS_SET ( bgx->count ),
		 ( bgx->regs + BGX_CMR_RX_LMACS ) );
	writeq ( BGX_CMR_TX_LMACS_LMACS_SET ( bgx->count ),
		 ( bgx->regs + BGX_CMR_TX_LMACS ) );

	/* Set LMAC types and lane mappings, and disable all LMACs */
	lane_to_sds = bgx->type->lane_to_sds;
	for ( i = 0 ; i < bgx->count ; i++ ) {
		writeq ( ( BGX_CMR_CONFIG_LMAC_TYPE_SET ( type ) |
			   BGX_CMR_CONFIG_LANE_TO_SDS ( lane_to_sds ) ),
			 ( bgx->regs + BGX_LMAC ( i ) + BGX_CMR_CONFIG ) );
		lane_to_sds >>= 8;
	}

	/* Reset all MAC address filtering */
	for ( i = 0 ; i < TXNIC_NUM_DMAC ; i++ )
		writeq ( 0, ( bgx->regs + BGX_CMR_RX_DMAC_CAM ( i ) ) );

	/* Reset NCSI steering */
	for ( i = 0 ; i < TXNIC_NUM_STEERING ; i++ )
		writeq ( 0, ( bgx->regs + BGX_CMR_RX_STEERING ( i ) ) );

	/* Enable backpressure to all channels */
	writeq ( BGX_CMR_CHAN_MSK_AND_ALL ( bgx->count ),
		 ( bgx->regs + BGX_CMR_CHAN_MSK_AND ) );

	/* Strip FCS */
	global_config = readq ( bgx->regs + BGX_CMR_GLOBAL_CONFIG );
	global_config |= BGX_CMR_GLOBAL_CONFIG_FCS_STRIP;
	writeq ( global_config, ( bgx->regs + BGX_CMR_GLOBAL_CONFIG ) );
}

/**
 * Get MAC address
 *
 * @v lmac		Logical MAC
 */
static void txnic_bgx_mac ( struct txnic_lmac *lmac ) {
	struct txnic_bgx *bgx = lmac->bgx;
	BOARD_CFG *boardcfg;
	NODE_CFG *nodecfg;
	BGX_CFG *bgxcfg;
	LMAC_CFG *lmaccfg;

	/* Extract MAC from Board Configuration protocol, if available */
	if ( txcfg ) {
		boardcfg = txcfg->BoardConfig;
		nodecfg = &boardcfg->Node[ bgx->node % MAX_NODES ];
		bgxcfg = &nodecfg->BgxCfg[ bgx->idx % BGX_PER_NODE_COUNT ];
		lmaccfg = &bgxcfg->Lmacs[ lmac->idx % LMAC_PER_BGX_COUNT ];
		lmac->mac.be64 = cpu_to_be64 ( lmaccfg->MacAddress );
	} else {
		DBGC ( TXNICCOL ( bgx ), "TXNIC %d/%d/%d has no board "
		       "configuration protocol\n", bgx->node, bgx->idx,
		       lmac->idx );
	}

	/* Use random MAC address if none available */
	if ( ! lmac->mac.be64 ) {
		DBGC ( TXNICCOL ( bgx ), "TXNIC %d/%d/%d has no MAC address\n",
		       bgx->node, bgx->idx, lmac->idx );
		eth_random_addr ( lmac->mac.raw );
	}
}

/**
 * Initialise Super PHY Unit (SPU)
 *
 * @v lmac		Logical MAC
 */
static void txnic_bgx_spu_init ( struct txnic_lmac *lmac ) {
	struct txnic_bgx *bgx = lmac->bgx;

	/* Reset PHY */
	writeq ( BGX_SPU_CONTROL1_RESET, ( lmac->regs + BGX_SPU_CONTROL1 ) );
	mdelay ( BGX_SPU_RESET_DELAY_MS );

	/* Power down PHY */
	writeq ( BGX_SPU_CONTROL1_LO_PWR, ( lmac->regs + BGX_SPU_CONTROL1 ) );

	/* Configure training, if applicable */
	if ( bgx->training ) {
		writeq ( 0, ( lmac->regs + BGX_SPU_BR_PMD_LP_CUP ) );
		writeq ( 0, ( lmac->regs + BGX_SPU_BR_PMD_LD_CUP ) );
		writeq ( 0, ( lmac->regs + BGX_SPU_BR_PMD_LD_REP ) );
		writeq ( BGX_SPU_BR_PMD_CONTROL_TRAIN_EN,
			 ( lmac->regs + BGX_SPU_BR_PMD_CONTROL ) );
	}

	/* Disable forward error correction */
	writeq ( 0, ( lmac->regs + BGX_SPU_FEC_CONTROL ) );

	/* Disable autonegotiation */
	writeq ( 0, ( lmac->regs + BGX_SPU_AN_CONTROL ) );

	/* Power up PHY */
	writeq ( 0, ( lmac->regs + BGX_SPU_CONTROL1 ) );
}

/**
 * Initialise LMAC
 *
 * @v bgx		BGX Ethernet interface
 * @v lmac_idx		LMAC index
 */
static void txnic_bgx_lmac_init ( struct txnic_bgx *bgx,
				  unsigned int lmac_idx ) {
	struct txnic_lmac *lmac = &bgx->lmac[lmac_idx];
	uint64_t config;

	/* Record associated BGX */
	lmac->bgx = bgx;

	/* Set register base address (already mapped) */
	lmac->regs = ( bgx->regs + BGX_LMAC ( lmac_idx ) );

	/* Calculate virtual NIC index */
	lmac->idx = TXNIC_VNIC_IDX ( bgx->idx, lmac_idx );

	/* Set MAC address */
	txnic_bgx_mac ( lmac );

	/* Initialise PHY */
	txnic_bgx_spu_init ( lmac );

	/* Accept all multicasts and broadcasts */
	writeq ( ( BGX_CMR_RX_DMAC_CTL_MCST_MODE_ACCEPT |
		   BGX_CMR_RX_DMAC_CTL_BCST_ACCEPT ),
		 ( lmac->regs + BGX_CMR_RX_DMAC_CTL ) );

	/* Enable LMAC */
	config = readq ( lmac->regs + BGX_CMR_CONFIG );
	config |= ( BGX_CMR_CONFIG_ENABLE |
		    BGX_CMR_CONFIG_DATA_PKT_RX_EN |
		    BGX_CMR_CONFIG_DATA_PKT_TX_EN );
	writeq ( config, ( lmac->regs + BGX_CMR_CONFIG ) );
}

/**
 * Probe PCI device
 *
 * @v pci		PCI device
 * @ret rc		Return status code
 */
static int txnic_bgx_probe ( struct pci_device *pci ) {
	struct txnic_bgx *bgx;
	struct txnic_pf *pf;
	unsigned long membase;
	unsigned int i;
	int type;
	int rc;

	/* Allocate and initialise structure */
	bgx = zalloc ( sizeof ( *bgx ) );
	if ( ! bgx ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	bgx->pci = pci;
	pci_set_drvdata ( pci, bgx );

	/* Get base address */
	membase = pciea_bar_start ( pci, PCIEA_BEI_BAR_0 );

	/* Calculate node ID and index */
	bgx->node = txnic_address_node ( membase );
	bgx->idx = txnic_address_bgx ( membase );

	/* Fix up PCI device */
	adjust_pci_device ( pci );

	/* Map registers */
	bgx->regs = ioremap ( membase, TXNIC_BGX_BAR_SIZE );
	if ( ! bgx->regs ) {
		rc = -ENODEV;
		goto err_ioremap;
	}

	/* Detect LMAC type */
	if ( ( type = txnic_bgx_detect ( bgx ) ) < 0 ) {
		rc = type;
		goto err_detect;
	}
	DBGC ( TXNICCOL ( bgx ), "TXNIC %d/%d/* BGX %s at %#lx %dx %s%s\n",
	       bgx->node, bgx->idx, pci->dev.name, membase, bgx->count,
	       bgx->type->name, ( bgx->training ? "(training)" : "" ) );

	/* Initialise interface */
	txnic_bgx_init ( bgx, type );

	/* Initialise all LMACs */
	for ( i = 0 ; i < bgx->count ; i++ )
		txnic_bgx_lmac_init ( bgx, i );

	/* Add to list of BGX devices */
	list_add_tail ( &bgx->list, &txnic_bgxs );

	/* Probe all LMACs, if applicable */
	list_for_each_entry ( pf, &txnic_pfs, list ) {
		if ( pf->node != bgx->node )
			continue;
		if ( ( rc = txnic_lmac_probe_all ( pf, bgx ) ) != 0 )
			goto err_probe;
	}

	return 0;

	if ( bgx->pf )
		txnic_lmac_remove_all ( bgx->pf, bgx );
	list_del ( &bgx->list );
 err_probe:
 err_detect:
	iounmap ( bgx->regs );
 err_ioremap:
	free ( bgx );
 err_alloc:
	return rc;
}

/**
 * Remove PCI device
 *
 * @v pci		PCI device
 */
static void txnic_bgx_remove ( struct pci_device *pci ) {
	struct txnic_bgx *bgx = pci_get_drvdata ( pci );

	/* Remove all LMACs, if applicable */
	if ( bgx->pf )
		txnic_lmac_remove_all ( bgx->pf, bgx );

	/* Remove from list of BGX devices */
	list_del ( &bgx->list );

	/* Unmap registers */
	iounmap ( bgx->regs );

	/* Free BGX device */
	free ( bgx );
}

/** BGX PCI device IDs */
static struct pci_device_id txnic_bgx_ids[] = {
	PCI_ROM ( 0x177d, 0xa026, "thunder-bgx", "ThunderX BGX", 0 ),
};

/** BGX PCI driver */
struct pci_driver txnic_bgx_driver __pci_driver = {
	.ids = txnic_bgx_ids,
	.id_count = ( sizeof ( txnic_bgx_ids ) / sizeof ( txnic_bgx_ids[0] ) ),
	.probe = txnic_bgx_probe,
	.remove = txnic_bgx_remove,
};

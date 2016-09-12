#ifndef _THUNDERX_H
#define _THUNDERX_H

/** @file
 *
 * Cavium ThunderX Ethernet driver
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <ipxe/list.h>
#include <ipxe/netdevice.h>
#include <ipxe/uaccess.h>

/******************************************************************************
 *
 * Address space
 *
 ******************************************************************************
 */

/** Size of a cache line */
#define TXNIC_LINE_SIZE 128

/** Virtual function BAR size */
#define TXNIC_VF_BAR_SIZE 0x200000UL

/** Physical function BAR size */
#define TXNIC_PF_BAR_SIZE 0x40000000UL

/** BGX BAR size */
#define TXNIC_BGX_BAR_SIZE 0x400000UL

/** Maximum number of BGX Ethernet interfaces (per node) */
#define TXNIC_NUM_BGX 2

/** Maximum number of Logical MACs (per BGX) */
#define TXNIC_NUM_LMAC 4

/** Maximum number of destination MAC addresses (per BGX) */
#define TXNIC_NUM_DMAC 32

/** Maximum number of steering rules (per BGX) */
#define TXNIC_NUM_STEERING 8

/**
 * Calculate node ID
 *
 * @v addr		PCI BAR base address
 * @ret node		Node ID
 */
static inline unsigned int txnic_address_node ( uint64_t addr ) {

	/* Node ID is in bits [45:44] of the hardcoded BAR address */
	return ( ( addr >> 44 ) & 0x3 );
}

/**
 * Calculate BGX Ethernet interface index
 *
 * @v addr		PCI BAR base address
 * @ret index		Index
 */
static inline unsigned int txnic_address_bgx ( uint64_t addr ) {

	/* Index is in bit 24 of the hardcoded BAR address */
	return ( ( addr >> 24 ) & 0x1 );
}

/******************************************************************************
 *
 * Send queue
 *
 ******************************************************************************
 */

/** Send queue configuration */
#define TXNIC_QS_SQ_CFG(q)		( ( (q) << 18 ) | 0x010800 )
#define TXNIC_QS_SQ_CFG_ENA			(		 1ULL	<< 19 )
#define TXNIC_QS_SQ_CFG_RESET			(		 1ULL	<< 17 )
#define TXNIC_QS_SQ_CFG_QSIZE(sz)		( ( ( uint64_t ) (sz) ) <<  8 )
#define TXNIC_QS_SQ_CFG_QSIZE_1K \
	TXNIC_QS_SQ_CFG_QSIZE ( 0 )

/** Send queue base address */
#define TXNIC_QS_SQ_BASE(q)		( ( (q) << 18 ) | 0x010820 )

/** Send queue head pointer */
#define TXNIC_QS_SQ_HEAD(q)		( ( (q) << 18 ) | 0x010828 )

/** Send queue tail pointer */
#define TXNIC_QS_SQ_TAIL(q)		( ( (q) << 18 ) | 0x010830 )

/** Send queue doorbell */
#define TXNIC_QS_SQ_DOOR(q)		( ( (q) << 18 ) | 0x010838 )

/** Send queue status */
#define TXNIC_QS_SQ_STATUS(q)		( ( (q) << 18 ) | 0x010840 )
#define TXNIC_QS_SQ_STATUS_STOPPED		(		 1ULL	<< 21 )

/** Maximum time to wait for a send queue to stop
 *
 * This is a policy decision.
 */
#define TXNIC_SQ_STOP_MAX_WAIT_MS 100

/** A send header subdescriptor */
struct txnic_send_header {
	/** Total length */
	uint32_t total;
	/** Unused */
	uint8_t unused_a[2];
	/** Subdescriptor count */
	uint8_t subdcnt;
	/** Flags */
	uint8_t flags;
	/** Unused */
	uint8_t unused_b[8];
} __attribute__ (( packed ));

/** Flags for send header subdescriptor
 *
 * These comprise SUBDC=0x1 and PNC=0x1.
 */
#define TXNIC_SEND_HDR_FLAGS 0x14

/** A send gather subdescriptor */
struct txnic_send_gather {
	/** Size */
	uint16_t size;
	/** Unused */
	uint8_t unused[5];
	/** Flags */
	uint8_t flags;
	/** Address */
	uint64_t addr;
} __attribute__ (( packed ));

/** Flags for send gather subdescriptor
 *
 * These comprise SUBDC=0x4 and LD_TYPE=0x0.
 */
#define TXNIC_SEND_GATHER_FLAGS 0x40

/** A send queue entry
 *
 * Each send queue entry comprises a single send header subdescriptor
 * and a single send gather subdescriptor.
 */
struct txnic_sqe {
	/** Send header descriptor */
	struct txnic_send_header hdr;
	/** Send gather descriptor */
	struct txnic_send_gather gather;
} __attribute__ (( packed ));

/** Number of subdescriptors per send queue entry */
#define TXNIC_SQE_SUBDESCS ( sizeof ( struct txnic_sqe ) / \
			     sizeof ( struct txnic_send_header ) )

/** Number of send queue entries
 *
 * The minimum send queue size is 1024 entries.
 */
#define TXNIC_SQES ( 1024 / TXNIC_SQE_SUBDESCS )

/** Send queue maximum fill level
 *
 * This is a policy decision.
 */
#define TXNIC_SQ_FILL 32

/** Send queue alignment */
#define TXNIC_SQ_ALIGN TXNIC_LINE_SIZE

/** Send queue stride */
#define TXNIC_SQ_STRIDE sizeof ( struct txnic_sqe )

/** Send queue size */
#define TXNIC_SQ_SIZE ( TXNIC_SQES * TXNIC_SQ_STRIDE )

/** A send queue */
struct txnic_sq {
	/** Producer counter */
	unsigned int prod;
	/** Consumer counter */
	unsigned int cons;
	/** Send queue entries */
	userptr_t sqe;
};

/******************************************************************************
 *
 * Receive queue
 *
 ******************************************************************************
 */

/** Receive queue configuration */
#define TXNIC_QS_RQ_CFG(q)		( ( (q) << 18 ) | 0x010600 )
#define TXNIC_QS_RQ_CFG_ENA			(		 1ULL	<<  1 )

/** Maximum time to wait for a receive queue to disable
 *
 * This is a policy decision.
 */
#define TXNIC_RQ_DISABLE_MAX_WAIT_MS 100

/** Receive buffer descriptor ring configuration */
#define TXNIC_QS_RBDR_CFG(q)		( ( (q) << 18 ) | 0x010c00 )
#define TXNIC_QS_RBDR_CFG_ENA			(		 1ULL	<< 44 )
#define TXNIC_QS_RBDR_CFG_RESET			(		 1ULL	<< 43 )
#define TXNIC_QS_RBDR_CFG_QSIZE(sz)		( ( ( uint64_t ) (sz) ) << 32 )
#define TXNIC_QS_RBDR_CFG_QSIZE_8K \
	TXNIC_QS_RBDR_CFG_QSIZE ( 0 )
#define TXNIC_QS_RBDR_CFG_LINES(sz)		( ( ( uint64_t ) (sz) ) <<  0 )

/** Receive buffer descriptor ring base address */
#define TXNIC_QS_RBDR_BASE(q)		( ( (q) << 18 ) | 0x010c20 )

/** Receive buffer descriptor ring head pointer */
#define TXNIC_QS_RBDR_HEAD(q)		( ( (q) << 18 ) | 0x010c28 )

/** Receive buffer descriptor ring tail pointer */
#define TXNIC_QS_RBDR_TAIL(q)		( ( (q) << 18 ) | 0x010c30 )

/** Receive buffer descriptor ring doorbell */
#define TXNIC_QS_RBDR_DOOR(q)		( ( (q) << 18 ) | 0x010c38 )

/** Receive buffer descriptor ring status 0 */
#define TXNIC_QS_RBDR_STATUS0(q)	( ( (q) << 18 ) | 0x010c40 )

/** A receive buffer descriptor ring entry */
struct txnic_rbdr_entry {
	/** Address */
	uint64_t addr;
} __attribute__ (( packed ));

/** A receive queue entry */
struct txnic_rqe {
	/** Receive buffer descriptor ring entry */
	struct txnic_rbdr_entry rbdre;
} __attribute__ (( packed ));

/** Number of receive queue entries
 *
 * The minimum receive queue size is 8192 entries.
 */
#define TXNIC_RQES 8192

/** Receive queue maximum fill level
 *
 * This is a policy decision.  Must not exceed TXNIC_RQES.
 */
#define TXNIC_RQ_FILL 32

/** Receive queue entry size
 *
 * This is a policy decision.
 */
#define TXNIC_RQE_SIZE ( ( ETH_DATA_ALIGN + ETH_FRAME_LEN +		\
			   4 /* VLAN */ + TXNIC_LINE_SIZE - 1 )		\
			 & ~( TXNIC_LINE_SIZE - 1 ) )

/** Receive queue alignment */
#define TXNIC_RQ_ALIGN TXNIC_LINE_SIZE

/** Receive queue stride */
#define TXNIC_RQ_STRIDE sizeof ( struct txnic_rqe )

/** Receive queue size */
#define TXNIC_RQ_SIZE ( TXNIC_RQES * TXNIC_RQ_STRIDE )

/** A receive queue */
struct txnic_rq {
	/** Producer counter */
	unsigned int prod;
	/** Consumer counter */
	unsigned int cons;
	/** Receive queue entries */
	userptr_t rqe;
	/** I/O buffers */
	struct io_buffer *iobuf[TXNIC_RQ_FILL];
};

/******************************************************************************
 *
 * Completion queue
 *
 ******************************************************************************
 */

/** Completion queue configuration */
#define TXNIC_QS_CQ_CFG(q)		( ( (q) << 18 ) | 0x010400 )
#define TXNIC_QS_CQ_CFG_ENA			(		 1ULL	<< 42 )
#define TXNIC_QS_CQ_CFG_RESET			(		 1ULL	<< 41 )
#define TXNIC_QS_CQ_CFG_QSIZE(sz)		( ( ( uint64_t ) (sz) ) << 32 )
#define TXNIC_QS_CQ_CFG_QSIZE_256 \
	TXNIC_QS_CQ_CFG_QSIZE ( 7 )

/** Maximum time to wait for a completion queue to disable
 *
 * This is a policy decision.
 */
#define TXNIC_CQ_DISABLE_MAX_WAIT_MS 100

/** Completion queue base address */
#define TXNIC_QS_CQ_BASE(q)		( ( (q) << 18 ) | 0x010420 )

/** Completion queue head pointer */
#define TXNIC_QS_CQ_HEAD(q)		( ( (q) << 18 ) | 0x010428 )

/** Completion queue tail pointer */
#define TXNIC_QS_CQ_TAIL(q)		( ( (q) << 18 ) | 0x010430 )

/** Completion queue doorbell */
#define TXNIC_QS_CQ_DOOR(q)		( ( (q) << 18 ) | 0x010438 )

/** Completion queue status */
#define TXNIC_QS_CQ_STATUS(q)		( ( (q) << 18 ) | 0x010440 )
#define TXNIC_QS_CQ_STATUS_QCOUNT(status) \
	( ( (status) >> 0 ) & 0xffff )

/** Completion queue status 2 */
#define TXNIC_QS_CQ_STATUS2(q)		( ( (q) << 18 ) | 0x010448 )

/** A send completion queue entry */
struct txnic_cqe_send {
	/** Status */
	uint8_t send_status;
	/** Unused */
	uint8_t unused[4];
	/** Send queue entry pointer */
	uint16_t sqe_ptr;
	/** Type */
	uint8_t cqe_type;
} __attribute__ (( packed ));

/** Send completion queue entry type */
#define TXNIC_CQE_TYPE_SEND 0x80

/** A receive completion queue entry */
struct txnic_cqe_rx {
	/** Error opcode */
	uint8_t errop;
	/** Unused */
	uint8_t unused_a[6];
	/** Type */
	uint8_t cqe_type;
	/** Unused */
	uint8_t unused_b[1];
	/** Padding */
	uint8_t apad;
	/** Unused */
	uint8_t unused_c[4];
	/** Length */
	uint16_t len;
} __attribute__ (( packed ));

/** Receive completion queue entry type */
#define TXNIC_CQE_TYPE_RX 0x20

/** Applied padding */
#define TXNIC_CQE_RX_APAD_LEN( apad ) ( (apad) >> 5 )

/** Completion queue entry common fields */
struct txnic_cqe_common {
	/** Unused */
	uint8_t unused_a[7];
	/** Type */
	uint8_t cqe_type;
} __attribute__ (( packed ));

/** A completion queue entry */
union txnic_cqe {
	/** Common fields */
	struct txnic_cqe_common common;
	/** Send completion */
	struct txnic_cqe_send send;
	/** Receive completion */
	struct txnic_cqe_rx rx;
};

/** Number of completion queue entries
 *
 * The minimum completion queue size is 256 entries.
 */
#define TXNIC_CQES 256

/** Completion queue alignment */
#define TXNIC_CQ_ALIGN 512

/** Completion queue stride */
#define TXNIC_CQ_STRIDE 512

/** Completion queue size */
#define TXNIC_CQ_SIZE ( TXNIC_CQES * TXNIC_CQ_STRIDE )

/** A completion queue */
struct txnic_cq {
	/** Consumer counter */
	unsigned int cons;
	/** Completion queue entries */
	userptr_t cqe;
};

/******************************************************************************
 *
 * Virtual NIC
 *
 ******************************************************************************
 */

/** A virtual NIC */
struct txnic {
	/** Registers */
	void *regs;
	/** Device name (for debugging) */
	const char *name;
	/** Network device */
	struct net_device *netdev;

	/** Send queue */
	struct txnic_sq sq;
	/** Receive queue */
	struct txnic_rq rq;
	/** Completion queue */
	struct txnic_cq cq;
};

/******************************************************************************
 *
 * Physical function
 *
 ******************************************************************************
 */

/** Physical function configuration */
#define TXNIC_PF_CFG			0x000000
#define TXNIC_PF_CFG_ENA			(		 1ULL	<<  0 )

/** Backpressure configuration */
#define TXNIC_PF_BP_CFG			0x000080
#define TXNIC_PF_BP_CFG_BP_POLL_ENA		(		 1ULL	<<  6 )
#define TXNIC_PF_BP_CFG_BP_POLL_DLY(dl)		( ( ( uint64_t ) (dl) ) <<  0 )
#define TXNIC_PF_BP_CFG_BP_POLL_DLY_DEFAULT \
	TXNIC_PF_BP_CFG_BP_POLL_DLY ( 3 )

/** Interface send configuration */
#define TXNIC_PF_INTF_SEND_CFG(in)	( ( (in) << 8 ) | 0x000200 )
#define TXNIC_PF_INTF_SEND_CFG_BLOCK_BGX	(		 1ULL	<<  3 )
#define TXNIC_PF_INTF_SEND_CFG_BLOCK(bl)	( ( ( uint64_t ) (bl) ) <<  0 )

/** Interface backpressure configuration */
#define TXNIC_PF_INTF_BP_CFG(in)	( ( (in) << 8 ) | 0x000208 )
#define TXNIC_PF_INTF_BP_CFG_BP_ENA		(		 1ULL	<< 63 )
#define TXNIC_PF_INTF_BP_CFG_BP_ID_BGX		(		 1ULL	<<  3 )
#define TXNIC_PF_INTF_BP_CFG_BP_ID(bp)		( ( ( uint64_t ) (bp) ) <<  0 )

/** Port kind configuration */
#define TXNIC_PF_PKIND_CFG(pk)		( ( (pk) << 3 ) | 0x000600 )
#define TXNIC_PF_PKIND_CFG_LENERR_EN		(		 1ULL	<< 33 )
#define TXNIC_PF_PKIND_CFG_MAXLEN(ct)		( ( ( uint64_t ) (ct) ) << 16 )
#define TXNIC_PF_PKIND_CFG_MAXLEN_DISABLE \
	TXNIC_PF_PKIND_CFG_MAXLEN ( 0xffff )
#define TXNIC_PF_PKIND_CFG_MINLEN(ct)		( ( ( uint64_t ) (ct) ) <<  0 )
#define TXNIC_PF_PKIND_CFG_MINLEN_DISABLE \
	TXNIC_PF_PKIND_CFG_MINLEN ( 0x0000 )

/** Match parse index configuration */
#define TXNIC_PF_MPI_CFG(ix)		( ( (ix) << 3 ) | 0x210000 )
#define TXNIC_PF_MPI_CFG_VNIC(vn)		( ( ( uint64_t ) (vn) )	<< 24 )
#define TXNIC_PF_MPI_CFG_RSSI_BASE(ix)		( ( ( uint64_t ) (ix) ) <<  0 )

/** RSS indirection receive queue */
#define TXNIC_PF_RSSI_RQ(ix)		( ( (ix) << 3 ) | 0x220000 )
#define TXNIC_PF_RSSI_RQ_RQ_QS(qs)		( ( ( uint64_t ) (qs) ) << 3 )

/** LMAC registers */
#define TXNIC_PF_LMAC(lm)		( ( (lm) << 3 ) | 0x240000 )

/** LMAC configuration */
#define TXNIC_PF_LMAC_CFG		0x000000
#define TXNIC_PF_LMAC_CFG_ADJUST(ad)		( ( ( uint64_t ) (ad) ) <<  8 )
#define TXNIC_PF_LMAC_CFG_ADJUST_DEFAULT \
	TXNIC_PF_LMAC_CFG_ADJUST ( 6 )
#define TXNIC_PF_LMAC_CFG_MIN_PKT_SIZE(sz)	( ( ( uint64_t ) (sz) ) <<  0 )

/** LMAC configuration 2 */
#define TXNIC_PF_LMAC_CFG2		0x000100
#define TXNIC_PF_LMAC_CFG2_MAX_PKT_SIZE(sz)	( ( ( uint64_t ) (sz) ) <<  0 )

/** LMAC credit */
#define TXNIC_PF_LMAC_CREDIT		0x004000
#define TXNIC_PF_LMAC_CREDIT_CC_UNIT_CNT(ct)	( ( ( uint64_t ) (ct) ) << 12 )
#define TXNIC_PF_LMAC_CREDIT_CC_UNIT_CNT_DEFAULT \
	TXNIC_PF_LMAC_CREDIT_CC_UNIT_CNT ( 192 )
#define TXNIC_PF_LMAC_CREDIT_CC_PACKET_CNT(ct)	( ( ( uint64_t ) (ct) ) <<  2 )
#define TXNIC_PF_LMAC_CREDIT_CC_PACKET_CNT_DEFAULT \
	TXNIC_PF_LMAC_CREDIT_CC_PACKET_CNT ( 511 )
#define TXNIC_PF_LMAC_CREDIT_CC_ENABLE		(		 1ULL	<<  1 )

/** Channel registers */
#define TXNIC_PF_CHAN(ch)		( ( (ch) << 3 ) | 0x400000 )

/** Channel transmit configuration */
#define TXNIC_PF_CHAN_TX_CFG		0x000000
#define TXNIC_PF_CHAN_TX_CFG_BP_ENA		(		 1ULL	<<  0 )

/** Channel receive configuration */
#define TXNIC_PF_CHAN_RX_CFG		0x020000
#define TXNIC_PF_CHAN_RX_CFG_CPI_BASE(ix)	( ( ( uint64_t ) (ix) ) << 48 )

/** Channel receive backpressure configuration */
#define TXNIC_PF_CHAN_RX_BP_CFG		0x080000
#define TXNIC_PF_CHAN_RX_BP_CFG_ENA		(		 1ULL	<< 63 )
#define TXNIC_PF_CHAN_RX_BP_CFG_BPID(bp)	( ( ( uint64_t ) (bp) ) <<  0 )

/** Traffic limiter 2 configuration */
#define TXNIC_PF_TL2_CFG(tl)		( ( (tl) << 3 ) | 0x500000 )
#define TXNIC_PF_TL2_CFG_RR_QUANTUM(rr)		( ( ( uint64_t ) (rr) ) <<  0 )
#define TXNIC_PF_TL2_CFG_RR_QUANTUM_DEFAULT \
	TXNIC_PF_TL2_CFG_RR_QUANTUM ( 0x905 )

/** Traffic limiter 3 configuration */
#define TXNIC_PF_TL3_CFG(tl)		( ( (tl) << 3 ) | 0x600000 )
#define TXNIC_PF_TL3_CFG_RR_QUANTUM(rr)		( ( ( uint64_t ) (rr) ) <<  0 )
#define TXNIC_PF_TL3_CFG_RR_QUANTUM_DEFAULT \
	TXNIC_PF_TL3_CFG_RR_QUANTUM ( 0x905 )

/** Traffic limiter 3 channel mapping */
#define TXNIC_PF_TL3_CHAN(tl)		( ( (tl) << 3 ) | 0x620000 )
#define TXNIC_PF_TL3_CHAN_CHAN(ch)		( ( (ch) & 0x7f ) << 0 )

/** Traffic limiter 4 configuration */
#define TXNIC_PF_TL4_CFG(tl)		( ( (tl) << 3 ) | 0x800000 )
#define TXNIC_PF_TL4_CFG_SQ_QS(qs)		( ( ( uint64_t ) (qs) ) << 27 )
#define TXNIC_PF_TL4_CFG_RR_QUANTUM(rr)		( ( ( uint64_t ) (rr) ) <<  0 )
#define TXNIC_PF_TL4_CFG_RR_QUANTUM_DEFAULT \
	TXNIC_PF_TL4_CFG_RR_QUANTUM ( 0x905 )

/** Queue set registers */
#define TXNIC_PF_QS(qs)			( ( (qs) << 21 ) | 0x20000000UL )

/** Queue set configuration */
#define TXNIC_PF_QS_CFG			0x010000
#define TXNIC_PF_QS_CFG_ENA			(		 1ULL	<< 31 )
#define TXNIC_PF_QS_CFG_VNIC(vn)		( ( ( uint64_t ) (vn) ) <<  0 )

/** Receive queue configuration */
#define TXNIC_PF_QS_RQ_CFG(q)		( ( (q) << 18 ) | 0x010400 )
#define TXNIC_PF_QS_RQ_CFG_CACHING(cx)		( ( ( uint64_t ) (cx) ) << 26 )
#define TXNIC_PF_QS_RQ_CFG_CACHING_ALL \
	TXNIC_PF_QS_RQ_CFG_CACHING ( 1 )
#define TXNIC_PF_QS_RQ_CFG_CQ_QS(qs)		( ( ( uint64_t ) (qs) ) << 19 )
#define TXNIC_PF_QS_RQ_CFG_RBDR_CONT_QS(qs)	( ( ( uint64_t ) (qs) ) <<  9 )
#define TXNIC_PF_QS_RQ_CFG_RBDR_STRT_QS(qs)	( ( ( uint64_t ) (qs) ) <<  1 )

/** Receive queue drop configuration */
#define TXNIC_PF_QS_RQ_DROP_CFG(q)	( ( (q) << 18 ) | 0x010420 )

/** Receive queue backpressure configuration */
#define TXNIC_PF_QS_RQ_BP_CFG(q)	( ( (q) << 18 ) | 0x010500 )
#define TXNIC_PF_QS_RQ_BP_CFG_RBDR_BP_ENA	(		 1ULL	<< 63 )
#define TXNIC_PF_QS_RQ_BP_CFG_CQ_BP_ENA		(		 1ULL	<< 62 )
#define TXNIC_PF_QS_RQ_BP_CFG_BPID(bp)		( ( ( uint64_t ) (bp) ) <<  0 )

/** Send queue configuration */
#define TXNIC_PF_QS_SQ_CFG(q)		( ( (q) << 18 ) | 0x010c00 )
#define TXNIC_PF_QS_SQ_CFG_CQ_QS(qs)		( ( ( uint64_t ) (qs) ) <<  3 )

/** Send queue configuration 2 */
#define TXNIC_PF_QS_SQ_CFG2(q)		( ( (q) << 18 ) | 0x010c08 )
#define TXNIC_PF_QS_SQ_CFG2_TL4(tl)		( ( ( uint64_t ) (tl) ) <<  0 )

/** A physical function */
struct txnic_pf {
	/** Registers */
	void *regs;
	/** PCI device */
	struct pci_device *pci;
	/** Node ID */
	unsigned int node;

	/** Virtual function BAR base */
	unsigned long vf_membase;
	/** Virtual function BAR stride */
	unsigned long vf_stride;

	/** List of physical functions */
	struct list_head list;
	/** BGX Ethernet interfaces (if known) */
	struct txnic_bgx *bgx[TXNIC_NUM_BGX];
};

/**
 * Calculate virtual NIC index
 *
 * @v bgx_idx		BGX Ethernet interface index
 * @v lmac_idx		Logical MAC index
 * @ret vnic_idx	Virtual NIC index
 */
#define TXNIC_VNIC_IDX( bgx_idx, lmac_idx ) \
	( ( (bgx_idx) * TXNIC_NUM_LMAC ) + (lmac_idx) )

/**
 * Calculate BGX Ethernet interface index
 *
 * @v vnic_idx		Virtual NIC index
 * @ret bgx_idx		BGX Ethernet interface index
 */
#define TXNIC_BGX_IDX( vnic_idx ) ( (vnic_idx) / TXNIC_NUM_LMAC )

/**
 * Calculate logical MAC index
 *
 * @v vnic_idx		Virtual NIC index
 * @ret lmac_idx	Logical MAC index
 */
#define TXNIC_LMAC_IDX( vnic_idx ) ( (vnic_idx) % TXNIC_NUM_LMAC )

/**
 * Calculate traffic limiter 2 index
 *
 * @v vnic_idx		Virtual NIC index
 * @v tl2_idx		Traffic limiter 2 index
 */
#define TXNIC_TL2_IDX( vnic_idx ) ( (vnic_idx) << 3 )

/**
 * Calculate traffic limiter 3 index
 *
 * @v vnic_idx		Virtual NIC index
 * @v tl3_idx		Traffic limiter 3 index
 */
#define TXNIC_TL3_IDX( vnic_idx ) ( (vnic_idx) << 5 )

/**
 * Calculate traffic limiter 4 index
 *
 * @v vnic_idx		Virtual NIC index
 * @v tl4_idx		Traffic limiter 4 index
 */
#define TXNIC_TL4_IDX( vnic_idx ) ( (vnic_idx) << 7 )

/**
 * Calculate channel index
 *
 * @v vnic_idx		Virtual NIC index
 * @v chan_idx		Channel index
 */
#define TXNIC_CHAN_IDX( vnic_idx ) ( ( TXNIC_BGX_IDX (vnic_idx) << 7 ) | \
				     ( TXNIC_LMAC_IDX (vnic_idx) << 4 ) )

/******************************************************************************
 *
 * BGX Ethernet interface
 *
 ******************************************************************************
 */

/** Per-LMAC registers */
#define BGX_LMAC(lm)			( ( (lm) << 20 ) | 0x00000000UL )

/** CMR configuration */
#define BGX_CMR_CONFIG			0x000000
#define BGX_CMR_CONFIG_ENABLE			(		 1ULL	<< 15 )
#define BGX_CMR_CONFIG_DATA_PKT_RX_EN		(		 1ULL	<< 14 )
#define BGX_CMR_CONFIG_DATA_PKT_TX_EN		(		 1ULL	<< 13 )
#define BGX_CMR_CONFIG_LMAC_TYPE_GET(config) \
	( ( (config) >> 8 ) & 0x7 )
#define BGX_CMR_CONFIG_LMAC_TYPE_SET(ty)	( ( ( uint64_t ) (ty) ) <<  8 )
#define BGX_CMR_CONFIG_LANE_TO_SDS(ls)		( ( ( uint64_t ) (ls) ) <<  0 )

/** CMR global configuration */
#define BGX_CMR_GLOBAL_CONFIG		0x000008
#define BGX_CMR_GLOBAL_CONFIG_FCS_STRIP		(		 1ULL	<<  6 )

/** CMR receive statistics 0 */
#define BGX_CMR_RX_STAT0		0x000070

/** CMR receive statistics 1 */
#define BGX_CMR_RX_STAT1		0x000078

/** CMR receive statistics 2 */
#define BGX_CMR_RX_STAT2		0x000080

/** CMR receive statistics 3 */
#define BGX_CMR_RX_STAT3		0x000088

/** CMR receive statistics 4 */
#define BGX_CMR_RX_STAT4		0x000090

/** CMR receive statistics 5 */
#define BGX_CMR_RX_STAT5		0x000098

/** CMR receive statistics 6 */
#define BGX_CMR_RX_STAT6		0x0000a0

/** CMR receive statistics 7 */
#define BGX_CMR_RX_STAT7		0x0000a8

/** CMR receive statistics 8 */
#define BGX_CMR_RX_STAT8		0x0000b0

/** CMR receive statistics 9 */
#define BGX_CMR_RX_STAT9		0x0000b8

/** CMR receive statistics 10 */
#define BGX_CMR_RX_STAT10		0x0000c0

/** CMR destination MAC control */
#define BGX_CMR_RX_DMAC_CTL		0x0000e8
#define BGX_CMR_RX_DMAC_CTL_MCST_MODE(md)	( ( ( uint64_t ) (md) ) <<  1 )
#define BGX_CMR_RX_DMAC_CTL_MCST_MODE_ACCEPT \
	BGX_CMR_RX_DMAC_CTL_MCST_MODE ( 1 )
#define BGX_CMR_RX_DMAC_CTL_BCST_ACCEPT		(		 1ULL	<<  0 )

/** CMR destination MAC CAM */
#define BGX_CMR_RX_DMAC_CAM(i)		( ( (i) << 3 ) | 0x000200 )

/** CMR receive steering */
#define BGX_CMR_RX_STEERING(i)		( ( (i) << 3 ) | 0x000300 )

/** CMR backpressure channel mask AND */
#define BGX_CMR_CHAN_MSK_AND		0x000450
#define BGX_CMR_CHAN_MSK_AND_ALL(count) \
	( 0xffffffffffffffffULL >> ( 16 * ( 4 - (count) ) ) )

/** CMR transmit statistics 0 */
#define BGX_CMR_TX_STAT0		0x000600

/** CMR transmit statistics 1 */
#define BGX_CMR_TX_STAT1		0x000608

/** CMR transmit statistics 2 */
#define BGX_CMR_TX_STAT2		0x000610

/** CMR transmit statistics 3 */
#define BGX_CMR_TX_STAT3		0x000618

/** CMR transmit statistics 4 */
#define BGX_CMR_TX_STAT4		0x000620

/** CMR transmit statistics 5 */
#define BGX_CMR_TX_STAT5		0x000628

/** CMR transmit statistics 6 */
#define BGX_CMR_TX_STAT6		0x000630

/** CMR transmit statistics 7 */
#define BGX_CMR_TX_STAT7		0x000638

/** CMR transmit statistics 8 */
#define BGX_CMR_TX_STAT8		0x000640

/** CMR transmit statistics 9 */
#define BGX_CMR_TX_STAT9		0x000648

/** CMR transmit statistics 10 */
#define BGX_CMR_TX_STAT10		0x000650

/** CMR transmit statistics 11 */
#define BGX_CMR_TX_STAT11		0x000658

/** CMR transmit statistics 12 */
#define BGX_CMR_TX_STAT12		0x000660

/** CMR transmit statistics 13 */
#define BGX_CMR_TX_STAT13		0x000668

/** CMR transmit statistics 14 */
#define BGX_CMR_TX_STAT14		0x000670

/** CMR transmit statistics 15 */
#define BGX_CMR_TX_STAT15		0x000678

/** CMR transmit statistics 16 */
#define BGX_CMR_TX_STAT16		0x000680

/** CMR transmit statistics 17 */
#define BGX_CMR_TX_STAT17		0x000688

/** CMR receive logical MACs */
#define BGX_CMR_RX_LMACS		0x000468
#define BGX_CMR_RX_LMACS_LMACS_GET(lmacs) \
	( ( (lmacs) >> 0 ) & 0x7 )
#define BGX_CMR_RX_LMACS_LMACS_SET(ct)		( ( ( uint64_t ) (ct) ) <<  0 )

/** CMR transmit logical MACs */
#define BGX_CMR_TX_LMACS		0x001000
#define BGX_CMR_TX_LMACS_LMACS_GET(lmacs) \
	( ( (lmacs) >> 0 ) & 0x7 )
#define BGX_CMR_TX_LMACS_LMACS_SET(ct)		( ( ( uint64_t ) (ct) ) <<  0 )

/** SPU control 1 */
#define BGX_SPU_CONTROL1		0x010000
#define BGX_SPU_CONTROL1_RESET			(		 1ULL	<< 15 )
#define BGX_SPU_CONTROL1_LO_PWR			(		 1ULL	<< 11 )

/** SPU reset delay */
#define BGX_SPU_RESET_DELAY_MS 10

/** SPU status 1 */
#define BGX_SPU_STATUS1			0x010008
#define BGX_SPU_STATUS1_FLT			(		 1ULL	<<  7 )
#define BGX_SPU_STATUS1_RCV_LNK			(		 1ULL	<<  2 )

/** SPU status 2 */
#define BGX_SPU_STATUS2			0x010020
#define BGX_SPU_STATUS2_RCVFLT			(		 1ULL	<< 10 )

/** SPU BASE-R status 1 */
#define BGX_SPU_BR_STATUS1		0x010030
#define BGX_SPU_BR_STATUS1_RCV_LNK		(		 1ULL	<< 12 )
#define BGX_SPU_BR_STATUS1_HI_BER		(		 1ULL	<<  1 )
#define BGX_SPU_BR_STATUS1_BLK_LOCK		(		 1ULL	<<  0 )

/** SPU BASE-R status 2 */
#define BGX_SPU_BR_STATUS2		0x010038
#define BGX_SPU_BR_STATUS2_LATCHED_LOCK		(		 1ULL	<< 15 )
#define BGX_SPU_BR_STATUS2_LATCHED_BER		(		 1ULL	<< 14 )

/** SPU BASE-R alignment status */
#define BGX_SPU_BR_ALGN_STATUS		0x010050
#define BGX_SPU_BR_ALGN_STATUS_ALIGND		(		 1ULL	<< 12 )

/** SPU BASE-R link training control */
#define BGX_SPU_BR_PMD_CONTROL		0x010068
#define BGX_SPU_BR_PMD_CONTROL_TRAIN_EN		(		 1ULL	<<  1 )

/** SPU BASE-R link training status */
#define BGX_SPU_BR_PMD_STATUS		0x010070

/** SPU link partner coefficient update */
#define BGX_SPU_BR_PMD_LP_CUP		0x010078

/** SPU local device coefficient update */
#define BGX_SPU_BR_PMD_LD_CUP		0x010088

/** SPU local device status report */
#define BGX_SPU_BR_PMD_LD_REP		0x010090

/** SPU forward error correction control */
#define BGX_SPU_FEC_CONTROL		0x0100a0

/** SPU autonegotation control */
#define BGX_SPU_AN_CONTROL		0x0100c8

/** SPU autonegotiation status */
#define BGX_SPU_AN_STATUS		0x0100d0
#define BGX_SPU_AN_STATUS_XNP_STAT		(		 1ULL	<<  7 )
#define BGX_SPU_AN_STATUS_PAGE_RX		(		 1ULL	<<  6 )
#define BGX_SPU_AN_STATUS_AN_COMPLETE		(		 1ULL	<<  5 )
#define BGX_SPU_AN_STATUS_LINK_STATUS		(		 1ULL	<<  2 )
#define BGX_SPU_AN_STATUS_LP_AN_ABLE		(		 1ULL	<<  0 )

/** SPU interrupt */
#define BGX_SPU_INT			0x010220
#define BGX_SPU_INT_TRAINING_FAIL		(		 1ULL	<< 14 )
#define BGX_SPU_INT_TRAINING_DONE		(		 1ULL	<< 13 )
#define BGX_SPU_INT_AN_COMPLETE			(		 1ULL	<< 12 )
#define BGX_SPU_INT_AN_LINK_GOOD		(		 1ULL	<< 11 )
#define BGX_SPU_INT_AN_PAGE_RX			(		 1ULL	<< 10 )
#define BGX_SPU_INT_FEC_UNCORR			(		 1ULL	<<  9 )
#define BGX_SPU_INT_FEC_CORR			(		 1ULL	<<  8 )
#define BGX_SPU_INT_BIP_ERR			(		 1ULL	<<  7 )
#define BGX_SPU_INT_DBG_SYNC			(		 1ULL	<<  6 )
#define BGX_SPU_INT_ALGNLOS			(		 1ULL	<<  5 )
#define BGX_SPU_INT_SYNLOS			(		 1ULL	<<  4 )
#define BGX_SPU_INT_BITLCKLS			(		 1ULL	<<  3 )
#define BGX_SPU_INT_ERR_BLK			(		 1ULL	<<  2 )
#define BGX_SPU_INT_RX_LINK_DOWN		(		 1ULL	<<  1 )
#define BGX_SPU_INT_RX_LINK_UP			(		 1ULL	<<  0 )

/** LMAC types */
enum txnic_lmac_types {
	TXNIC_LMAC_SGMII	= 0x0,		/**< SGMII/1000BASE-X */
	TXNIC_LMAC_XAUI		= 0x1,		/**< 10GBASE-X/XAUI or DXAUI */
	TXNIC_LMAC_RXAUI	= 0x2,		/**< Reduced XAUI */
	TXNIC_LMAC_10G_R	= 0x3,		/**< 10GBASE-R */
	TXNIC_LMAC_40G_R	= 0x4,		/**< 40GBASE-R */
};

/** An LMAC type */
struct txnic_lmac_type {
	/** Name */
	const char *name;
	/** Number of LMACs */
	uint8_t count;
	/** Lane-to-SDS mapping */
	uint32_t lane_to_sds;
};

/** An LMAC address */
union txnic_lmac_address {
	struct {
		uint8_t pad[2];
		uint8_t raw[ETH_ALEN];
	} __attribute__ (( packed ));
	uint64_t be64;
};

/** A Logical MAC (LMAC) */
struct txnic_lmac {
	/** Registers */
	void *regs;
	/** Containing BGX Ethernet interface */
	struct txnic_bgx *bgx;
	/** Virtual NIC index */
	unsigned int idx;

	/** MAC address */
	union txnic_lmac_address mac;

	/** Virtual NIC (if applicable) */
	struct txnic *vnic;
};

/** A BGX Ethernet interface */
struct txnic_bgx {
	/** Registers */
	void *regs;
	/** PCI device */
	struct pci_device *pci;
	/** Node ID */
	unsigned int node;
	/** BGX index */
	unsigned int idx;

	/** LMAC type */
	struct txnic_lmac_type *type;
	/** Number of LMACs */
	unsigned int count;
	/** Link training is in use */
	int training;

	/** List of BGX Ethernet interfaces */
	struct list_head list;
	/** Physical function (if known) */
	struct txnic_pf *pf;

	/** Logical MACs */
	struct txnic_lmac lmac[TXNIC_NUM_LMAC];
};

#endif /* _THUNDERX_H */

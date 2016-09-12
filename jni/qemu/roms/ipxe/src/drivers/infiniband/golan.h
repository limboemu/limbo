#ifndef _GOLAN_H_
#define _GOLAN_H_

/*
 * Copyright (C) 2013-2015 Mellanox Technologies Ltd.
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

#include <byteswap.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <ipxe/io.h>
#include <ipxe/pci.h>
#include <ipxe/pcibackup.h>
#include "CIB_PRM.h"

#define GOLAN_PCI_CONFIG_BAR_SIZE	0x100000//HERMON_PCI_CONFIG_BAR_SIZE //TODO: What is the BAR size?

#define GOLAN_PAS_SIZE	sizeof(uint64_t)

#define GOLAN_INVALID_LKEY 0x00000100UL

#define GOLAN_MAX_PORTS	2
#define GOLAN_PORT_BASE 1

#define MELLANOX_VID	0x15b3
#define GOLAN_HCA_BAR	PCI_BASE_ADDRESS_0	//BAR 0

#define GOLAN_HCR_MAX_WAIT_MS	10000

#define min(a,b) ((a)<(b)?(a):(b))

#define GOLAN_PAGE_SHIFT	12
#define	GOLAN_PAGE_SIZE		(1 << GOLAN_PAGE_SHIFT)
#define GOLAN_PAGE_MASK		(GOLAN_PAGE_SIZE - 1)

#define MAX_MBOX	( GOLAN_PAGE_SIZE / MAILBOX_STRIDE )
#define DEF_CMD_IDX	1
#define MEM_CMD_IDX	0
#define NO_MBOX		0xffff
#define MEM_MBOX	MEM_CMD_IDX
#define GEN_MBOX	DEF_CMD_IDX

#define CMD_IF_REV	4

#define MAX_PASE_MBOX	((GOLAN_CMD_PAS_CNT) - 2)

#define CMD_STATUS( golan , idx )		((struct golan_outbox_hdr *)(get_cmd( (golan) , (idx) )->out))->status
#define CMD_SYND( golan , idx )		((struct golan_outbox_hdr *)(get_cmd( (golan) , (idx) )->out))->syndrome
#define QRY_PAGES_OUT( golan, idx )		((struct golan_query_pages_outbox *)(get_cmd( (golan) , (idx) )->out))

#define VIRT_2_BE64_BUS( addr )		cpu_to_be64(((unsigned long long )virt_to_bus(addr)))
#define BE64_BUS_2_VIRT( addr )		bus_to_virt(be64_to_cpu(addr))
#define USR_2_BE64_BUS( addr )		cpu_to_be64(((unsigned long long )user_to_phys(addr, 0)))
#define BE64_BUS_2_USR( addr )		be64_to_cpu(phys_to_user(addr))

#define GET_INBOX(golan, idx)		(&(((struct mbox *)(golan->mboxes.inbox))[idx]))
#define GET_OUTBOX(golan, idx)		(&(((struct mbox *)(golan->mboxes.outbox))[idx]))

#define GOLAN_MBOX_IN( cmd_ptr, in_ptr ) ( {				  \
	union {								  \
		__be32 raw[4];						  \
		typeof ( *(in_ptr) ) cooked;				  \
	} *u = container_of ( &(cmd_ptr)->in[0], typeof ( *u ), raw[0] ); \
	&u->cooked; } )

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

/* Fw status fields */
typedef enum {
	NO_ERRORS          = 0x0,
	SIGNATURE_ERROR    = 0x1,
	TOKEN_ERROR        = 0x2,
	BAD_BLOCK_NUMBER   = 0x3,
	BAD_OUTPUT_POINTER = 0x4,   // pointer not align to mailbox size
	BAD_INPUT_POINTER  = 0x5,   // pointer not align to mailbox size
	INTERNAL_ERROR     = 0x6,
	INPUT_LEN_ERROR    = 0x7,   // input  length less than 0x8.
	OUTPUT_LEN_ERROR   = 0x8,   // output length less than 0x8.
	RESERVE_NOT_ZERO   = 0x9,
	BAD_CMD_TYPE       = 0x10,
} return_hdr_t;

struct golan_cmdq_md {
	void	*addr;
	u16	log_stride;
	u16	size;
};

struct golan_uar {
    uint32_t    	index;
    void        	*virt;
    unsigned long	phys;
};

/* Queue Pair */
#define GOLAN_SEND_WQE_BB_SIZE			64
#define GOLAN_SEND_UD_WQE_SIZE			sizeof(struct golan_send_wqe_ud)
#define GOLAN_RECV_WQE_SIZE				sizeof(struct golan_recv_wqe_ud)
#define GOLAN_WQEBBS_PER_SEND_UD_WQE	DIV_ROUND_UP(GOLAN_SEND_UD_WQE_SIZE, GOLAN_SEND_WQE_BB_SIZE)
#define GOLAN_SEND_OPCODE			0x0a
#define GOLAN_WQE_CTRL_WQE_IDX_BIT	8

enum golan_ib_qp_state {
	GOLAN_IB_QPS_RESET,
	GOLAN_IB_QPS_INIT,
	GOLAN_IB_QPS_RTR,
	GOLAN_IB_QPS_RTS,
	GOLAN_IB_QPS_SQD,
	GOLAN_IB_QPS_SQE,
	GOLAN_IB_QPS_ERR
};

struct golan_send_wqe_ud {
	struct golan_wqe_ctrl_seg ctrl;
	struct golan_av datagram;
	struct golan_wqe_data_seg data;
};

union golan_send_wqe {
	struct golan_send_wqe_ud ud;
	uint8_t pad[GOLAN_WQEBBS_PER_SEND_UD_WQE * GOLAN_SEND_WQE_BB_SIZE];
};

struct golan_recv_wqe_ud {
	struct golan_wqe_data_seg data[2];
};

struct golan_recv_wq {
	struct golan_recv_wqe_ud *wqes;
	/* WQ size in bytes */
	int	size;
	/* In SQ, it will be increased in wqe_size (number of WQEBBs per WQE) */
	u16 next_idx;
	/** GRH buffers (if applicable) */
	struct ib_global_route_header *grh;
	/** Size of GRH buffers */
	size_t grh_size;
};

struct golan_send_wq {
	union golan_send_wqe *wqes;
	/* WQ size in bytes */
	int size;
	/* In SQ, it will be increased in wqe_size (number of WQEBBs per WQE) */
	u16 next_idx;
};

struct golan_queue_pair {
	void *wqes;
	int size;
	struct golan_recv_wq rq;
	struct golan_send_wq sq;
	struct golan_qp_db *doorbell_record;
	u32 doorbell_qpn;
	enum golan_ib_qp_state state;
};

/* Completion Queue */
#define GOLAN_CQE_OPCODE_NOT_VALID	0x0f
#define GOLAN_CQE_OPCODE_BIT		4
#define GOLAN_CQ_DB_RECORD_SIZE		sizeof(uint64_t)
#define GOLAN_CQE_OWNER_MASK		1

#define MANAGE_PAGES_PSA_OFFSET		0
#define PXE_CMDIF_REF			5

enum {
	GOLAN_CQE_SW_OWNERSHIP = 0x0,
	GOLAN_CQE_HW_OWNERSHIP = 0x1
};

enum {
	GOLAN_CQE_SIZE_64	= 0,
	GOLAN_CQE_SIZE_128	= 1
};

struct golan_completion_queue {
	struct golan_cqe64	*cqes;
	int					size;
	__be64		*doorbell_record;
};


/* Event Queue */
#define GOLAN_EQE_SIZE				sizeof(struct golan_eqe)
#define GOLAN_NUM_EQES 				8
#define GOLAN_EQ_DOORBELL_OFFSET		0x40

#define GOLAN_EQ_MAP_ALL_EVENTS					\
	((1 << GOLAN_EVENT_TYPE_PATH_MIG         	)|	\
	(1 << GOLAN_EVENT_TYPE_COMM_EST          	)|	\
	(1 << GOLAN_EVENT_TYPE_SQ_DRAINED        	)|	\
	(1 << GOLAN_EVENT_TYPE_SRQ_LAST_WQE		)|	\
	(1 << GOLAN_EVENT_TYPE_SRQ_RQ_LIMIT      	)|	\
	(1 << GOLAN_EVENT_TYPE_CQ_ERROR          	)|	\
	(1 << GOLAN_EVENT_TYPE_WQ_CATAS_ERROR    	)|	\
	(1 << GOLAN_EVENT_TYPE_PATH_MIG_FAILED   	)|	\
	(1 << GOLAN_EVENT_TYPE_WQ_INVAL_REQ_ERROR	)|	\
	(1 << GOLAN_EVENT_TYPE_WQ_ACCESS_ERROR	 	)|	\
	(1 << GOLAN_EVENT_TYPE_SRQ_CATAS_ERROR   	)|	\
	(1 << GOLAN_EVENT_TYPE_INTERNAL_ERROR  	 	)|	\
	(1 << GOLAN_EVENT_TYPE_PORT_CHANGE   	  	)|	\
	(1 << GOLAN_EVENT_TYPE_GPIO_EVENT         	)|	\
	(1 << GOLAN_EVENT_TYPE_CLIENT_RE_REGISTER 	)|	\
	(1 << GOLAN_EVENT_TYPE_REMOTE_CONFIG     	)|	\
	(1 << GOLAN_EVENT_TYPE_DB_BF_CONGESTION   	)|	\
	(1 << GOLAN_EVENT_TYPE_STALL_EVENT        	)|	\
	(1 << GOLAN_EVENT_TYPE_PACKET_DROPPED     	)|	\
	(1 << GOLAN_EVENT_TYPE_CMD             	  	)|	\
	(1 << GOLAN_EVENT_TYPE_PAGE_REQUEST       	))

enum golan_event {
	GOLAN_EVENT_TYPE_COMP			= 0x0,

	GOLAN_EVENT_TYPE_PATH_MIG		= 0x01,
	GOLAN_EVENT_TYPE_COMM_EST		= 0x02,
	GOLAN_EVENT_TYPE_SQ_DRAINED		= 0x03,
	GOLAN_EVENT_TYPE_SRQ_LAST_WQE		= 0x13,
	GOLAN_EVENT_TYPE_SRQ_RQ_LIMIT		= 0x14,

	GOLAN_EVENT_TYPE_CQ_ERROR		= 0x04,
	GOLAN_EVENT_TYPE_WQ_CATAS_ERROR		= 0x05,
	GOLAN_EVENT_TYPE_PATH_MIG_FAILED	= 0x07,
	GOLAN_EVENT_TYPE_WQ_INVAL_REQ_ERROR	= 0x10,
	GOLAN_EVENT_TYPE_WQ_ACCESS_ERROR	= 0x11,
	GOLAN_EVENT_TYPE_SRQ_CATAS_ERROR	= 0x12,

	GOLAN_EVENT_TYPE_INTERNAL_ERROR		= 0x08,
	GOLAN_EVENT_TYPE_PORT_CHANGE		= 0x09,
	GOLAN_EVENT_TYPE_GPIO_EVENT		= 0x15,
//	GOLAN_EVENT_TYPE_CLIENT_RE_REGISTER	= 0x16,
	GOLAN_EVENT_TYPE_REMOTE_CONFIG		= 0x19,

	GOLAN_EVENT_TYPE_DB_BF_CONGESTION	= 0x1a,
	GOLAN_EVENT_TYPE_STALL_EVENT		= 0x1b,

	GOLAN_EVENT_TYPE_PACKET_DROPPED		= 0x1f,

	GOLAN_EVENT_TYPE_CMD			= 0x0a,
	GOLAN_EVENT_TYPE_PAGE_REQUEST		= 0x0b,
	GOLAN_EVENT_TYPE_PAGE_FAULT		= 0x0C,
};

enum golan_port_sub_event {
    GOLAN_PORT_CHANGE_SUBTYPE_DOWN		= 1,
    GOLAN_PORT_CHANGE_SUBTYPE_ACTIVE		= 4,
    GOLAN_PORT_CHANGE_SUBTYPE_INITIALIZED	= 5,
    GOLAN_PORT_CHANGE_SUBTYPE_LID		= 6,
    GOLAN_PORT_CHANGE_SUBTYPE_PKEY		= 7,
    GOLAN_PORT_CHANGE_SUBTYPE_GUID		= 8,
    GOLAN_PORT_CHANGE_SUBTYPE_CLIENT_REREG	= 9
};


enum {
	GOLAN_EQE_SW_OWNERSHIP = 0x0,
	GOLAN_EQE_HW_OWNERSHIP = 0x1
};

enum {
	GOLAN_EQ_UNARMED	= 0,
	GOLAN_EQ_ARMED		= 1,
};

struct golan_event_queue {
	uint8_t			eqn;
	uint64_t		mask;
	struct golan_eqe	*eqes;
	int			size;
	__be32			*doorbell;
	uint32_t		cons_index;
};

struct golan_port {
	/** Infiniband device */
	struct ib_device	*ibdev;
	/** Network device */
	struct net_device	*netdev;
	/** VEP number */
	u8 vep_number;
};

struct golan_mboxes {
	void 	*inbox;
	void	*outbox;
};

#define GOLAN_OPEN	0x1

struct golan {
	struct pci_device		*pci;
	struct golan_hca_init_seg	*iseg;
	struct golan_cmdq_md		cmd;
	struct golan_hca_cap		caps; /* stored as big indian*/
	struct golan_mboxes		mboxes;
	struct list_head		pages;
	uint32_t			cmd_bm;
	uint32_t			total_dma_pages;
	struct golan_uar		uar;
	struct golan_event_queue 	eq;
	uint32_t			pdn;
	u32				mkey;
	u32				flags;

	struct golan_port		ports[GOLAN_MAX_PORTS];
};

#endif /* _GOLAN_H_*/

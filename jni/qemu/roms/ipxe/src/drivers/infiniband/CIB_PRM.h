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

#ifndef __CIB_PRM__
#define __CIB_PRM__

typedef unsigned long long	__be64;
typedef uint32_t		__be32;
typedef uint16_t		__be16;

#define GOLAN_CMD_DATA_BLOCK_SIZE	(1 << 9)
#define GOLAN_CMD_PAS_CNT			(GOLAN_CMD_DATA_BLOCK_SIZE / sizeof(__be64))
#define MAILBOX_STRIDE				(1 << 10)
#define MAILBOX_MASK				(MAILBOX_STRIDE - 1)

#define GOLAN_PCI_CMD_XPORT			7
#define CMD_OWNER_HW				0x1

#define IB_NUM_PKEYS		0x20

struct health_buffer {
	__be32          assert_var[5];
	__be32          rsvd0[3];
	__be32          assert_exit_ptr;
	__be32          assert_callra;
	__be32          rsvd1[2];
	__be32          fw_ver;
	__be32          hw_id;
	__be32          rsvd2;
	u8              irisc_index;
	u8              synd;
	__be16          ext_sync;
} __attribute ( ( packed ) );

struct golan_hca_init_seg {
	__be32                  fw_rev;
	__be32                  cmdif_rev_fw_sub;
	__be32                  rsvd0[2];
	__be32                  cmdq_addr_h;
	__be32                  cmdq_addr_l_sz;
	__be32                  cmd_dbell;
	__be32                  rsvd1[121];
	struct health_buffer    health;
	__be32                  rsvd2[884];
	__be32                  health_counter;
	__be32                  rsvd3[1023];
	__be64                  ieee1588_clk;
	__be32                  ieee1588_clk_type;
	__be32                  clr_intx;
} __attribute ( ( packed ) );

enum golan_manage_pages_mode {
	GOLAN_PAGES_CANT_GIVE    = 0,
	GOLAN_PAGES_GIVE         = 1,
	GOLAN_PAGES_TAKE         = 2
};

enum golan_qry_pages_mode {
	GOLAN_BOOT_PAGES	= 0x1,
	GOLAN_INIT_PAGES	= 0x2,
	GOLAN_REG_PAGES		= 0x3,
};

enum {
	GOLAN_REG_PCAP           = 0x5001,
	GOLAN_REG_PMTU           = 0x5003,
	GOLAN_REG_PTYS           = 0x5004,
	GOLAN_REG_PAOS           = 0x5006,
	GOLAN_REG_PMAOS          = 0x5012,
	GOLAN_REG_PUDE           = 0x5009,
	GOLAN_REG_PMPE           = 0x5010,
	GOLAN_REG_PELC           = 0x500e,
	GOLAN_REG_PMLP           = 0, /* TBD */
	GOLAN_REG_NODE_DESC      = 0x6001,
	GOLAN_REG_HOST_ENDIANESS = 0x7004,
};

enum {
	GOLAN_CMD_OP_QUERY_HCA_CAP		= 0x100,
	GOLAN_CMD_OP_QUERY_ADAPTER		= 0x101,
	GOLAN_CMD_OP_INIT_HCA			= 0x102,
	GOLAN_CMD_OP_TEARDOWN_HCA		= 0x103,
	GOLAN_CMD_OP_ENABLE_HCA			= 0x104,
	GOLAN_CMD_OP_DISABLE_HCA		= 0x105,

	GOLAN_CMD_OP_QUERY_PAGES		= 0x107,
	GOLAN_CMD_OP_MANAGE_PAGES		= 0x108,
	GOLAN_CMD_OP_SET_HCA_CAP		= 0x109,

	GOLAN_CMD_OP_CREATE_MKEY		= 0x200,
	GOLAN_CMD_OP_QUERY_MKEY			= 0x201,
	GOLAN_CMD_OP_DESTROY_MKEY		= 0x202,
	GOLAN_CMD_OP_QUERY_SPECIAL_CONTEXTS	= 0x203,

	GOLAN_CMD_OP_CREATE_EQ			= 0x301,
	GOLAN_CMD_OP_DESTROY_EQ			= 0x302,
	GOLAN_CMD_OP_QUERY_EQ			= 0x303,

	GOLAN_CMD_OP_CREATE_CQ			= 0x400,
	GOLAN_CMD_OP_DESTROY_CQ			= 0x401,
	GOLAN_CMD_OP_QUERY_CQ			= 0x402,
	GOLAN_CMD_OP_MODIFY_CQ			= 0x403,

	GOLAN_CMD_OP_CREATE_QP			= 0x500,
	GOLAN_CMD_OP_DESTROY_QP			= 0x501,
	GOLAN_CMD_OP_RST2INIT_QP		= 0x502,
	GOLAN_CMD_OP_INIT2RTR_QP		= 0x503,
	GOLAN_CMD_OP_RTR2RTS_QP			= 0x504,
	GOLAN_CMD_OP_RTS2RTS_QP			= 0x505,
	GOLAN_CMD_OP_SQERR2RTS_QP		= 0x506,
	GOLAN_CMD_OP_2ERR_QP			= 0x507,
	GOLAN_CMD_OP_RTS2SQD_QP			= 0x508,
	GOLAN_CMD_OP_SQD2RTS_QP			= 0x509,
	GOLAN_CMD_OP_2RST_QP			= 0x50a,
	GOLAN_CMD_OP_QUERY_QP			= 0x50b,
	GOLAN_CMD_OP_CONF_SQP			= 0x50c,
	GOLAN_CMD_OP_MAD_IFC			= 0x50d,
	GOLAN_CMD_OP_INIT2INIT_QP		= 0x50e,
	GOLAN_CMD_OP_SUSPEND_QP			= 0x50f,
	GOLAN_CMD_OP_UNSUSPEND_QP		= 0x510,
	GOLAN_CMD_OP_SQD2SQD_QP			= 0x511,
	GOLAN_CMD_OP_ALLOC_QP_COUNTER_SET	= 0x512,
	GOLAN_CMD_OP_DEALLOC_QP_COUNTER_SET	= 0x513,
	GOLAN_CMD_OP_QUERY_QP_COUNTER_SET	= 0x514,

	GOLAN_CMD_OP_CREATE_PSV			= 0x600,
	GOLAN_CMD_OP_DESTROY_PSV		= 0x601,
	GOLAN_CMD_OP_QUERY_PSV			= 0x602,
	GOLAN_CMD_OP_QUERY_SIG_RULE_TABLE	= 0x603,
	GOLAN_CMD_OP_QUERY_BLOCK_SIZE_TABLE	= 0x604,

	GOLAN_CMD_OP_CREATE_SRQ			= 0x700,
	GOLAN_CMD_OP_DESTROY_SRQ		= 0x701,
	GOLAN_CMD_OP_QUERY_SRQ			= 0x702,
	GOLAN_CMD_OP_ARM_RQ			= 0x703,
	GOLAN_CMD_OP_RESIZE_SRQ			= 0x704,

	GOLAN_CMD_OP_QUERY_HCA_VPORT_CONTEXT	= 0x762,
	GOLAN_CMD_OP_QUERY_HCA_VPORT_GID		= 0x764,
	GOLAN_CMD_OP_QUERY_HCA_VPORT_PKEY	= 0x765,

	GOLAN_CMD_OP_ALLOC_PD			= 0x800,
	GOLAN_CMD_OP_DEALLOC_PD			= 0x801,
	GOLAN_CMD_OP_ALLOC_UAR			= 0x802,
	GOLAN_CMD_OP_DEALLOC_UAR		= 0x803,

	GOLAN_CMD_OP_ATTACH_TO_MCG		= 0x806,
	GOLAN_CMD_OP_DETACH_FROM_MCG		= 0x807,


	GOLAN_CMD_OP_ALLOC_XRCD			= 0x80e,
	GOLAN_CMD_OP_DEALLOC_XRCD		= 0x80f,

	GOLAN_CMD_OP_ACCESS_REG			= 0x805,
};

struct golan_inbox_hdr {
	__be16		opcode;
	u8		rsvd[4];
	__be16		opmod;
} __attribute ( ( packed ) );

struct golan_cmd_layout {
	u8		type;
	u8		rsvd0[3];
	__be32		inlen;
	union {
		__be64		in_ptr;
		__be32		in_ptr32[2];
	};
	__be32		in[4];
	__be32		out[4];
	union {
		__be64		out_ptr;
		__be32		out_ptr32[2];
	};
	__be32		outlen;
	u8		token;
	u8		sig;
	u8		rsvd1;
	volatile u8	status_own;
} __attribute ( ( packed ) );

struct golan_outbox_hdr {
	u8		status;
	u8		rsvd[3];
	__be32		syndrome;
} __attribute ( ( packed ) );

enum {
    GOLAN_DEV_CAP_FLAG_RC		= 1LL <<  0,
    GOLAN_DEV_CAP_FLAG_UC		= 1LL <<  1,
    GOLAN_DEV_CAP_FLAG_UD		= 1LL <<  2,
    GOLAN_DEV_CAP_FLAG_XRC		= 1LL <<  3,
    GOLAN_DEV_CAP_FLAG_SRQ		= 1LL <<  6,
    GOLAN_DEV_CAP_FLAG_BAD_PKEY_CNTR	= 1LL <<  8,
    GOLAN_DEV_CAP_FLAG_BAD_QKEY_CNTR	= 1LL <<  9,
    GOLAN_DEV_CAP_FLAG_APM		= 1LL << 17,
    GOLAN_DEV_CAP_FLAG_ATOMIC		= 1LL << 18,
    GOLAN_DEV_CAP_FLAG_ON_DMND_PG	= 1LL << 24,
    GOLAN_DEV_CAP_FLAG_RESIZE_SRQ	= 1LL << 32,
    GOLAN_DEV_CAP_FLAG_REMOTE_FENCE	= 1LL << 38,
    GOLAN_DEV_CAP_FLAG_TLP_HINTS	= 1LL << 39,
    GOLAN_DEV_CAP_FLAG_SIG_HAND_OVER	= 1LL << 40,
    GOLAN_DEV_CAP_FLAG_DCT		= 1LL << 41,
    GOLAN_DEV_CAP_FLAG_CMDIF_CSUM	= 1LL << 46,
};


struct golan_hca_cap {
	u8		rsvd1[16];
	u8		log_max_srq_sz;
	u8		log_max_qp_sz;
	u8		rsvd2;
	u8		log_max_qp;
	u8		log_max_strq_sz;
	u8		log_max_srqs;
	u8		rsvd4[2];
	u8		rsvd5;
	u8		log_max_cq_sz;
	u8		rsvd6;
	u8		log_max_cq;
	u8		log_max_eq_sz;
	u8		log_max_mkey;
	u8		rsvd7;
	u8		log_max_eq;
	u8		max_indirection;
	u8		log_max_mrw_sz;
	u8		log_max_bsf_list_sz;
	u8		log_max_klm_list_sz;
	u8		rsvd_8_0;
	u8		log_max_ra_req_dc;
	u8		rsvd_8_1;
	u8		log_max_ra_res_dc;
	u8		rsvd9;
	u8		log_max_ra_req_qp;
	u8		rsvd10;
	u8		log_max_ra_res_qp;
	u8		rsvd11[4];
	__be16		max_qp_count;
	__be16		pkey_table_size;
	u8		rsvd13;
	u8		local_ca_ack_delay;
	u8		rsvd14;
	u8		num_ports;
	u8		log_max_msg;
	u8		rsvd15[3];
	__be16		stat_rate_support;
	u8		rsvd16[2];
	__be64		flags;
	u8		rsvd17;
	u8		uar_sz;
	u8		rsvd18;
	u8		log_pg_sz;
	__be16		bf_log_bf_reg_size;
	u8		rsvd19[4];
	__be16		max_wqe_sz_sq;
	u8		rsvd20[2];
	__be16		max_wqe_sz_rq;
	u8		rsvd21[2];
	__be16		max_wqe_sz_sq_dc;
	u8		rsvd22[4];
	__be16		max_qp_mcg;
	u8		rsvd23;
	u8		log_max_mcg;
	u8		rsvd24;
	u8		log_max_pd;
	u8		rsvd25;
	u8		log_max_xrcd;
	u8		rsvd26[40];
	__be32		uar_page_sz;
	u8		rsvd27[28];
	u8		log_msx_atomic_size_qp;
	u8		rsvd28[2];
	u8		log_msx_atomic_size_dc;
	u8		rsvd29[76];
} __attribute ( ( packed ) );

struct golan_query_pages_inbox {
	struct golan_inbox_hdr	hdr;
	u8                      rsvd[8];
} __attribute ( ( packed ) );

struct golan_query_pages_outbox {
	struct golan_outbox_hdr hdr;
	u8                  	rsvd[2];
	__be16                  func_id;
	__be32                  num_pages;
} __attribute ( ( packed ) );

struct golan_cmd_query_hca_cap_mbox_in {
	struct golan_inbox_hdr	hdr;
	u8						rsvd[8];
} __attribute ( ( packed ) );

struct golan_cmd_query_hca_cap_mbox_out {
	struct golan_outbox_hdr	hdr;
	u8						rsvd0[8];
	struct golan_hca_cap    hca_cap;
} __attribute ( ( packed ) );

struct golan_cmd_set_hca_cap_mbox_in {
	struct golan_inbox_hdr	hdr;
	u8						rsvd[8];
	struct golan_hca_cap    hca_cap;
} __attribute ( ( packed ) );

struct golan_cmd_set_hca_cap_mbox_out {
	struct golan_outbox_hdr	hdr;
	u8						rsvd0[8];
} __attribute ( ( packed ) );

struct golan_cmd_init_hca_mbox_in {
	struct golan_inbox_hdr	hdr;
	u8						rsvd0[2];
	__be16					profile;
	u8						rsvd1[4];
} __attribute ( ( packed ) );

struct golan_cmd_init_hca_mbox_out {
	struct golan_outbox_hdr	hdr;
	u8						rsvd[8];
} __attribute ( ( packed ) );

enum golan_teardown {
	GOLAN_TEARDOWN_GRACEFUL = 0x0,
	GOLAN_TEARDOWN_PANIC 	= 0x1
};

struct golan_cmd_teardown_hca_mbox_in {
    struct golan_inbox_hdr	hdr;
    u8         				rsvd0[2];
    __be16          		profile;
    u8          			rsvd1[4];
} __attribute ( ( packed ) );

struct golan_cmd_teardown_hca_mbox_out {
    struct golan_outbox_hdr hdr;
    u8          			rsvd[8];
} __attribute ( ( packed ) );

struct golan_enable_hca_mbox_in {
	struct golan_inbox_hdr  hdr;
	u8                      rsvd[8];
} __attribute ( ( packed ) );

struct golan_enable_hca_mbox_out {
	struct golan_outbox_hdr	hdr;
	u8                      rsvd[8];
} __attribute ( ( packed ) );

struct golan_disable_hca_mbox_in {
    struct golan_inbox_hdr  hdr;
    u8          			rsvd[8];
} __attribute ( ( packed ) );

struct golan_disable_hca_mbox_out {
    struct golan_outbox_hdr	hdr;
    u8          			rsvd[8];
} __attribute ( ( packed ) );

struct golan_manage_pages_inbox_data {
	u8                      rsvd2[16];
	__be64                  pas[0];
} __attribute ( ( packed ) );

struct golan_manage_pages_inbox {
	struct golan_inbox_hdr	hdr;
	__be16			rsvd0;
	__be16			func_id;
	__be32			num_entries;
	struct golan_manage_pages_inbox_data 	data;
} __attribute ( ( packed ) );

struct golan_manage_pages_outbox_data {
	__be64                  pas[0];
} __attribute ( ( packed ) );

struct golan_manage_pages_outbox {
	struct golan_outbox_hdr 		hdr;
	__be32                			num_entries;
	__be32					rsrvd;
	struct golan_manage_pages_outbox_data	data;
} __attribute ( ( packed ) );

struct golan_reg_host_endianess {
	u8      he;
	u8      rsvd[15];
} __attribute ( ( packed ) );

struct golan_cmd_prot_block {
	union {
		__be64		data[GOLAN_CMD_PAS_CNT];
		u8          bdata[GOLAN_CMD_DATA_BLOCK_SIZE];
	};
	u8              rsvd0[48];
	__be64          next;
	__be32          block_num;
	u8              rsvd1;
	u8              token;
	u8              ctrl_sig;
	u8              sig;
} __attribute ( ( packed ) );

/* MAD IFC structures */
#define GOLAN_MAD_SIZE						256
#define GOLAN_MAD_IFC_NO_VALIDATION			0x3
#define GOLAN_MAD_IFC_RLID_BIT				16

struct golan_mad_ifc_mbox_in {
    struct golan_inbox_hdr	hdr;
    __be16          		remote_lid;
    u8          			rsvd0;
    u8          			port;
    u8          			rsvd1[4];
    u8 						mad[GOLAN_MAD_SIZE];
} __attribute ( ( packed ) );

struct golan_mad_ifc_mbox_out {
    struct golan_outbox_hdr	hdr;
    u8          			rsvd[8];
    u8 						mad[GOLAN_MAD_SIZE];
} __attribute ( ( packed ) );

/* UAR Structures */
struct golan_alloc_uar_mbox_in {
    struct golan_inbox_hdr  hdr;
    u8                      rsvd[8];
} __attribute ( ( packed ) );

struct golan_alloc_uar_mbox_out {
    struct golan_outbox_hdr hdr;
    __be32                  uarn;
    u8                      rsvd[4];
} __attribute ( ( packed ) );

struct golan_free_uar_mbox_in {
    struct golan_inbox_hdr  hdr;
    __be32                  uarn;
    u8                      rsvd[4];
} __attribute ( ( packed ) );

struct golan_free_uar_mbox_out {
    struct golan_outbox_hdr hdr;
    u8                      rsvd[8];
} __attribute ( ( packed ) );

/* Event Queue Structures */
enum {
    GOLAN_EQ_STATE_ARMED     	= 0x9,
    GOLAN_EQ_STATE_FIRED     	= 0xa,
    GOLAN_EQ_STATE_ALWAYS_ARMED	= 0xb,
};


struct golan_eq_context {
	u8			status;
	u8			ec_oi;
	u8			st;
	u8			rsvd2[7];
	__be16		page_pffset;
	__be32		log_sz_usr_page;
	u8			rsvd3[7];
	u8			intr;
	u8			log_page_size;
	u8			rsvd4[15];
	__be32		consumer_counter;
	__be32		produser_counter;
	u8			rsvd5[16];
} __attribute ( ( packed ) );

struct golan_create_eq_mbox_in_data {
	struct golan_eq_context	ctx;
	u8						rsvd2[8];
	__be64					events_mask;
	u8						rsvd3[176];
	__be64					pas[0];
} __attribute ( ( packed ) );

struct golan_create_eq_mbox_in {
	struct golan_inbox_hdr				hdr;
	u8									rsvd0[3];
	u8									input_eqn;
	u8									rsvd1[4];
	struct golan_create_eq_mbox_in_data data;
} __attribute ( ( packed ) );

struct golan_create_eq_mbox_out {
	struct golan_outbox_hdr	hdr;
	u8						rsvd0[3];
	u8						eq_number;
	u8						rsvd1[4];
} __attribute ( ( packed ) );

struct golan_destroy_eq_mbox_in {
	struct golan_inbox_hdr	hdr;
	u8						rsvd0[3];
	u8						eqn;
	u8						rsvd1[4];
} __attribute ( ( packed ) );

struct golan_destroy_eq_mbox_out {
	struct golan_outbox_hdr	hdr;
	u8						rsvd[8];
} __attribute ( ( packed ) );

/***********************************************/
/************** Query Vport ****************/
struct golan_query_hca_vport_context_inbox {
	struct golan_inbox_hdr	hdr;
	__be16			other_vport	: 1;
	__be16			rsvd1		: 7;
	__be16			port_num		: 4;
	__be16			rsvd2		: 4;
	__be16			vport_number;
	u8			rsvd[4];
} __attribute ( ( packed ) );

struct golan_query_hca_vport_context_data {
	__be32			field_select;
	__be32			rsvd1[7];
	//****
	__be16			sm_virt_aware			: 1;
	__be16			has_smi				: 1;
	__be16			has_raw				: 1;
	__be16			grh_required			: 1;
	__be16			rsvd2				: 12;
	u8			port_physical_state	: 4;
	u8			vport_state_policy	: 4;
	u8			port_state			: 4;
	u8			vport_state			: 4;
	//****
	u8			rsvd3[4];
	//****
	__be32			system_image_guid[2];
	//****
	__be32			port_guid[2];
	//****
	__be32			node_guid[2];
	//****
	__be32			cap_mask1;
	__be32			cap_mask1_field_select;
	__be32			cap_mask2;
	__be32			cap_mask2_field_select;
	u8			rsvd4[16];
	__be16			lid;
	u8			rsvd5				: 4;
	u8			init_type_reply		: 4;
	u8			lmc					: 3;
	u8			subnet_timeout		: 5;
	__be16			sm_lid;
	u8			sm_sl				: 4;
	u8			rsvd6				: 4;
	u8			rsvd7;
	__be16			qkey_violation_counter;
	__be16			pkey_violation_counter;
	u8			rsvd8[100];
} __attribute ( ( packed ) );

struct golan_query_hca_vport_context_outbox {
	struct golan_outbox_hdr	hdr;
	u8			rsvd[8];
	struct golan_query_hca_vport_context_data context_data;
} __attribute ( ( packed ) );

struct golan_query_hca_vport_gid_inbox {
	struct golan_inbox_hdr	hdr;
	u8			other_vport	: 1;
	u8			rsvd1		: 7;
	u8			port_num		: 4;
	u8			rsvd2		: 4;
	__be16			vport_number;
	__be16			rsvd3;
	__be16			gid_index;
} __attribute ( ( packed ) );

struct golan_query_hca_vport_gid_outbox {
	struct golan_outbox_hdr	hdr;
	u8			rsvd0[4];
	__be16			gids_num;
	u8			rsvd1[2];
	__be32 		gid0[4];
} __attribute ( ( packed ) );

struct golan_query_hca_vport_pkey_inbox {
	struct golan_inbox_hdr	hdr;
	u8			other_vport	: 1;
	u8			rsvd1		: 7;
	u8			port_num		: 4;
	u8			rsvd2		: 4;
	__be16			vport_number;
	__be16			rsvd3;
	__be16			pkey_index;
} __attribute ( ( packed ) );

struct golan_query_hca_vport_pkey_data {
	__be16			rsvd1;
	__be16			pkey0;
} __attribute ( ( packed ) );

struct golan_query_hca_vport_pkey_outbox {
	struct golan_outbox_hdr	hdr;
	u8			rsvd[8];
	struct golan_query_hca_vport_pkey_data *pkey_data;
} __attribute ( ( packed ) );

struct golan_eqe_comp {
	__be32	reserved[6];
	__be32	cqn;
} __attribute ( ( packed ) );

struct golan_eqe_qp_srq {
	__be32	reserved[6];
	__be32	qp_srq_n;
} __attribute ( ( packed ) );

struct golan_eqe_cq_err {
	__be32	cqn;
	u8	reserved1[7];
	u8	syndrome;
} __attribute ( ( packed ) );

struct golan_eqe_dropped_packet {
};

struct golan_eqe_port_state {
	u8	reserved0[8];
	u8	port;
} __attribute ( ( packed ) );

struct golan_eqe_gpio {
	__be32	reserved0[2];
	__be64	gpio_event;
} __attribute ( ( packed ) );

struct golan_eqe_congestion {
	u8	type;
	u8	rsvd0;
	u8	congestion_level;
} __attribute ( ( packed ) );

struct golan_eqe_stall_vl {
	u8	rsvd0[3];
	u8	port_vl;
} __attribute ( ( packed ) );

struct golan_eqe_cmd {
	__be32	vector;
	__be32	rsvd[6];
} __attribute ( ( packed ) );

struct golan_eqe_page_req {
	u8		rsvd0[2];
	__be16		func_id;
	u8		rsvd1[2];
	__be16		num_pages;
	__be32		rsvd2[5];
} __attribute ( ( packed ) );

union ev_data {
	__be32				raw[7];
	struct golan_eqe_cmd		cmd;
	struct golan_eqe_comp		comp;
	struct golan_eqe_qp_srq		qp_srq;
	struct golan_eqe_cq_err		cq_err;
	struct golan_eqe_dropped_packet	dp;
	struct golan_eqe_port_state	port;
	struct golan_eqe_gpio		gpio;
	struct golan_eqe_congestion	cong;
	struct golan_eqe_stall_vl	stall_vl;
	struct golan_eqe_page_req	req_pages;
} __attribute__ ((packed));

struct golan_eqe {
	u8				rsvd0;
	u8				type;
	u8				rsvd1;
	u8				sub_type;
	__be32			rsvd2[7];
	union ev_data	data;
	__be16			rsvd3;
	u8				signature;
	u8				owner;
} __attribute__ ((packed));

/* Protection Domain Structures */
struct golan_alloc_pd_mbox_in {
	struct golan_inbox_hdr	hdr;
	u8			rsvd[8];
} __attribute ( ( packed ) );

struct golan_alloc_pd_mbox_out {
	struct golan_outbox_hdr	hdr;
	__be32			pdn;
	u8			rsvd[4];
} __attribute ( ( packed ) );

struct golan_dealloc_pd_mbox_in {
	struct golan_inbox_hdr	hdr;
	__be32			pdn;
	u8			rsvd[4];
} __attribute ( ( packed ) );

struct golan_dealloc_pd_mbox_out {
	struct golan_outbox_hdr	hdr;
	u8			rsvd[8];
} __attribute ( ( packed ) );

/* Memory key structures */
#define GOLAN_IB_ACCESS_LOCAL_READ	(1 << 2)
#define GOLAN_IB_ACCESS_LOCAL_WRITE	(1 << 3)
#define GOLAN_MKEY_LEN64		(1 << 31)
#define GOLAN_CREATE_MKEY_SEG_QPN_BIT	8

struct golan_mkey_seg {
	/*
	 * This is a two bit field occupying bits 31-30.
	 * bit 31 is always 0,
	 * bit 30 is zero for regular MRs and 1 (e.g free) for UMRs that do not have tanslation
	 */
	u8		status;
	u8		pcie_control;
	u8		flags;
	u8		version;
	__be32		qpn_mkey7_0;
	u8		rsvd1[4];
	__be32		flags_pd;
	__be64		start_addr;
	__be64		len;
	__be32		bsfs_octo_size;
	u8		rsvd2[16];
	__be32		xlt_oct_size;
	u8		rsvd3[3];
	u8		log2_page_size;
	u8		rsvd4[4];
} __attribute ( ( packed ) );

struct golan_create_mkey_mbox_in_data {
	struct golan_mkey_seg	seg;
	u8			rsvd1[16];
	__be32			xlat_oct_act_size;
	__be32			bsf_coto_act_size;
	u8			rsvd2[168];
	__be64			pas[0];
} __attribute ( ( packed ) );

struct golan_create_mkey_mbox_in {
	struct golan_inbox_hdr			hdr;
	__be32					input_mkey_index;
	u8					rsvd0[4];
	struct golan_create_mkey_mbox_in_data	data;
} __attribute ( ( packed ) );

struct golan_create_mkey_mbox_out {
	struct golan_outbox_hdr	hdr;
	__be32			mkey;
	u8			rsvd[4];
} __attribute ( ( packed ) );

struct golan_destroy_mkey_mbox_in {
	struct golan_inbox_hdr	hdr;
	__be32			mkey;
	u8			rsvd[4];
} __attribute ( ( packed ) );

struct golan_destroy_mkey_mbox_out {
	struct golan_outbox_hdr	hdr;
	u8			rsvd[8];
} __attribute ( ( packed ) );

/* Completion Queue Structures */
enum {
    GOLAN_CQ_STATE_ARMED     	= 9,
    GOLAN_CQ_STATE_ALWAYS_ARMED	= 0xb,
    GOLAN_CQ_STATE_FIRED    	= 0xa
};

enum {
    GOLAN_CQE_REQ        	= 0,
    GOLAN_CQE_RESP_WR_IMM	= 1,
    GOLAN_CQE_RESP_SEND  	= 2,
    GOLAN_CQE_RESP_SEND_IMM	= 3,
    GOLAN_CQE_RESP_SEND_INV	= 4,
    GOLAN_CQE_RESIZE_CQ		= 0xff, /* TBD */
    GOLAN_CQE_REQ_ERR		= 13,
    GOLAN_CQE_RESP_ERR		= 14
};

struct golan_cq_context {
	u8		status;
	u8		cqe_sz_flags;
	u8		st;
	u8		rsvd3;
	u8		rsvd4[6];
	__be16		page_offset;
	__be32		log_sz_usr_page;
	__be16		cq_period;
	__be16		cq_max_count;
	__be16		rsvd20;
	__be16		c_eqn;
	u8		log_pg_sz;
	u8		rsvd25[7];
	__be32		last_notified_index;
	__be32		solicit_producer_index;
	__be32		consumer_counter;
	__be32		producer_counter;
	u8		rsvd48[8];
	__be64		db_record_addr;
} __attribute ( ( packed ) );


struct golan_create_cq_mbox_in_data	{
	struct golan_cq_context	ctx;
	u8						rsvd6[192];
	__be64					pas[0];
} __attribute ( ( packed ) );

struct golan_create_cq_mbox_in {
	struct golan_inbox_hdr				hdr;
	__be32								input_cqn;
	u8									rsvdx[4];
	struct golan_create_cq_mbox_in_data	data;
} __attribute ( ( packed ) );

struct golan_create_cq_mbox_out {
	struct golan_outbox_hdr	hdr;
	__be32					cqn;
	u8						rsvd0[4];
} __attribute ( ( packed ) );

struct golan_destroy_cq_mbox_in {
	struct golan_inbox_hdr	hdr;
	__be32					cqn;
	u8						rsvd0[4];
} __attribute ( ( packed ) );

struct golan_destroy_cq_mbox_out {
	struct golan_outbox_hdr	hdr;
	u8						rsvd0[8];
} __attribute ( ( packed ) );

struct golan_err_cqe {
	u8		rsvd0[32];
	__be32	srqn;
	u8		rsvd1[16];
	u8		hw_syndrom;
	u8		rsvd2;
	u8		vendor_err_synd;
	u8		syndrome;
	__be32	s_wqe_opcode_qpn;
	__be16	wqe_counter;
	u8		signature;
	u8		op_own;
} __attribute ( ( packed ) );

struct golan_cqe64 {
	u8		rsvd0[17];
	u8		ml_path;
	u8		rsvd20[4];
	__be16	slid;
	__be32	flags_rqpn;
	u8		rsvd28[4];
	__be32	srqn;
	__be32	imm_inval_pkey;
	u8		rsvd40[4];
	__be32	byte_cnt;
	__be64	timestamp;
	__be32	sop_drop_qpn;
	__be16	wqe_counter;
	u8		signature;
	u8		op_own;
} __attribute ( ( packed ) );

/* Queue Pair Structures */
#define GOLAN_QP_CTX_ST_BIT			16
#define GOLAN_QP_CTX_PM_STATE_BIT		11
#define GOLAN_QP_CTX_FRE_BIT			11
#define GOLAN_QP_CTX_RLKY_BIT			4
#define GOLAN_QP_CTX_RQ_SIZE_BIT		3
#define GOLAN_QP_CTX_SQ_SIZE_BIT		11
#define GOLAN_QP_CTX_MTU_BIT			5
#define GOLAN_QP_CTX_ACK_REQ_FREQ_BIT		28

enum {
	GOLAN_QP_CTX_DONT_USE_RSRVD_LKEY	= 0,
	GOLAN_QP_CTX_USE_RSRVD_LKEY		= 1
};

enum {
	GOLAN_IB_ACK_REQ_FREQ			= 8,
};

enum golan_qp_optpar {
	GOLAN_QP_PARAM_ALT_ADDR_PATH		= 1 << 0,
	GOLAN_QP_PARAM_RRE			= 1 << 1,
	GOLAN_QP_PARAM_RAE			= 1 << 2,
	GOLAN_QP_PARAM_RWE			= 1 << 3,
	GOLAN_QP_PARAM_PKEY_INDEX		= 1 << 4,
	GOLAN_QP_PARAM_Q_KEY			= 1 << 5,
	GOLAN_QP_PARAM_RNR_TIMEOUT		= 1 << 6,
	GOLAN_QP_PARAM_PRIMARY_ADDR_PATH	= 1 << 7,
	GOLAN_QP_PARAM_SRA_MAX			= 1 << 8,
	GOLAN_QP_PARAM_RRA_MAX			= 1 << 9,
	GOLAN_QP_PARAM_PM_STATE			= 1 << 10,
	GOLAN_QP_PARAM_RETRY_COUNT		= 1 << 12,
	GOLAN_QP_PARAM_RNR_RETRY		= 1 << 13,
	GOLAN_QP_PARAM_ACK_TIMEOUT		= 1 << 14,
	GOLAN_QP_PARAM_PRI_PORT			= 1 << 16,
	GOLAN_QP_PARAM_SRQN			= 1 << 18,
	GOLAN_QP_PARAM_CQN_RCV			= 1 << 19,
	GOLAN_QP_PARAM_DC_HS			= 1 << 20,
	GOLAN_QP_PARAM_DC_KEY			= 1 << 21
};

#define GOLAN_QP_PARAMS_INIT2RTR_MASK	(GOLAN_QP_PARAM_PKEY_INDEX	|\
					 GOLAN_QP_PARAM_Q_KEY		|\
					 GOLAN_QP_PARAM_RWE		|\
					 GOLAN_QP_PARAM_RRE)

#define GOLAN_QP_PARAMS_RTR2RTS_MASK    (GOLAN_QP_PARAM_PM_STATE	|\
					 GOLAN_QP_PARAM_RNR_TIMEOUT	|\
					 GOLAN_QP_PARAM_Q_KEY		|\
					 GOLAN_QP_PARAM_RWE		|\
					 GOLAN_QP_PARAM_RRE)


enum {
	GOLAN_QP_ST_RC			= 0x0,
	GOLAN_QP_ST_UC			= 0x1,
	GOLAN_QP_ST_UD			= 0x2,
	GOLAN_QP_ST_XRC			= 0x3,
	GOLAN_QP_ST_MLX			= 0x4,
	GOLAN_QP_ST_DC			= 0x5,
	GOLAN_QP_ST_QP0			= 0x7,
	GOLAN_QP_ST_QP1			= 0x8,
	GOLAN_QP_ST_RAW_ETHERTYPE	= 0x9,
	GOLAN_QP_ST_RAW_IPV6		= 0xa,
	GOLAN_QP_ST_SNIFFER		= 0xb,
	GOLAN_QP_ST_SYNC_UMR		= 0xe,
	GOLAN_QP_ST_PTP_1588		= 0xd,
	GOLAN_QP_ST_REG_UMR		= 0xc,
	GOLAN_QP_ST_MAX
};

enum {
	GOLAN_QP_PM_MIGRATED	= 0x3,
	GOLAN_QP_PM_ARMED       = 0x0,
	GOLAN_QP_PM_REARM       = 0x1
};

enum {
	GOLAN_QP_LAT_SENSITIVE	= 1 << 28,
	GOLAN_QP_ENABLE_SIG	= 1 << 31
};


struct golan_qp_db {
	u8		rsvd0[2];
	__be16	recv_db;
	u8		rsvd1[2];
	__be16	send_db;
} __attribute ( ( packed ) );

enum {
	GOLAN_WQE_CTRL_CQ_UPDATE     = 2 << 2, /*Wissam, wtf?*/
	GOLAN_WQE_CTRL_SOLICITED     = 1 << 1
};

struct golan_wqe_ctrl_seg {
	__be32		opmod_idx_opcode;
	__be32		qpn_ds;
	u8			signature;
	u8			rsvd[2];
	u8			fm_ce_se;
	__be32		imm;
} __attribute ( ( packed ) );

struct golan_av {
	union {
		struct {
			__be32	qkey;
			__be32	reserved;
		} qkey;
		__be64	dc_key;
	} key;
	__be32	dqp_dct;
	u8		stat_rate_sl;
	u8		fl_mlid;
	__be16	rlid;
	u8		reserved0[10];
	u8		tclass;
	u8		hop_limit;
	__be32	grh_gid_fl;
	u8		rgid[16];
} __attribute ( ( packed ) );

struct golan_wqe_data_seg {
	__be32	byte_count;
	__be32	lkey;
	__be64	addr;
} __attribute ( ( packed ) );

struct golan_wqe_signature_seg {
	u8	rsvd0[4];
	u8	signature;
	u8	rsvd1[11];
} __attribute ( ( packed ) );

struct golan_wqe_inline_seg {
	__be32	byte_count;
} __attribute ( ( packed ) );

struct golan_qp_path {
	u8			fl;
	u8			rsvd3;
	u8			free_ar;
	u8			pkey_index;
	u8			rsvd0;
	u8			grh_mlid;
	__be16		rlid;
	u8			ackto_lt;
	u8			mgid_index;
	u8			static_rate;
	u8			hop_limit;
	__be32		tclass_flowlabel;
	u8			rgid[16];
	u8			rsvd1[4];
	u8			sl;
	u8			port;
	u8			rsvd2[6];
} __attribute ( ( packed ) );

struct golan_qp_context {
	__be32			flags;
	__be32			flags_pd;
	u8			mtu_msgmax;
	u8			rq_size_stride;
	__be16			sq_crq_size;
	__be32			qp_counter_set_usr_page;
	__be32			wire_qpn;
	__be32			log_pg_sz_remote_qpn;
	struct			golan_qp_path pri_path;
	struct			golan_qp_path alt_path;
	__be32			params1;
	u8			reserved2[4];
	__be32			next_send_psn;
	__be32			cqn_send;
	u8			reserved3[8];
	__be32			last_acked_psn;
	__be32			ssn;
	__be32			params2;
	__be32			rnr_nextrecvpsn;
	__be32			xrcd;
	__be32			cqn_recv;
	__be64			db_rec_addr;
	__be32			qkey;
	__be32			rq_type_srqn;
	__be32			rmsn;
	__be16			hw_sq_wqe_counter;
	__be16			sw_sq_wqe_counter;
	__be16			hw_rcyclic_byte_counter;
	__be16			hw_rq_counter;
	__be16			sw_rcyclic_byte_counter;
	__be16			sw_rq_counter;
	u8			rsvd0[5];
	u8			cgs;
	u8			cs_req;
	u8			cs_res;
	__be64			dc_access_key;
	u8			rsvd1[24];
} __attribute ( ( packed ) );

struct golan_create_qp_mbox_in_data {
	__be32				opt_param_mask;
	u8				rsvd1[4];
	struct golan_qp_context		ctx;
	u8				rsvd3[16];
	__be64				pas[0];
} __attribute ( ( packed ) );

struct golan_create_qp_mbox_in {
	struct golan_inbox_hdr			hdr;
	__be32					input_qpn;
	u8					rsvd0[4];
	struct golan_create_qp_mbox_in_data	data;
} __attribute ( ( packed ) );

struct golan_create_qp_mbox_out {
	struct golan_outbox_hdr	hdr;
	__be32			qpn;
	u8			rsvd0[4];
} __attribute ( ( packed ) );

struct golan_destroy_qp_mbox_in {
	struct golan_inbox_hdr	hdr;
	__be32			qpn;
	u8			rsvd0[4];
} __attribute ( ( packed ) );

struct golan_destroy_qp_mbox_out {
	struct golan_outbox_hdr	hdr;
	u8			rsvd0[8];
} __attribute ( ( packed ) );

struct golan_modify_qp_mbox_in_data {
	__be32			optparam;
	u8			rsvd0[4];
	struct golan_qp_context	ctx;
} __attribute ( ( packed ) );

struct golan_modify_qp_mbox_in {
	struct golan_inbox_hdr		hdr;
	__be32				qpn;
	u8				rsvd1[4];
	struct golan_modify_qp_mbox_in_data	data;
} __attribute ( ( packed ) );

struct golan_modify_qp_mbox_out {
	struct golan_outbox_hdr		hdr;
	u8				rsvd0[8];
} __attribute ( ( packed ) );

struct golan_attach_mcg_mbox_in {
    struct golan_inbox_hdr	hdr;
    __be32          		qpn;
    __be32          		rsvd;
    u8          		gid[16];
} __attribute ( ( packed ) );

struct golan_attach_mcg_mbox_out {
    struct golan_outbox_hdr	hdr;
    u8          		rsvf[8];
} __attribute ( ( packed ) );

struct golan_detach_mcg_mbox_in {
    struct golan_inbox_hdr	hdr;
    __be32         		qpn;
    __be32          		rsvd;
    u8          		gid[16];
} __attribute ( ( packed ) );

struct golan_detach_mcg_mbox_out {
    struct golan_outbox_hdr	hdr;
    u8          			rsvf[8];
} __attribute ( ( packed ) );


#define MAILBOX_SIZE   sizeof(struct golan_cmd_prot_block)

#endif /* __CIB_PRM__ */

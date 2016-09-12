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

#ifndef SRC_DRIVERS_INFINIBAND_MLX_NODNIC_INCLUDE_PRM_NODNIC_SHOMRON_PRM_H_
#define SRC_DRIVERS_INFINIBAND_MLX_NODNIC_INCLUDE_PRM_NODNIC_SHOMRON_PRM_H_



#include "nodnic_prm.h"


#define SHOMRON_MAX_GATHER 1

/* Send wqe segment ctrl */

struct shomronprm_wqe_segment_ctrl_send_st {	/* Little Endian */
    pseudo_bit_t	opcode[0x00008];
    pseudo_bit_t	wqe_index[0x00010];
    pseudo_bit_t	reserved1[0x00008];
/* -------------- */
    pseudo_bit_t	ds[0x00006];           /* descriptor (wqe) size in 16bytes chunk */
    pseudo_bit_t	reserved2[0x00002];
    pseudo_bit_t	qpn[0x00018];
/* -------------- */
	pseudo_bit_t	reserved3[0x00002];
	pseudo_bit_t	ce[0x00002];
	pseudo_bit_t	reserved4[0x0001c];
/* -------------- */
	pseudo_bit_t	reserved5[0x00040];
/* -------------- */
	pseudo_bit_t	mss[0x0000e];
	pseudo_bit_t	reserved6[0x0000e];
	pseudo_bit_t	cs13_inner[0x00001];
	pseudo_bit_t	cs14_inner[0x00001];
	pseudo_bit_t	cs13[0x00001];
	pseudo_bit_t	cs14[0x00001];
/* -------------- */
	pseudo_bit_t	reserved7[0x00020];
/* -------------- */
	pseudo_bit_t	inline_headers1[0x00010];
	pseudo_bit_t	inline_headers_size[0x0000a]; //sum size of inline_hdr1+inline_hdrs (0x10)
	pseudo_bit_t	reserved8[0x00006];
/* -------------- */
	pseudo_bit_t	inline_headers2[0x00020];
/* -------------- */
	pseudo_bit_t	inline_headers3[0x00020];
/* -------------- */
	pseudo_bit_t	inline_headers4[0x00020];
/* -------------- */
	pseudo_bit_t	inline_headers5[0x00020];
};



/* Completion Queue Entry Format        #### michal - fixed by gdror */

struct shomronprm_completion_queue_entry_st {	/* Little Endian */

	pseudo_bit_t	reserved1[0x00080];
/* -------------- */
	pseudo_bit_t	reserved2[0x00010];
	pseudo_bit_t	ml_path[0x00007];
	pseudo_bit_t	reserved3[0x00009];
/* -------------- */
	pseudo_bit_t	slid[0x00010];
	pseudo_bit_t	reserved4[0x00010];
/* -------------- */
	pseudo_bit_t	rqpn[0x00018];
	pseudo_bit_t	sl[0x00004];
	pseudo_bit_t	l3_hdr[0x00002];
	pseudo_bit_t	reserved5[0x00002];
/* -------------- */
	pseudo_bit_t	reserved10[0x00020];
/* -------------- */
	pseudo_bit_t	srqn[0x00018];
	pseudo_bit_t	reserved11[0x0008];
/* -------------- */
	pseudo_bit_t	pkey_index[0x00020];
/* -------------- */
	pseudo_bit_t	reserved6[0x00020];
/* -------------- */
	pseudo_bit_t	byte_cnt[0x00020];
/* -------------- */
	pseudo_bit_t	reserved7[0x00040];
/* -------------- */
	pseudo_bit_t	qpn[0x00018];
	pseudo_bit_t	rx_drop_counter[0x00008];
/* -------------- */
	pseudo_bit_t	owner[0x00001];
	pseudo_bit_t	reserved8[0x00003];
	pseudo_bit_t	opcode[0x00004];
	pseudo_bit_t	reserved9[0x00008];
	pseudo_bit_t	wqe_counter[0x00010];
};


/* Completion with Error CQE             #### michal - gdror fixed */

struct shomronprm_completion_with_error_st {	/* Little Endian */
	pseudo_bit_t	reserved1[0x001a0];
	/* -------------- */
	pseudo_bit_t	syndrome[0x00008];
	pseudo_bit_t	vendor_error_syndrome[0x00008];
	pseudo_bit_t	reserved2[0x00010];
	/* -------------- */
	pseudo_bit_t	reserved3[0x00040];
};


struct MLX_DECLARE_STRUCT ( shomronprm_wqe_segment_ctrl_send );
struct MLX_DECLARE_STRUCT ( shomronprm_completion_queue_entry );
struct MLX_DECLARE_STRUCT ( shomronprm_completion_with_error );

struct shomron_nodnic_eth_send_wqe {
	struct shomronprm_wqe_segment_ctrl_send ctrl;
	struct nodnic_wqe_segment_data_ptr data[SHOMRON_MAX_GATHER];
} __attribute__ (( packed ));

union shomronprm_completion_entry {
	struct shomronprm_completion_queue_entry normal;
	struct shomronprm_completion_with_error error;
} __attribute__ (( packed ));


#endif /* SRC_DRIVERS_INFINIBAND_MLX_NODNIC_INCLUDE_PRM_NODNIC_SHOMRON_PRM_H_ */

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

#ifndef SRC_DRIVERS_INFINIBAND_MLX_NODNIC_INCLUDE_PRM_NODNIC_PRM_H_
#define SRC_DRIVERS_INFINIBAND_MLX_NODNIC_INCLUDE_PRM_NODNIC_PRM_H_

#include "mlx_bitops.h"

struct nodnic_wqe_segment_data_ptr_st {	/* Little Endian */
    pseudo_bit_t	byte_count[0x0001f];
    pseudo_bit_t	always0[0x00001];
/* -------------- */
    pseudo_bit_t	l_key[0x00020];
/* -------------- */
    pseudo_bit_t	local_address_h[0x00020];
/* -------------- */
    pseudo_bit_t	local_address_l[0x00020];
/* -------------- */
};

struct MLX_DECLARE_STRUCT ( nodnic_wqe_segment_data_ptr );

#define HERMON_MAX_SCATTER 1

struct nodnic_recv_wqe {
	struct nodnic_wqe_segment_data_ptr data[HERMON_MAX_SCATTER];
} __attribute__ (( packed ));

#endif /* SRC_DRIVERS_INFINIBAND_MLX_NODNIC_INCLUDE_PRM_NODNIC_PRM_H_ */

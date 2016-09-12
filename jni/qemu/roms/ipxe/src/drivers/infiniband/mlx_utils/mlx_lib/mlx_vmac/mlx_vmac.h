#ifndef MLX_VMAC_H_
#define MLX_VMAC_H_

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

#include "../../include/public/mlx_utils.h"

struct mlx_vmac_query_virt_mac {
	mlx_uint32 reserved0	:30;
	mlx_uint32 mac_aux_v	:1;
	mlx_uint32 virtual_mac_en	:1;
	mlx_uint32 parmanent_mac_high	:16;
	mlx_uint32 reserved1	:16;
	mlx_uint32 parmanent_mac_low	:32;
	mlx_uint32 virtual_mac_high	:16;
	mlx_uint32 Reserved2	:16;
	mlx_uint32 virtual_mac_low	:32;
};

struct mlx_vmac_set_virt_mac {
	mlx_uint32 Reserved0	:30;
	mlx_uint32 mac_aux_v	:1;
	mlx_uint32 virtual_mac_en	:1;
	mlx_uint32 reserved1	:32;
	mlx_uint32 reserved2	:32;
	mlx_uint32 virtual_mac_high;
	mlx_uint32 virtual_mac_low;
};

mlx_status
mlx_vmac_query_virt_mac (
	IN mlx_utils *utils,
	OUT struct mlx_vmac_query_virt_mac *virt_mac
	);

mlx_status
mlx_vmac_set_virt_mac (
	IN mlx_utils *utils,
	OUT struct mlx_vmac_set_virt_mac *virt_mac
	);
#endif /* MLX_VMAC_H_ */

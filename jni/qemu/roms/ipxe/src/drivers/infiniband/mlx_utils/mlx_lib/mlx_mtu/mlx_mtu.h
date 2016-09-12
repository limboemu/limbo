#ifndef MLX_MTU_H_
#define MLX_MTU_H_

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

#include "mlx_reg_access.h"
#include "mlx_utils.h"

#define BYTE_TO_BIT	0x8

struct mlx_mtu {
	mlx_uint32 reserved1	:16;
	mlx_uint32 local_port	:8;
	mlx_uint32 reserved2	:8;
	/* -------------- */
	mlx_uint32 reserved3	:16;
	mlx_uint32 max_mtu		:16;
	/* -------------- */
	mlx_uint32 reserved4	:16;
	mlx_uint32 admin_mtu	:16;
	/* -------------- */
	mlx_uint32 reserved5	:16;
	mlx_uint32 oper_mtu		:16;
};

mlx_status
mlx_get_max_mtu(
		IN mlx_utils 	*utils,
		IN mlx_uint8 	port_num,
		OUT mlx_uint32 	*max_mtu
		);

#endif /* MLX_MTU_H_ */

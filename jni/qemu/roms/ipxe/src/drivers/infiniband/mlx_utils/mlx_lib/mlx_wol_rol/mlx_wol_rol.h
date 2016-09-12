#ifndef MLX_WOL_ROL_H_
#define MLX_WOL_ROL_H_

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


#include "mlx_utils.h"

typedef enum {
	WOL_MODE_DISABLE = 0x0,
	WOL_MODE_SECURE = 0x2,
	WOL_MODE_MAGIC = 0x4,
	WOL_MODE_ARP = 0x8,
	WOL_MODE_BC = 0x10,
	WOL_MODE_MC = 0x20,
	WOL_MODE_UC = 0x40,
	WOL_MODE_PHY = 0x80,
} WOL_MODE;

struct mlx_wol_rol {
	mlx_uint32 reserved0	:32;
	mlx_uint32 reserved1	:32;
	mlx_uint32 wol_mode		:8;
	mlx_uint32 rol_mode		:8;
	mlx_uint32 reserved3	:14;
	mlx_uint32 wol_mode_valid	:1;
	mlx_uint32 rol_mode_valid	:1;
};

mlx_status
mlx_set_wol (
	IN mlx_utils *utils,
	IN mlx_uint8 wol_mask
	);

mlx_status
mlx_query_wol (
	IN mlx_utils *utils,
	OUT mlx_uint8 *wol_mask
	);

#endif /* MLX_WOL_ROL_H_ */

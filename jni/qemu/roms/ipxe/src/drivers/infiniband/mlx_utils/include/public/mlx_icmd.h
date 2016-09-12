#ifndef MLXUTILS_INCLUDE_PUBLIC_MLX_ICMD_H_
#define MLXUTILS_INCLUDE_PUBLIC_MLX_ICMD_H_
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

#define MLX_ICMD_MB_ADDR 0x100000
#define MLX_ICMD_MB_SIZE_ADDR 0x1000
#define MLX_ICMD_CTRL_ADDR 0x0

#define MLX_ICMD_SEMAPHORE_ADDR 0x0

#define MLX_ICMD_SEMAPHORE_ID 1234

enum {
	FLASH_REG_ACCESS	= 0x9001,
	GET_FW_INFO			= 0x8007,
	QUERY_VIRTUAL_MAC	= 0x9003,
	SET_VIRTUAL_MAC		= 0x9004,
	QUERY_WOL_ROL		= 0x9005,
	SET_WOL_ROL			= 0x9006,
	OCBB_INIT			= 0x9007,
	OCBB_QUERY_HEADER_STATUS	= 0x9008,
	OCBB_QUERY_ETOC_STATUS	= 0x9009,
	OCBB_QUERY_SET_EVENT	= 0x900A,
	OCSD_INIT			= 0xf004,
};

struct mlx_icmd_ocsd {
	mlx_uint32 reserved;
	mlx_uint64 address;
};

mlx_status
mlx_icmd_send_command(
				IN mlx_utils *utils,
				IN  mlx_uint16 opcode,
				IN OUT mlx_void* data,
				IN mlx_uint32 write_data_size,
				IN mlx_uint32 read_data_size
				);

#endif /* MLXUTILS_INCLUDE_PUBLIC_MLX_ICMD_H_ */

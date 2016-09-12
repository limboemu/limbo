#ifndef MLX_OCBB_H_
#define MLX_OCBB_H_

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

#define MLX_OCBB_EVENT_DATA_SIZE 2
struct mlx_ocbb_init {
	mlx_uint32 address_hi;
	mlx_uint32 address_lo;
};

struct mlx_ocbb_query_status {
	mlx_uint32 reserved	:24;
	mlx_uint32 status	:8;
};

struct mlx_ocbb_set_event {
	mlx_uint64 event_data;
	mlx_uint32 event_number	:8;
	mlx_uint32 event_length	:8;
	mlx_uint32 data_length	:8;
	mlx_uint32 data_start_offset	:8;
};

mlx_status
mlx_ocbb_init (
	IN mlx_utils *utils,
	IN mlx_uint64 address
	);

mlx_status
mlx_ocbb_query_header_status (
	IN mlx_utils *utils,
	OUT mlx_uint8 *ocbb_status
	);

mlx_status
mlx_ocbb_query_etoc_status (
	IN mlx_utils *utils,
	OUT mlx_uint8 *ocbb_status
	);

mlx_status
mlx_ocbb_set_event (
	IN mlx_utils *utils,
	IN mlx_uint64			EventData,
	IN mlx_uint8			EventNumber,
	IN mlx_uint8			EventLength,
	IN mlx_uint8			DataLength,
	IN mlx_uint8			DataStartOffset
	);
#endif /* MLX_OCBB_H_ */

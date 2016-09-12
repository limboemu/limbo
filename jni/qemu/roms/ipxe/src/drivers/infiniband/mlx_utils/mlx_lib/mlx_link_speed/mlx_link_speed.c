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

#include "../../mlx_lib/mlx_link_speed/mlx_link_speed.h"
#include "../../include/public/mlx_memory.h"
#include "../../include/public/mlx_bail.h"

mlx_status
mlx_set_link_speed(
		IN mlx_utils *utils,
		IN mlx_uint8 port_num,
		IN LINK_SPEED_TYPE type,
		IN LINK_SPEED speed
		)
{
	mlx_status status = MLX_SUCCESS;
	struct mlx_link_speed link_speed;
	mlx_uint32 reg_status;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	mlx_memory_set(utils, &link_speed, 0, sizeof(link_speed));

	link_speed.loacl_port = port_num;
	link_speed.proto_mask = 1 << type;

	status = mlx_reg_access(utils, REG_ID_PTYS, REG_ACCESS_READ, &link_speed,
			sizeof(link_speed), &reg_status);

	MLX_CHECK_STATUS(utils, status, reg_err, "mlx_reg_access failed ");
	if (reg_status != 0) {
		MLX_DEBUG_ERROR(utils,"mlx_reg_access failed with status = %d\n", reg_status);
		status = MLX_FAILED;
		goto reg_err;
	}
	switch (speed) {
	case LINK_SPEED_1GB:
		link_speed.eth_proto_admin = link_speed.eth_proto_capability & LINK_SPEED_1GB_MASK;
		break;
	case LINK_SPEED_10GB:
		link_speed.eth_proto_admin = link_speed.eth_proto_capability & LINK_SPEED_10GB_MASK;
		break;
	case LINK_SPEED_40GB:
		link_speed.eth_proto_admin = link_speed.eth_proto_capability & LINK_SPEED_40GB_MASK;
		break;
	case LINK_SPEED_100GB:
		link_speed.eth_proto_admin = link_speed.eth_proto_capability & LINK_SPEED_100GB_MASK;
		break;
	case LINK_SPEED_SDR:
		link_speed.ib_proto_admin = link_speed.ib_proto_capability & LINK_SPEED_SDR_MASK;
		break;
	case LINK_SPEED_DEFAULT:
		if (type == LINK_SPEED_ETH) {
			link_speed.eth_proto_admin = link_speed.eth_proto_capability;
		} else {
			link_speed.ib_proto_admin = link_speed.ib_proto_capability;
		}
		break;
	}
	status = mlx_reg_access(utils, REG_ID_PTYS, REG_ACCESS_WRITE, &link_speed,
				sizeof(link_speed), &reg_status);
	MLX_CHECK_STATUS(utils, status, reg_err, "mlx_reg_access failed ");
	if (reg_status != 0) {
		MLX_DEBUG_ERROR(utils,"mlx_reg_access failed with status = %d\n", reg_status);
		status = MLX_FAILED;
		goto reg_err;
	}
reg_err:
bad_param:
	return status;
}

mlx_status
mlx_get_max_speed(
		IN mlx_utils *utils,
		IN mlx_uint8 port_num,
		IN LINK_SPEED_TYPE type,
		OUT mlx_uint64 *speed
		)
{
	mlx_status status = MLX_SUCCESS;
	struct mlx_link_speed link_speed;
	mlx_uint32 reg_status;
	mlx_uint64 speed_giga = 0;
	mlx_uint8  lanes_number = 1;

	*speed = 0;
	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	mlx_memory_set(utils, &link_speed, 0, sizeof(link_speed));

	link_speed.loacl_port = port_num;
	link_speed.proto_mask = 1 << type;

	status = mlx_reg_access(utils, REG_ID_PTYS, REG_ACCESS_READ, &link_speed,
			sizeof(link_speed), &reg_status);
	MLX_CHECK_STATUS(utils, status, reg_err, "mlx_reg_access failed ");
	if (reg_status != 0) {
		MLX_DEBUG_ERROR(utils,"mlx_reg_access failed with status = %d\n", reg_status);
		status = MLX_FAILED;
		goto reg_err;
	}

	if ( type == LINK_SPEED_ETH ) {
		if ( link_speed.eth_proto_capability & LINK_SPEED_100GB_MASK ) {
			speed_giga = 100;
		} else if ( link_speed.eth_proto_capability & LINK_SPEED_56GB_MASK ) {
			speed_giga = 56;
		} else if ( link_speed.eth_proto_capability & LINK_SPEED_50GB_MASK ) {
			speed_giga = 50;
		} else if ( link_speed.eth_proto_capability & LINK_SPEED_40GB_MASK ) {
			speed_giga = 40;
		} else if (link_speed.eth_proto_capability & LINK_SPEED_25GB_MASK) {
			speed_giga = 25;
		} else if ( link_speed.eth_proto_capability & LINK_SPEED_20GB_MASK ) {
			speed_giga = 20;
		} else if ( link_speed.eth_proto_capability & LINK_SPEED_10GB_MASK) {
			speed_giga = 10;
		} else if ( link_speed.eth_proto_capability & LINK_SPEED_1GB_MASK ) {
			speed_giga = 1;
		}
	} else {
		if ( link_speed.ib_proto_capability & LINK_SPEED_EDR_MASK ) {
			speed_giga = 25;
		} else if ( link_speed.ib_proto_capability & LINK_SPEED_EDR20_MASK ) {
			speed_giga = 20;
		} else if ( link_speed.ib_proto_capability & LINK_SPEED_FDR_MASK ) {
			speed_giga = 14;
		} else if ( link_speed.ib_proto_capability & LINK_SPEED_QDR_MASK ) {
			speed_giga = 10;
		} else if ( link_speed.ib_proto_capability & LINK_SPEED_DDR_MASK ) {
			speed_giga = 5;
		} else if ( link_speed.ib_proto_capability & LINK_SPEED_SDR_MASK ) {
			speed_giga = 2.5;
		}
		if ( link_speed.ib_link_width_capability & LINK_SPEED_WITDH_12_MASK ) {
			lanes_number = 12;
		} else if ( link_speed.ib_link_width_capability & LINK_SPEED_WITDH_8_MASK ) {
			lanes_number = 8;
		} else if (link_speed.ib_link_width_capability & LINK_SPEED_WITDH_4_MASK ) {
			lanes_number = 4;
		} else if (link_speed.ib_link_width_capability & LINK_SPEED_WITDH_2_MASK ) {
			lanes_number = 2;
		} else if (link_speed.ib_link_width_capability & LINK_SPEED_WITDH_1_MASK ) {
			lanes_number = 1;
		}
		speed_giga = speed_giga * lanes_number;
	}
	// Return data in bits
	*speed = speed_giga * GIGA_TO_BIT;
reg_err:
bad_param:
	return status;
}



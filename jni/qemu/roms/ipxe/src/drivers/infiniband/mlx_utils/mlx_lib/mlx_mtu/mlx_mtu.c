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

#include "mlx_mtu.h"
#include "mlx_memory.h"
#include "mlx_bail.h"

mlx_status
mlx_get_max_mtu(
		IN mlx_utils 	*utils,
		IN mlx_uint8 	port_num,
		OUT mlx_uint32 	*max_mtu
		)
{
	mlx_status status = MLX_SUCCESS;
	struct mlx_mtu mtu;
	mlx_uint32 reg_status;
	*max_mtu = 0;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	mlx_memory_set(utils, &mtu, 0, sizeof(mtu));

	mtu.local_port = port_num;

	status = mlx_reg_access(utils, REG_ID_PMTU, REG_ACCESS_READ, &mtu,
			sizeof(mtu), &reg_status);
	MLX_CHECK_STATUS(utils, status, reg_err, "mlx_reg_access failed ");
	if (reg_status != 0) {
		MLX_DEBUG_ERROR(utils,"mlx_reg_access failed with status = %d\n", reg_status);
		status = MLX_FAILED;
		goto reg_err;
	}
	// Return data in bits
	*max_mtu = mtu.max_mtu * BYTE_TO_BIT;
reg_err:
bad_param:
	return status;
}

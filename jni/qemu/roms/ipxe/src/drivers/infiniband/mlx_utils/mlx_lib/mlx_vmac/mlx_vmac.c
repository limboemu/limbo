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

#include "../../mlx_lib/mlx_vmac/mlx_vmac.h"
#include "../../include/public/mlx_icmd.h"
#include "../../include/public/mlx_bail.h"

mlx_status
mlx_vmac_query_virt_mac (
	IN mlx_utils *utils,
	OUT struct mlx_vmac_query_virt_mac *virt_mac
	)
{
	mlx_status status = MLX_SUCCESS;
	if (utils == NULL || virt_mac == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = mlx_icmd_send_command(
			utils,
			QUERY_VIRTUAL_MAC,
			virt_mac,
			0,
			sizeof(*virt_mac)
			);
	MLX_CHECK_STATUS(utils, status, icmd_err, "mlx_icmd_send_command failed");
icmd_err:
bad_param:
	return status;
}

mlx_status
mlx_vmac_set_virt_mac (
	IN mlx_utils *utils,
	OUT struct mlx_vmac_set_virt_mac *virt_mac
	)
{
	mlx_status status = MLX_SUCCESS;
	if (utils == NULL || virt_mac == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = mlx_icmd_send_command(
			utils,
			SET_VIRTUAL_MAC,
			virt_mac,
			sizeof(*virt_mac),
			0
			);
	MLX_CHECK_STATUS(utils, status, icmd_err, "mlx_icmd_send_command failed");
icmd_err:
bad_param:
	return status;
}

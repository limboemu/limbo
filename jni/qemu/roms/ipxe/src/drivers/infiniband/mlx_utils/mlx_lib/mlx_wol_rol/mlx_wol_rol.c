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

#include "mlx_wol_rol.h"
#include "mlx_icmd.h"
#include "mlx_memory.h"
#include "mlx_bail.h"

mlx_status
mlx_set_wol (
	IN mlx_utils *utils,
	IN mlx_uint8 wol_mask
	)
{
	mlx_status status = MLX_SUCCESS;
	struct mlx_wol_rol wol_rol;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	mlx_memory_set(utils, &wol_rol, 0, sizeof(wol_rol));
	wol_rol.wol_mode_valid = TRUE;
	wol_rol.wol_mode = wol_mask;
	status = mlx_icmd_send_command(
			utils,
			SET_WOL_ROL,
			&wol_rol,
			sizeof(wol_rol),
			0
			);
	MLX_CHECK_STATUS(utils, status, icmd_err, "mlx_icmd_send_command failed");
icmd_err:
bad_param:
	return status;
}

mlx_status
mlx_query_wol (
	IN mlx_utils *utils,
	OUT mlx_uint8 *wol_mask
	)
{
	mlx_status status = MLX_SUCCESS;
	struct mlx_wol_rol wol_rol;

	if (utils == NULL || wol_mask == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	mlx_memory_set(utils, &wol_rol, 0, sizeof(wol_rol));
	status = mlx_icmd_send_command(
			utils,
			QUERY_WOL_ROL,
			&wol_rol,
			0,
			sizeof(wol_rol)
			);
	MLX_CHECK_STATUS(utils, status, icmd_err, "mlx_icmd_send_command failed");
	*wol_mask = wol_rol.wol_mode;
icmd_err:
bad_param:
	return status;
}

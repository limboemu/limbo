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

#include "../../mlx_lib/mlx_blink_leds/mlx_blink_leds.h"
#include "../../include/public/mlx_memory.h"
#include "../../include/public/mlx_bail.h"

mlx_status
mlx_blink_leds(
		IN mlx_utils *utils,
		IN mlx_uint16 secs
		)
{
	mlx_status status = MLX_SUCCESS;
	struct mlx_led_control led_control;
	mlx_uint32 reg_status;

	if (utils == NULL ) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	mlx_memory_set(utils, &led_control, 0, sizeof(led_control));
	led_control.beacon_duration = secs;
	status = mlx_reg_access(utils, REG_ID_MLCR, REG_ACCESS_WRITE, &led_control, sizeof(led_control),
			&reg_status);
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


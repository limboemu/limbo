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

#include "mlx_ocbb.h"
#include "mlx_icmd.h"
#include "mlx_bail.h"

mlx_status
mlx_ocbb_init (
	IN mlx_utils *utils,
	IN mlx_uint64 address
	)
{
	mlx_status status = MLX_SUCCESS;
	struct mlx_ocbb_init ocbb_init;
	ocbb_init.address_hi = (mlx_uint32)(address >> 32);
	ocbb_init.address_lo = (mlx_uint32)address;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = mlx_icmd_send_command(
			utils,
			OCBB_INIT,
			&ocbb_init,
			sizeof(ocbb_init),
			0
			);
	MLX_CHECK_STATUS(utils, status, icmd_err, "mlx_icmd_send_command failed");
icmd_err:
bad_param:
	return status;
}

mlx_status
mlx_ocbb_query_header_status (
	IN mlx_utils *utils,
	OUT mlx_uint8 *ocbb_status
	)
{
	mlx_status status = MLX_SUCCESS;
	struct mlx_ocbb_query_status ocbb_query_status;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = mlx_icmd_send_command(
			utils,
			OCBB_QUERY_HEADER_STATUS,
			&ocbb_query_status,
			0,
			sizeof(ocbb_query_status)
			);
	MLX_CHECK_STATUS(utils, status, icmd_err, "mlx_icmd_send_command failed");
	*ocbb_status = ocbb_query_status.status;
icmd_err:
bad_param:
	return status;
}

mlx_status
mlx_ocbb_query_etoc_status (
	IN mlx_utils *utils,
	OUT mlx_uint8 *ocbb_status
	)
{
	mlx_status status = MLX_SUCCESS;
	struct mlx_ocbb_query_status ocbb_query_status;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = mlx_icmd_send_command(
			utils,
			OCBB_QUERY_ETOC_STATUS,
			&ocbb_query_status,
			0,
			sizeof(ocbb_query_status)
			);
	MLX_CHECK_STATUS(utils, status, icmd_err, "mlx_icmd_send_command failed");
	*ocbb_status = ocbb_query_status.status;
icmd_err:
bad_param:
	return status;
}

mlx_status
mlx_ocbb_set_event (
	IN mlx_utils *utils,
	IN mlx_uint64			event_data,
	IN mlx_uint8			event_number,
	IN mlx_uint8			event_length,
	IN mlx_uint8			data_length,
	IN mlx_uint8			data_start_offset
	)
{
	mlx_status status = MLX_SUCCESS;
	struct mlx_ocbb_set_event ocbb_event;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	ocbb_event.data_length = data_length;
	ocbb_event.data_start_offset = data_start_offset;
	ocbb_event.event_number = event_number;
	ocbb_event.event_data = event_data;
	ocbb_event.event_length = event_length;
	status = mlx_icmd_send_command(
			utils,
			OCBB_QUERY_SET_EVENT,
			&ocbb_event,
			sizeof(ocbb_event),
			0
			);
	MLX_CHECK_STATUS(utils, status, icmd_err, "mlx_icmd_send_command failed");
icmd_err:
bad_param:
	return status;
}

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

#include "../../include/public/mlx_bail.h"
#include "../../include/public/mlx_icmd.h"
#include "../../include/public/mlx_pci_gw.h"
#include "../../include/public/mlx_utils.h"

static
mlx_status
mlx_icmd_get_semaphore(
				IN mlx_utils *utils
				)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 retries = 0;
	mlx_uint32 semaphore_id;
	mlx_uint32 buffer;
	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto invalid_param;
	}

	status = mlx_utils_rand(utils, &semaphore_id);
	MLX_CHECK_STATUS(utils, status, rand_err, "failed to get random number");
#define ICMD_GET_SEMAPHORE_TRIES 2560
	for (retries = 0 ; retries < ICMD_GET_SEMAPHORE_TRIES ; retries++) {
		status = mlx_pci_gw_read( utils, PCI_GW_SPACE_SEMAPHORE,
					MLX_ICMD_SEMAPHORE_ADDR, &buffer);
		MLX_CHECK_STATUS(utils, status, read_err, "failed to read icmd semaphore");
		if (buffer != 0) {
			mlx_utils_delay_in_ms(10);
			continue;
		}
		mlx_pci_gw_write( utils, PCI_GW_SPACE_SEMAPHORE,
							MLX_ICMD_SEMAPHORE_ADDR, semaphore_id);
		MLX_CHECK_STATUS(utils, status, set_err, "failed to set icmd semaphore");
		status = mlx_pci_gw_read( utils, PCI_GW_SPACE_SEMAPHORE,
							MLX_ICMD_SEMAPHORE_ADDR, &buffer);
		MLX_CHECK_STATUS(utils, status, read_err, "failed to read icmd semaphore");
		if (semaphore_id == buffer) {
			status = MLX_SUCCESS;
			utils->icmd.took_semaphore = TRUE;
			break;
		}
		mlx_utils_delay_in_ms(10);
	}
	if (semaphore_id != buffer) {
		status = MLX_FAILED;
	}
read_err:
set_err:
rand_err:
invalid_param:
	return status;
}
static
mlx_status
mlx_icmd_clear_semaphore(
				IN mlx_utils *utils
				)
{
	mlx_status status = MLX_SUCCESS;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto invalid_param;
	}

	if (utils->icmd.took_semaphore == FALSE) {
		goto semaphore_not_taken;
	}
	status = mlx_pci_gw_write( utils, PCI_GW_SPACE_SEMAPHORE,
			MLX_ICMD_SEMAPHORE_ADDR, 0);
	MLX_CHECK_STATUS(utils, status, read_err, "failed to clear icmd semaphore");

	utils->icmd.took_semaphore = FALSE;
read_err:
semaphore_not_taken:
invalid_param:
	return status;
}

static
mlx_status
mlx_icmd_init(
				IN mlx_utils *utils
				)
{
	mlx_status status = MLX_SUCCESS;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto invalid_param;
	}
	if (utils->icmd.icmd_opened == TRUE) {
		goto already_opened;
	}

	utils->icmd.took_semaphore = FALSE;

	status = mlx_pci_gw_read( utils, PCI_GW_SPACE_ALL_ICMD,
			MLX_ICMD_MB_SIZE_ADDR, &utils->icmd.max_cmd_size);
	MLX_CHECK_STATUS(utils, status, read_err, "failed to read icmd mail box size");

	utils->icmd.icmd_opened = TRUE;
read_err:
already_opened:
invalid_param:
	return status;
}

static
mlx_status
mlx_icmd_set_opcode(
				IN mlx_utils *utils,
				IN mlx_uint16 opcode
				)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 buffer;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto invalid_param;
	}

	status = mlx_pci_gw_read( utils, PCI_GW_SPACE_ALL_ICMD,
				MLX_ICMD_CTRL_ADDR, &buffer);
	MLX_CHECK_STATUS(utils, status, read_err, "failed to read icmd ctrl");

#define MLX_ICMD_OPCODE_ALIGN 16
#define MLX_ICMD_OPCODE_MASK 0xffff

	buffer = buffer & ~(MLX_ICMD_OPCODE_MASK << MLX_ICMD_OPCODE_ALIGN);
	buffer = buffer | (opcode << MLX_ICMD_OPCODE_ALIGN);

	status = mlx_pci_gw_write( utils, PCI_GW_SPACE_ALL_ICMD,
					MLX_ICMD_CTRL_ADDR, buffer);
	MLX_CHECK_STATUS(utils, status, write_err, "failed to write icmd ctrl");
write_err:
read_err:
invalid_param:
	return status;
}

static
mlx_status
mlx_icmd_go(
			IN mlx_utils *utils
			)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 buffer;
	mlx_uint32 busy;
	mlx_uint32 wait_iteration = 0;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto invalid_param;
	}

	status = mlx_pci_gw_read( utils, PCI_GW_SPACE_ALL_ICMD,
				MLX_ICMD_CTRL_ADDR, &buffer);
	MLX_CHECK_STATUS(utils, status, read_err, "failed to read icmd ctrl");

#define MLX_ICMD_BUSY_ALIGN 0
#define MLX_ICMD_BUSY_MASK 0x1

	busy = (buffer >> MLX_ICMD_BUSY_ALIGN) & MLX_ICMD_BUSY_MASK;
	if (busy != 0) {
		status = MLX_FAILED;
		goto already_busy;
	}

	buffer = buffer | (1 << MLX_ICMD_BUSY_ALIGN);

	status = mlx_pci_gw_write( utils, PCI_GW_SPACE_ALL_ICMD,
					MLX_ICMD_CTRL_ADDR, buffer);
	MLX_CHECK_STATUS(utils, status, write_err, "failed to write icmd ctrl");

#define MLX_ICMD_BUSY_MAX_ITERATIONS 1024
	do {
		if (++wait_iteration > MLX_ICMD_BUSY_MAX_ITERATIONS) {
			status = MLX_FAILED;
			MLX_DEBUG_ERROR(utils, "ICMD time out");
			goto busy_timeout;
		}

		mlx_utils_delay_in_ms(10);
		status = mlx_pci_gw_read( utils, PCI_GW_SPACE_ALL_ICMD,
					MLX_ICMD_CTRL_ADDR, &buffer);
		MLX_CHECK_STATUS(utils, status, read_err, "failed to read icmd ctrl");
		busy = (buffer >> MLX_ICMD_BUSY_ALIGN) & MLX_ICMD_BUSY_MASK;
	} while (busy != 0);

busy_timeout:
write_err:
already_busy:
read_err:
invalid_param:
	return status;
}

static
mlx_status
mlx_icmd_get_status(
			IN mlx_utils *utils,
			OUT mlx_uint32 *out_status
			)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 buffer;

	if (utils == NULL || out_status == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto invalid_param;
	}

	status = mlx_pci_gw_read( utils, PCI_GW_SPACE_ALL_ICMD,
				MLX_ICMD_CTRL_ADDR, &buffer);
	MLX_CHECK_STATUS(utils, status, read_err, "failed to read icmd ctrl");

#define MLX_ICMD_STATUS_ALIGN 8
#define MLX_ICMD_STATUS_MASK 0xff

	*out_status = (buffer >> MLX_ICMD_STATUS_ALIGN) & MLX_ICMD_STATUS_MASK;

read_err:
invalid_param:
	return status;
}

static
mlx_status
mlx_icmd_write_buffer(
		IN mlx_utils *utils,
		IN mlx_void* data,
		IN mlx_uint32 data_size
		)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 data_offset = 0;
	mlx_size dword_size = sizeof(mlx_uint32);

	if (utils == NULL || data == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto invalid_param;
	}

	for (data_offset = 0 ; data_offset*dword_size < data_size ; data_offset++) {
		status = mlx_pci_gw_write( utils, PCI_GW_SPACE_ALL_ICMD,
							MLX_ICMD_MB_ADDR + data_offset*dword_size,
							((mlx_uint32*)data)[data_offset]);
		MLX_CHECK_STATUS(utils, status, write_err, "failed to write icmd MB");
	}
write_err:
invalid_param:
	return status;
}


static
mlx_status
mlx_icmd_read_buffer(
		IN mlx_utils *utils,
		OUT mlx_void* data,
		IN mlx_uint32 data_size
		)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 data_offset = 0;
	mlx_size dword_size = sizeof(mlx_uint32);

	if (utils == NULL || data == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto invalid_param;
	}

	for (data_offset = 0 ; data_offset*dword_size < data_size ; data_offset++) {
		status = mlx_pci_gw_read( utils, PCI_GW_SPACE_ALL_ICMD,
							MLX_ICMD_MB_ADDR + data_offset*dword_size,
							(mlx_uint32*)data + data_offset);
		MLX_CHECK_STATUS(utils, status, read_err, "failed to read icmd MB");
	}
read_err:
invalid_param:
	return status;
}
mlx_status
mlx_icmd_send_command(
				IN mlx_utils *utils,
				IN  mlx_uint16 opcode,
				IN OUT mlx_void* data,
				IN mlx_uint32 write_data_size,
				IN mlx_uint32 read_data_size
				)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 icmd_status = MLX_FAILED;

	if (utils == NULL || data == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto invalid_param;
	}
	status = mlx_icmd_init(utils);
	MLX_CHECK_STATUS(utils, status, open_err, "failed to open icmd");

	if (write_data_size > utils->icmd.max_cmd_size ||
			read_data_size > utils->icmd.max_cmd_size) {
		status = MLX_INVALID_PARAMETER;
		goto size_err;
	}

	status = mlx_icmd_get_semaphore(utils);
	MLX_CHECK_STATUS(utils, status, semaphore_err, "failed to get icmd semaphore");

	status = mlx_icmd_set_opcode(utils, opcode);
	MLX_CHECK_STATUS(utils, status, opcode_err, "failed to set icmd opcode");

	if (write_data_size != 0) {
		status = mlx_icmd_write_buffer(utils, data, write_data_size);
		MLX_CHECK_STATUS(utils, status, opcode_err, "failed to write icmd MB");
	}

	status = mlx_icmd_go(utils);
	MLX_CHECK_STATUS(utils, status, go_err, "failed to activate icmd");

	status = mlx_icmd_get_status(utils, &icmd_status);
	MLX_CHECK_STATUS(utils, status, get_status_err, "failed to set icmd opcode");

	if (icmd_status != 0) {
		MLX_DEBUG_ERROR(utils, "icmd failed with status = %d\n", icmd_status);
		status = MLX_FAILED;
		goto icmd_failed;
	}
	if (read_data_size != 0) {
		status = mlx_icmd_read_buffer(utils, data, read_data_size);
		MLX_CHECK_STATUS(utils, status, read_err, "failed to read icmd MB");
	}
read_err:
icmd_failed:
get_status_err:
go_err:
opcode_err:
	mlx_icmd_clear_semaphore(utils);
semaphore_err:
size_err:
open_err:
invalid_param:
	return status;
}

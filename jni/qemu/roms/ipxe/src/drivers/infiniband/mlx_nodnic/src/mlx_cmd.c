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

#include "../include/mlx_cmd.h"
#include "../../mlx_utils/include/public/mlx_pci_gw.h"
#include "../../mlx_utils/include/public/mlx_bail.h"
#include "../../mlx_utils/include/public/mlx_pci.h"
#include "../../mlx_utils/include/public/mlx_logging.h"

mlx_status
nodnic_cmd_read(
				IN nodnic_device_priv *device_priv,
				IN mlx_uint32 address,
				OUT mlx_pci_gw_buffer *buffer
				)
{
	mlx_status 		status = MLX_SUCCESS;
	mlx_utils 		*utils = NULL;

	if ( device_priv == NULL || buffer == NULL ) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	utils = device_priv->utils;

	status = mlx_pci_gw_read(utils, PCI_GW_SPACE_NODNIC, address, buffer);
	MLX_CHECK_STATUS(device_priv, status, read_error,"mlx_pci_gw_read failed");

read_error:
bad_param:
	return status;
}

mlx_status
nodnic_cmd_write(
				IN nodnic_device_priv *device_priv,
				IN mlx_uint32 address,
				IN mlx_pci_gw_buffer buffer
				)
{
	mlx_status 		status = MLX_SUCCESS;
	mlx_utils 		*utils = NULL;


	if ( device_priv == NULL ) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	utils = device_priv->utils;


	status = mlx_pci_gw_write(utils, PCI_GW_SPACE_NODNIC, address, buffer);
	MLX_CHECK_STATUS(device_priv, status, write_error,"mlx_pci_gw_write failed");
write_error:
bad_param:
	return status;
}

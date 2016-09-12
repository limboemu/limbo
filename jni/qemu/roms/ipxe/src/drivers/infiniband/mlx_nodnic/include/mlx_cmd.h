#ifndef NODNIC_CMD_H_
#define NODNIC_CMD_H_

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

#include "mlx_nodnic_data_structures.h"
#include "../../mlx_utils/include/public/mlx_utils.h"
#include "../../mlx_utils/include/public/mlx_pci_gw.h"

mlx_status
nodnic_cmd_read(
				IN nodnic_device_priv *device_priv,
				IN mlx_uint32 address,
				OUT mlx_pci_gw_buffer *buffer
				);

mlx_status
nodnic_cmd_write(
				IN nodnic_device_priv *device_priv,
				IN mlx_uint32 address,
				IN mlx_pci_gw_buffer buffer
				);

#endif /* STUB_NODNIC_CMD_H_ */

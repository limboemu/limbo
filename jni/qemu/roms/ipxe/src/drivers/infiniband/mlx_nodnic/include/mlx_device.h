#ifndef NODNIC_DEVICE_H_
#define NODNIC_DEVICE_H_

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

#define NODIC_SUPPORTED_REVISION 1
//Initialization segment
#define NODNIC_CMDQ_PHY_ADDR_HIGH_OFFSET 0x10
#define NODNIC_CMDQ_PHY_ADDR_LOW_OFFSET 0x14
#define NODNIC_NIC_INTERFACE_OFFSET 0x14
#define NODNIC_INITIALIZING_OFFSET 0x1fc
#define NODNIC_NIC_INTERFACE_SUPPORTED_OFFSET 0x1fc
#define NODNIC_LOCATION_OFFSET 0x240

#define NODNIC_CMDQ_PHY_ADDR_LOW_MASK 0xFFFFE000
#define NODNIC_NIC_INTERFACE_SUPPORTED_MASK 0x4000000

#define NODNIC_NIC_INTERFACE_BIT 9
#define NODNIC_DISABLE_INTERFACE_BIT 8
#define NODNIC_NIC_INTERFACE_SUPPORTED_BIT 26
#define NODNIC_INITIALIZING_BIT 31

#define NODNIC_NIC_DISABLE_INT_OFFSET	0x100c

//nodnic segment
#define NODNIC_REVISION_OFFSET 0x0
#define NODNIC_HARDWARE_FORMAT_OFFSET 0x0



mlx_status
nodnic_device_init(
				IN nodnic_device_priv *device_priv
				);

mlx_status
nodnic_device_teardown(
				IN nodnic_device_priv *device_priv
				);


mlx_status
nodnic_device_get_cap(
				IN nodnic_device_priv *device_priv
				);

mlx_status
nodnic_device_clear_int (
				IN nodnic_device_priv *device_priv
				);

mlx_status
nodnic_device_get_fw_version(
				IN nodnic_device_priv *device_priv,
				OUT mlx_uint16		*fw_ver_minor,
				OUT mlx_uint16  	*fw_ver_sub_minor,
				OUT mlx_uint16  	*fw_ver_major
				);
#endif /* STUB_NODNIC_DEVICE_H_ */

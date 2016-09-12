#ifndef INCLUDE_PUBLIC_MLX_PCI_GW_H_
#define INCLUDE_PUBLIC_MLX_PCI_GW_H_

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

#include "mlx_utils.h"

#define PCI_GW_FIRST_CAPABILITY_POINTER_OFFSET 0x34

#define PCI_GW_CAPABILITY_ID 0x9

#define PCI_GW_CAPABILITY_ID_OFFSET 0x0
#define PCI_GW_CAPABILITY_NEXT_POINTER_OFFSET 0x1
#define PCI_GW_CAPABILITY_SPACE_OFFSET 0x4
#define PCI_GW_CAPABILITY_STATUS_OFFSET 0x7
#define PCI_GW_CAPABILITY_COUNTER_OFFSET 0x8
#define PCI_GW_CAPABILITY_SEMAPHORE_OFFSET 0xC
#define PCI_GW_CAPABILITY_ADDRESS_OFFSET 0x10
#define PCI_GW_CAPABILITY_FLAG_OFFSET 0x10
#define PCI_GW_CAPABILITY_DATA_OFFSET 0x14

#define PCI_GW_SEMPHORE_TRIES 3000000
#define PCI_GW_GET_OWNERSHIP_TRIES 5000
#define PCI_GW_READ_FLAG_TRIES 3000000

#define PCI_GW_WRITE_FLAG 0x80000000

#define PCI_GW_SPACE_NODNIC 0x4
#define PCI_GW_SPACE_ALL_ICMD 0x3
#define PCI_GW_SPACE_SEMAPHORE 0xa
#define PCI_GW_SPACE_CR0 0x2

typedef mlx_uint32	mlx_pci_gw_buffer;


mlx_status
mlx_pci_gw_init(
				IN mlx_utils *utils
				);
mlx_status
mlx_pci_gw_teardown(
				IN mlx_utils *utils
				);
mlx_status
mlx_pci_gw_read(
				IN mlx_utils *utils,
				IN mlx_pci_gw_space space,
				IN mlx_uint32 address,
				OUT mlx_pci_gw_buffer *buffer
				);

mlx_status
mlx_pci_gw_write(
				IN mlx_utils *utils,
				IN mlx_pci_gw_space space,
				IN mlx_uint32 address,
				IN mlx_pci_gw_buffer buffer
				);



#endif /* INCLUDE_PUBLIC_MLX_PCI_GW_H_ */

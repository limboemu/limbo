#ifndef STUB_MLXUTILS_INCLUDE_PUBLIC_PCI_H_
#define STUB_MLXUTILS_INCLUDE_PUBLIC_PCI_H_

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

typedef enum {
			  MlxPciWidthUint8      = 0,
			  MlxPciWidthUint16,
			  MlxPciWidthUint32,
			  MlxPciWidthUint64,
} mlx_pci_width;

mlx_status
mlx_pci_init(
			IN mlx_utils *utils
			);

mlx_status
mlx_pci_read(
			IN mlx_utils *utils,
			IN mlx_pci_width width,
			IN mlx_uint32 offset,
			IN mlx_uintn count,
			OUT mlx_void *buffer
			);

mlx_status
mlx_pci_write(
			IN mlx_utils *utils,
			IN mlx_pci_width width,
			IN mlx_uint32 offset,
			IN mlx_uintn count,
			IN mlx_void *buffer
			);

mlx_status
mlx_pci_mem_read(
				IN mlx_utils *utils,
				IN mlx_pci_width width,
				IN mlx_uint8 bar_index,
				IN mlx_uint64 offset,
				IN mlx_uintn count,
				OUT mlx_void *buffer
				);

mlx_status
mlx_pci_mem_write(
				IN mlx_utils *utils,
				IN mlx_pci_width width,
				IN mlx_uint8 bar_index,
				IN mlx_uint64 offset,
				IN mlx_uintn count,
				IN mlx_void *buffer
				);


#endif /* STUB_MLXUTILS_INCLUDE_PUBLIC_PCI_H_ */

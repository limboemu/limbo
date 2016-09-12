#ifndef MLXUTILS_INCLUDE_PUBLIC_MEMORY_H_
#define MLXUTILS_INCLUDE_PUBLIC_MEMORY_H_

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


mlx_status
mlx_memory_alloc(
				IN mlx_utils *utils,
				IN mlx_size size,
				OUT mlx_void **ptr
				);

mlx_status
mlx_memory_zalloc(
				IN mlx_utils *utils,
				IN mlx_size size,
				OUT mlx_void **ptr
				);

mlx_status
mlx_memory_free(
				IN mlx_utils *utils,
				IN mlx_void **ptr
				);
mlx_status
mlx_memory_alloc_dma(
					IN mlx_utils *utils,
					IN mlx_size size ,
					IN mlx_size align,
					OUT mlx_void **ptr
					);

mlx_status
mlx_memory_free_dma(
					IN mlx_utils *utils,
					IN mlx_size size ,
					IN mlx_void **ptr
					);
mlx_status
mlx_memory_map_dma(
					IN mlx_utils *utils,
					IN mlx_void *Addr ,
					IN mlx_size NumberOfBytes,
					OUT mlx_physical_address *PhysAddr,
					OUT mlx_void **Mapping
					);

mlx_status
mlx_memory_ummap_dma(
					IN mlx_utils *utils,
					IN mlx_void *Mapping
					);

mlx_status
mlx_memory_cmp(
					IN mlx_utils *utils,
					IN mlx_void *first_block,
					IN mlx_void *second_block,
					IN mlx_size size,
					OUT mlx_uint32 *out
					);

mlx_status
mlx_memory_set(
					IN mlx_utils *utils,
					IN mlx_void *block,
					IN mlx_int32 value,
					IN mlx_size size
					);

mlx_status
mlx_memory_cpy(
					IN mlx_utils *utils,
					OUT mlx_void *destination_buffer,
					IN mlx_void *source_buffer,
					IN mlx_size length
					);

mlx_status
mlx_memory_cpu_to_be32(
			IN mlx_utils *utils,
			IN mlx_uint32 source,
			IN mlx_uint32 *destination
			);

mlx_status
mlx_memory_be32_to_cpu(
			IN mlx_utils *utils,
			IN mlx_uint32 source,
			IN mlx_uint32 *destination
			);

#endif /* STUB_MLXUTILS_INCLUDE_PUBLIC_MEMORY_H_ */

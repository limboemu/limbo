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

#include <stddef.h>
#include "../../include/private/mlx_memory_priv.h"
#include "../../include/public/mlx_memory.h"

mlx_status
mlx_memory_alloc(
				IN mlx_utils *utils,
				IN mlx_size size,
				OUT mlx_void **ptr
				)
{
	mlx_status status = MLX_SUCCESS;
	*ptr = NULL;
	if ( utils == NULL || size == 0 || *ptr != NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_alloc_priv(utils, size, ptr);
bad_param:
	return status;
}

mlx_status
mlx_memory_zalloc(
				IN mlx_utils *utils,
				IN mlx_size size,
				OUT mlx_void **ptr
				)
{
	mlx_status status = MLX_SUCCESS;
	*ptr = NULL;
	if ( utils == NULL || size == 0 || *ptr != NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_zalloc_priv(utils, size, ptr);
bad_param:
	return status;
}

mlx_status
mlx_memory_free(
				IN mlx_utils *utils,
				IN mlx_void **ptr
				)
{
	mlx_status status = MLX_SUCCESS;
	if ( utils == NULL ||  ptr == NULL || *ptr == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_free_priv(utils, *ptr);
	*ptr = NULL;
bad_param:
	return status;
}
mlx_status
mlx_memory_alloc_dma(
					IN mlx_utils *utils,
					IN mlx_size size ,
					IN mlx_size align,
					OUT mlx_void **ptr
					)
{
	mlx_status status = MLX_SUCCESS;
	*ptr = NULL;
	if ( utils == NULL || size == 0 || *ptr != NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_alloc_dma_priv(utils, size, align, ptr);
bad_param:
	return status;
}

mlx_status
mlx_memory_free_dma(
					IN mlx_utils *utils,
					IN mlx_size size ,
					IN mlx_void **ptr
					)
{
	mlx_status status = MLX_SUCCESS;
	if ( utils == NULL || size == 0 || ptr == NULL || *ptr == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_free_dma_priv(utils, size, *ptr);
	*ptr = NULL;
bad_param:
	return status;
}

mlx_status
mlx_memory_map_dma(
					IN mlx_utils *utils,
					IN mlx_void *addr ,
					IN mlx_size number_of_bytes,
					OUT mlx_physical_address *phys_addr,
					OUT mlx_void **mapping
					)
{
	mlx_status status = MLX_SUCCESS;
	if ( utils == NULL || phys_addr == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_map_dma_priv(utils, addr, number_of_bytes, phys_addr, mapping);
bad_param:
	return status;
}

mlx_status
mlx_memory_ummap_dma(
					IN mlx_utils *utils,
					IN mlx_void *mapping
					)
{
	mlx_status status = MLX_SUCCESS;
	if ( utils == NULL){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_ummap_dma_priv(utils, mapping);
bad_param:
	return status;
}

mlx_status
mlx_memory_cmp(
			IN mlx_utils *utils,
			IN mlx_void *first_block,
			IN mlx_void *second_block,
			IN mlx_size size,
			OUT mlx_uint32 *out
			)
{
	mlx_status status = MLX_SUCCESS;
	if ( utils == NULL || first_block == NULL || second_block == NULL ||
			out == NULL){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_cmp_priv(utils, first_block, second_block, size, out);
bad_param:
	return status;
}

mlx_status
mlx_memory_set(
					IN mlx_utils *utils,
					IN mlx_void *block,
					IN mlx_int32 value,
					IN mlx_size size
					)
{
	mlx_status status = MLX_SUCCESS;
	if ( utils == NULL || block == NULL){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_set_priv(utils, block, value, size);
bad_param:
	return status;
}

mlx_status
mlx_memory_cpy(
			IN mlx_utils *utils,
			OUT mlx_void *destination_buffer,
			IN mlx_void *source_buffer,
			IN mlx_size length
			)
{
	mlx_status status = MLX_SUCCESS;
	if ( utils == NULL || destination_buffer == NULL || source_buffer == NULL){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_cpy_priv(utils, destination_buffer, source_buffer, length);
bad_param:
	return status;
}

mlx_status
mlx_memory_cpu_to_be32(
			IN mlx_utils *utils,
			IN mlx_uint32 source,
			IN mlx_uint32 *destination
			)
{
	mlx_status status = MLX_SUCCESS;
	if ( utils == NULL || destination == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_cpu_to_be32_priv(utils, source, destination);
bad_param:
	return status;
}

mlx_status
mlx_memory_be32_to_cpu(
			IN mlx_utils *utils,
			IN mlx_uint32 source,
			IN mlx_uint32 *destination
			)
{
	mlx_status status = MLX_SUCCESS;
	if ( utils == NULL || destination == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}
	status = mlx_memory_be32_to_cpu_priv(utils, source, destination);
bad_param:
	return status;
}

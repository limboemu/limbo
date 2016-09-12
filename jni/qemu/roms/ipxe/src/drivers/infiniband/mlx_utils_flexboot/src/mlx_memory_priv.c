/*
 * MemoryPriv.c
 *
 *  Created on: Jan 21, 2015
 *      Author: maord
 */

#include <ipxe/malloc.h>
#include <stddef.h>
#include <byteswap.h>
#include <ipxe/io.h>
#include "../../mlx_utils/include/private/mlx_memory_priv.h"


mlx_status
mlx_memory_alloc_priv(
				IN mlx_utils *utils __attribute__ ((unused)),
				IN mlx_size size,
				OUT mlx_void **ptr
				)
{
	mlx_status status = MLX_SUCCESS;
	*ptr = malloc(size);
	if(*ptr == NULL){
		status = MLX_OUT_OF_RESOURCES;
	}
	return status;
}

mlx_status
mlx_memory_zalloc_priv(
				IN mlx_utils *utils __attribute__ ((unused)),
				IN mlx_size size,
				OUT mlx_void **ptr
				)
{
	mlx_status status = MLX_SUCCESS;
	*ptr = zalloc(size);
	if(*ptr == NULL){
		status = MLX_OUT_OF_RESOURCES;
	}
	return status;
}

mlx_status
mlx_memory_free_priv(
				IN mlx_utils *utils __attribute__ ((unused)),
				IN mlx_void *ptr
				)
{
	mlx_status status = MLX_SUCCESS;
	free(ptr);
	return status;
}
mlx_status
mlx_memory_alloc_dma_priv(
					IN mlx_utils *utils __attribute__ ((unused)),
					IN mlx_size size ,
					IN mlx_size align,
					OUT mlx_void **ptr
					)
{
	mlx_status status = MLX_SUCCESS;
	*ptr = malloc_dma(size, align);
	if (*ptr == NULL) {
		status = MLX_OUT_OF_RESOURCES;
	} else {
		memset(*ptr, 0, size);
	}
	return status;
}

mlx_status
mlx_memory_free_dma_priv(
					IN mlx_utils *utils __attribute__ ((unused)),
					IN mlx_size size ,
					IN mlx_void *ptr
					)
{
	mlx_status status = MLX_SUCCESS;
	free_dma(ptr, size);
	return status;
}
mlx_status
mlx_memory_map_dma_priv(
					IN mlx_utils *utils __attribute__ ((unused)),
					IN mlx_void *addr ,
					IN mlx_size number_of_bytes __attribute__ ((unused)),
					OUT mlx_physical_address *phys_addr,
					OUT mlx_void **mapping __attribute__ ((unused))
					)
{
	mlx_status status = MLX_SUCCESS;
	*phys_addr = virt_to_bus(addr);
	return status;
}

mlx_status
mlx_memory_ummap_dma_priv(
					IN mlx_utils *utils __attribute__ ((unused)),
					IN mlx_void *mapping __attribute__ ((unused))
					)
{
	mlx_status status = MLX_SUCCESS;
	return status;
}

mlx_status
mlx_memory_cmp_priv(
					IN mlx_utils *utils __unused,
					IN mlx_void *first_block,
					IN mlx_void *second_block,
					IN mlx_size size,
					OUT mlx_uint32 *out
					)
{
	mlx_status status = MLX_SUCCESS;
	*out = memcmp(first_block, second_block, size);
	return status;
}

mlx_status
mlx_memory_set_priv(
					IN mlx_utils *utils __unused,
					IN mlx_void *block,
					IN mlx_int32 value,
					IN mlx_size size
					)
{
	mlx_status status = MLX_SUCCESS;
	memset(block, value, size);
	return status;
}

mlx_status
mlx_memory_cpy_priv(
					IN mlx_utils *utils __unused,
					OUT mlx_void *destination_buffer,
					IN mlx_void *source_buffer,
					IN mlx_size length
					)
{
	mlx_status status = MLX_SUCCESS;
	memcpy(destination_buffer, source_buffer, length);
	return status;
}

mlx_status
mlx_memory_cpu_to_be32_priv(
			IN mlx_utils *utils __unused,
			IN mlx_uint32 source,
			IN mlx_uint32 *destination
			)
{
	mlx_status status = MLX_SUCCESS;
	*destination = cpu_to_be32(source);
	return status;
}


mlx_status
mlx_memory_be32_to_cpu_priv(
			IN mlx_utils *utils __unused,
			IN mlx_uint32 source,
			IN mlx_uint32 *destination
			)
{
	mlx_status status = MLX_SUCCESS;
	*destination = be32_to_cpu(source);
	return status;
}


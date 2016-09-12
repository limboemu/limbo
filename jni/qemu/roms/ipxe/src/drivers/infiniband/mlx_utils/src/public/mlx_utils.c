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
#include "../../include/private/mlx_utils_priv.h"
#include "../../include/public/mlx_pci.h"
#include "../../include/public/mlx_utils.h"

mlx_status
mlx_utils_init(
				IN mlx_utils *utils,
				IN mlx_pci *pci
				)
{
	mlx_status status = MLX_SUCCESS;
	if( pci == NULL || utils == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto bail;
	}
	utils->pci = pci;
	status = mlx_pci_init(utils);
	status = mlx_utils_init_lock(utils);
bail:
	return status;
}

mlx_status
mlx_utils_teardown(
				IN mlx_utils *utils __attribute__ ((unused))
				)
{
	mlx_status status = MLX_SUCCESS;
	mlx_utils_free_lock(utils);
	return status;
}

mlx_status
mlx_utils_delay_in_ms(
			IN mlx_uint32 msecs
		)
{
	mlx_utils_delay_in_ms_priv(msecs);
	return MLX_SUCCESS;
}
mlx_status
mlx_utils_delay_in_us(
			IN mlx_uint32 usecs
		)
{
	mlx_utils_delay_in_us_priv(usecs);
	return MLX_SUCCESS;
}
mlx_status
mlx_utils_ilog2(
			IN mlx_uint32 i,
			OUT mlx_uint32 *log
		)
{
	mlx_utils_ilog2_priv(i, log);
	return MLX_SUCCESS;
}

mlx_status
mlx_utils_init_lock(
			IN OUT mlx_utils *utils
		)
{
	return mlx_utils_init_lock_priv(&(utils->lock));

}

mlx_status
mlx_utils_free_lock(
			IN OUT mlx_utils *utils
		)
{
	return mlx_utils_free_lock_priv(utils->lock);
}

mlx_status
mlx_utils_acquire_lock (
			IN OUT mlx_utils *utils
		)
{
	return mlx_utils_acquire_lock_priv(utils->lock);
}

mlx_status
mlx_utils_release_lock (
		IN OUT mlx_utils *utils
		)
{
	return mlx_utils_release_lock_priv(utils->lock);
}

mlx_status
mlx_utils_rand (
		IN mlx_utils *utils,
		OUT mlx_uint32 *rand_num
		)
{
	return mlx_utils_rand_priv(utils, rand_num);
}

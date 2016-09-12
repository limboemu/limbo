#ifndef SRC_DRIVERS_INFINIBAND_MLX_UTILS_INCLUDE_PRIVATE_MLX_UTILS_PRIV_H_
#define SRC_DRIVERS_INFINIBAND_MLX_UTILS_INCLUDE_PRIVATE_MLX_UTILS_PRIV_H_

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

#include "../../include/public/mlx_utils.h"

mlx_status
mlx_utils_delay_in_ms_priv(
			IN mlx_uint32 msecs
		);

mlx_status
mlx_utils_delay_in_us_priv(
			IN mlx_uint32 usecs
		);

mlx_status
mlx_utils_ilog2_priv(
			IN mlx_uint32 i,
			OUT mlx_uint32 *log
		);

mlx_status
mlx_utils_init_lock_priv(
			OUT void **lock
		);

mlx_status
mlx_utils_free_lock_priv(
			IN void *lock
		);

mlx_status
mlx_utils_acquire_lock_priv (
			IN void *lock
		);

mlx_status
mlx_utils_release_lock_priv (
			IN void *lock
		);

mlx_status
mlx_utils_rand_priv (
		IN mlx_utils *utils,
		OUT mlx_uint32 *rand_num
		);
#endif /* SRC_DRIVERS_INFINIBAND_MLX_UTILS_INCLUDE_PRIVATE_MLX_UTILS_PRIV_H_ */

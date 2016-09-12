/*
 * MlxUtilsPriv.c
 *
 *  Created on: Jan 25, 2015
 *      Author: maord
 */

#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include "../../mlx_utils/include/private/mlx_utils_priv.h"

mlx_status
mlx_utils_delay_in_ms_priv(
			IN mlx_uint32 msecs
		)
{
	mdelay(msecs);
	return MLX_SUCCESS;
}

mlx_status
mlx_utils_delay_in_us_priv(
			IN mlx_uint32 usecs
		)
{
	udelay(usecs);
	return MLX_SUCCESS;
}

mlx_status
mlx_utils_ilog2_priv(
			IN mlx_uint32 i,
			OUT mlx_uint32 *log
		)
{
	*log = ( fls ( i ) - 1 );
	return MLX_SUCCESS;
}

mlx_status
mlx_utils_init_lock_priv(
			OUT void **lock __unused
		)
{
	return MLX_SUCCESS;
}

mlx_status
mlx_utils_free_lock_priv(
			IN void *lock __unused
		)
{
	return MLX_SUCCESS;
}

mlx_status
mlx_utils_acquire_lock_priv (
			IN void *lock __unused
		)
{
	return MLX_SUCCESS;
}

mlx_status
mlx_utils_release_lock_priv (
			IN void *lock __unused
		)
{
	return MLX_SUCCESS;
}

mlx_status
mlx_utils_rand_priv (
			IN  mlx_utils *utils __unused,
			OUT mlx_uint32 *rand_num
		)
{
	do {
		*rand_num = rand();
	} while ( *rand_num == 0 );
	return MLX_SUCCESS;
}

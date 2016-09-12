#ifndef MLXUTILS_INCLUDE_PUBLIC_MLXUTILS_H_
#define MLXUTILS_INCLUDE_PUBLIC_MLXUTILS_H_

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

#include "mlx_logging.h"
#include "mlx_types.h"

#define IN
#define OUT

typedef mlx_uint16	mlx_pci_gw_space;

typedef struct{
	mlx_uint32	pci_cmd_offset;
	mlx_pci_gw_space space;
} __attribute__ (( packed )) mlx_pci_gw;

typedef struct {
	mlx_boolean icmd_opened;
	mlx_boolean took_semaphore;
	mlx_uint32 max_cmd_size;
} __attribute__ (( packed )) mlx_icmd ;

typedef  struct{
	mlx_pci *pci;
	mlx_pci_gw pci_gw;
	mlx_icmd icmd;
	void *lock;
#ifdef DEVICE_CX3
	/* ACCESS to BAR0 */
	void *config;
#endif
} __attribute__ (( packed )) mlx_utils;

mlx_status
mlx_utils_init(
				IN mlx_utils *utils,
				IN mlx_pci *pci
				);

mlx_status
mlx_utils_teardown(
				IN mlx_utils *utils
				);
mlx_status
mlx_utils_delay_in_ms(
			IN mlx_uint32 msecs
		);

mlx_status
mlx_utils_delay_in_us(
			IN mlx_uint32 usecs
		);

mlx_status
mlx_utils_ilog2(
			IN mlx_uint32 i,
			OUT mlx_uint32 *log
		);

mlx_status
mlx_utils_init_lock(
			IN OUT mlx_utils *utils
		);

mlx_status
mlx_utils_free_lock(
			IN OUT mlx_utils *utils
		);

mlx_status
mlx_utils_acquire_lock (
			IN OUT mlx_utils *utils
		);

mlx_status
mlx_utils_release_lock (
		IN OUT mlx_utils *utils
		);

mlx_status
mlx_utils_rand (
		IN mlx_utils *utils,
		OUT mlx_uint32 *rand_num
		);
#endif /* STUB_MLXUTILS_INCLUDE_PUBLIC_MLXUTILS_H_ */

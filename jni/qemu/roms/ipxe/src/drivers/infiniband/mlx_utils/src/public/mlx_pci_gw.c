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

#include "../../include/public/mlx_pci_gw.h"
#include "../../include/public/mlx_bail.h"
#include "../../include/public/mlx_pci.h"
#include "../../include/public/mlx_logging.h"

/* Lock/unlock GW on each VSEC access */
#undef VSEC_DEBUG

static
mlx_status
mlx_pci_gw_check_capability_id(
							IN mlx_utils *utils,
							IN mlx_uint8 cap_pointer,
							OUT mlx_boolean *bool
							)
{
	mlx_status 		status = MLX_SUCCESS;
	mlx_uint8 		offset = cap_pointer + PCI_GW_CAPABILITY_ID_OFFSET;
	mlx_uint8 		id = 0;
	status = mlx_pci_read(utils, MlxPciWidthUint8, offset,
				1, &id);
	MLX_CHECK_STATUS(utils, status, read_err,"failed to read capability id");
	*bool = ( id == PCI_GW_CAPABILITY_ID );
read_err:
	return status;
}

static
mlx_status
mlx_pci_gw_get_ownership(
						IN mlx_utils *utils
						)
{
	mlx_status 			status = MLX_SUCCESS;
	mlx_uint32			cap_offset = utils->pci_gw.pci_cmd_offset;
	mlx_uint32 			semaphore = 0;
	mlx_uint32		 	counter = 0;
	mlx_uint32 			get_semaphore_try = 0;
	mlx_uint32 			get_ownership_try = 0;

	for( ; get_ownership_try < PCI_GW_GET_OWNERSHIP_TRIES; get_ownership_try ++){
		for( ; get_semaphore_try <= PCI_GW_SEMPHORE_TRIES ; get_semaphore_try++){
			status = mlx_pci_read(utils, MlxPciWidthUint32, cap_offset + PCI_GW_CAPABILITY_SEMAPHORE_OFFSET,
					1, &semaphore);
			MLX_CHECK_STATUS(utils, status, read_err,"failed to read semaphore");
			if( semaphore == 0 ){
				break;
			}
			mlx_utils_delay_in_us(10);
		}
		if( semaphore != 0 ){
			status = MLX_FAILED;
			goto semaphore_err;
		}

		status = mlx_pci_read(utils, MlxPciWidthUint32, cap_offset + PCI_GW_CAPABILITY_COUNTER_OFFSET,
						1, &counter);
		MLX_CHECK_STATUS(utils, status, read_err, "failed to read counter");

		status = mlx_pci_write(utils, MlxPciWidthUint32, cap_offset + PCI_GW_CAPABILITY_SEMAPHORE_OFFSET,
						1, &counter);
		MLX_CHECK_STATUS(utils, status, write_err,"failed to write semaphore");

		status = mlx_pci_read(utils, MlxPciWidthUint32, cap_offset + PCI_GW_CAPABILITY_SEMAPHORE_OFFSET,
						1, &semaphore);
		MLX_CHECK_STATUS(utils, status, read_err,"failed to read semaphore");
		if( counter == semaphore ){
			break;
		}
	}
	if( counter != semaphore ){
		status = MLX_FAILED;
	}
write_err:
read_err:
semaphore_err:
	return status;
}

static
mlx_status
mlx_pci_gw_free_ownership(
						IN mlx_utils *utils
						)
{
	mlx_status 		status = MLX_SUCCESS;
	mlx_uint32		cap_offset = utils->pci_gw.pci_cmd_offset;
	mlx_uint32 		value = 0;

	status = mlx_pci_write(utils, MlxPciWidthUint32, cap_offset + PCI_GW_CAPABILITY_SEMAPHORE_OFFSET,
					1, &value);
	MLX_CHECK_STATUS(utils, status, write_err,"failed to write semaphore");
write_err:
	return status;
}

static
mlx_status
mlx_pci_gw_set_space(
					IN mlx_utils *utils,
					IN mlx_pci_gw_space space
					)
{
	mlx_status 		status = MLX_SUCCESS;
	mlx_uint32		cap_offset = utils->pci_gw.pci_cmd_offset;;
	mlx_uint8		space_status = 0;

	/* set nodnic*/
	status = mlx_pci_write(utils, MlxPciWidthUint16, cap_offset + PCI_GW_CAPABILITY_SPACE_OFFSET, 1, &space);
	MLX_CHECK_STATUS(utils, status, read_error,"failed to write capability space");

	status = mlx_pci_read(utils, MlxPciWidthUint8, cap_offset + PCI_GW_CAPABILITY_STATUS_OFFSET, 1, &space_status);
	MLX_CHECK_STATUS(utils, status, read_error,"failed to read capability status");
	if( (space_status & 0x20) == 0){
		status = MLX_FAILED;
		goto space_unsupported;
	}
read_error:
space_unsupported:
	return status;
}

static
mlx_status
mlx_pci_gw_wait_for_flag_value(
							IN mlx_utils *utils,
							IN mlx_boolean value
							)
{
	mlx_status 		status = MLX_SUCCESS;
	mlx_uint32 		try = 0;
	mlx_uint32		cap_offset = utils->pci_gw.pci_cmd_offset;
	mlx_uint32		flag = 0;

	for(; try < PCI_GW_READ_FLAG_TRIES ; try ++ ) {
		status = mlx_pci_read(utils, MlxPciWidthUint32, cap_offset + PCI_GW_CAPABILITY_FLAG_OFFSET, 1, &flag);
		MLX_CHECK_STATUS(utils, status, read_error, "failed to read capability flag");
		if( ((flag & 0x80000000) != 0) == value ){
			goto flag_valid;
		}
		mlx_utils_delay_in_us(10);
	}
	status = MLX_FAILED;
flag_valid:
read_error:
	return status;
}
static
mlx_status
mlx_pci_gw_search_capability(
				IN mlx_utils *utils,
				OUT mlx_uint32	*cap_offset
				)
{
	mlx_status 		status = MLX_SUCCESS;
	mlx_uint8 		cap_pointer = 0;
	mlx_boolean		is_capability = FALSE;

	if( cap_offset == NULL || utils == NULL){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	//get first capability pointer
	status = mlx_pci_read(utils, MlxPciWidthUint8, PCI_GW_FIRST_CAPABILITY_POINTER_OFFSET,
			1, &cap_pointer);
	MLX_CHECK_STATUS(utils, status, read_err,
			"failed to read capability pointer");

	//search the right capability
	while( cap_pointer != 0 ){
		status = mlx_pci_gw_check_capability_id(utils, cap_pointer, &is_capability);
		MLX_CHECK_STATUS(utils, status, check_err
				,"failed to check capability id");

		if( is_capability == TRUE ){
			*cap_offset = cap_pointer;
			break;
		}

		status = mlx_pci_read(utils, MlxPciWidthUint8, cap_pointer +
				PCI_GW_CAPABILITY_NEXT_POINTER_OFFSET ,
				1, &cap_pointer);
		MLX_CHECK_STATUS(utils, status, read_err,
				"failed to read capability pointer");
	}
	if( is_capability != TRUE ){
		status = MLX_NOT_FOUND;
	}
check_err:
read_err:
bad_param:
	return status;
}

mlx_status
mlx_pci_gw_init(
			IN mlx_utils *utils
			)
{
	mlx_status 		status = MLX_SUCCESS;
	mlx_pci_gw *pci_gw = NULL;

	if( utils == NULL){
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	pci_gw = &utils->pci_gw;

	status = mlx_pci_gw_search_capability(utils, &pci_gw->pci_cmd_offset);
	MLX_CHECK_STATUS(utils, status, cap_err,
					"mlx_pci_gw_search_capability failed");

#if ! defined ( VSEC_DEBUG )
	status = mlx_pci_gw_get_ownership(utils);
	MLX_CHECK_STATUS(utils, status, ownership_err,"failed to get ownership");
ownership_err:
#endif
cap_err:
bad_param:
	return status;
}

mlx_status
mlx_pci_gw_teardown(
		IN mlx_utils *utils __attribute__ ((unused))
		)
{
#if ! defined ( VSEC_DEBUG )
	mlx_pci_gw_free_ownership(utils);
#endif
	return MLX_SUCCESS;
}

mlx_status
mlx_pci_gw_read(
		IN mlx_utils *utils,
		IN mlx_pci_gw_space space,
		IN mlx_uint32 address,
		OUT mlx_pci_gw_buffer *buffer
		)
{
	mlx_status 		status = MLX_SUCCESS;
	mlx_pci_gw 		*pci_gw = NULL;
	mlx_uint32		cap_offset = 0;

	if (utils == NULL || buffer == NULL || utils->pci_gw.pci_cmd_offset == 0) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	mlx_utils_acquire_lock(utils);

	pci_gw = &utils->pci_gw;
	cap_offset = pci_gw->pci_cmd_offset;

#if ! defined ( VSEC_DEBUG )
	if (pci_gw->space != space) {
		   status = mlx_pci_gw_set_space(utils, space);
		   MLX_CHECK_STATUS(utils, status, space_error,"failed to set space");
		   pci_gw->space = space;
	}
#else
	status = mlx_pci_gw_get_ownership(utils);
	MLX_CHECK_STATUS(utils, status, ownership_err,"failed to get ownership");

	status = mlx_pci_gw_set_space(utils, space);
	MLX_CHECK_STATUS(utils, status, space_error,"failed to set space");
	pci_gw->space = space;
#endif

	status = mlx_pci_write(utils, MlxPciWidthUint32, cap_offset + PCI_GW_CAPABILITY_ADDRESS_OFFSET, 1, &address);
	MLX_CHECK_STATUS(utils, status, read_error,"failed to write capability address");

#if defined ( DEVICE_CX3 )
	/* WA for PCI issue (race) */
	mlx_utils_delay_in_us ( 10 );
#endif

	status = mlx_pci_gw_wait_for_flag_value(utils, TRUE);
	MLX_CHECK_STATUS(utils, status, read_error, "flag failed to change");

	status = mlx_pci_read(utils, MlxPciWidthUint32, cap_offset + PCI_GW_CAPABILITY_DATA_OFFSET, 1, buffer);
	MLX_CHECK_STATUS(utils, status, read_error,"failed to read capability data");

#if defined ( VSEC_DEBUG )
	status = mlx_pci_gw_free_ownership(utils);
	MLX_CHECK_STATUS(utils, status, free_err,
						"mlx_pci_gw_free_ownership failed");
free_err:
	mlx_utils_release_lock(utils);
	return status;
#endif
read_error:
space_error:
#if defined ( VSEC_DEBUG )
	mlx_pci_gw_free_ownership(utils);
ownership_err:
#endif
mlx_utils_release_lock(utils);
bad_param:
	return status;
}

mlx_status
mlx_pci_gw_write(
				IN mlx_utils *utils,
				IN mlx_pci_gw_space space,
				IN mlx_uint32 address,
				IN mlx_pci_gw_buffer buffer
				)
{
	mlx_status 		status = MLX_SUCCESS;
	mlx_pci_gw 		*pci_gw = NULL;
	mlx_uint32		cap_offset = 0;
	mlx_uint32		fixed_address = address | PCI_GW_WRITE_FLAG;

	if (utils == NULL || utils->pci_gw.pci_cmd_offset == 0) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	mlx_utils_acquire_lock(utils);

	pci_gw = &utils->pci_gw;
	cap_offset = pci_gw->pci_cmd_offset;

#if ! defined ( VSEC_DEBUG )
	if (pci_gw->space != space) {
		   status = mlx_pci_gw_set_space(utils, space);
		   MLX_CHECK_STATUS(utils, status, space_error,"failed to set space");
		   pci_gw->space = space;
	}
#else
	status = mlx_pci_gw_get_ownership(utils);
	MLX_CHECK_STATUS(utils, status, ownership_err,"failed to get ownership");

	status = mlx_pci_gw_set_space(utils, space);
	MLX_CHECK_STATUS(utils, status, space_error,"failed to set space");
	pci_gw->space = space;
#endif
	status = mlx_pci_write(utils, MlxPciWidthUint32, cap_offset + PCI_GW_CAPABILITY_DATA_OFFSET, 1, &buffer);
	MLX_CHECK_STATUS(utils, status, read_error,"failed to write capability data");

	status = mlx_pci_write(utils, MlxPciWidthUint32, cap_offset + PCI_GW_CAPABILITY_ADDRESS_OFFSET, 1, &fixed_address);
	MLX_CHECK_STATUS(utils, status, read_error,"failed to write capability address");

	status = mlx_pci_gw_wait_for_flag_value(utils, FALSE);
	MLX_CHECK_STATUS(utils, status, read_error, "flag failed to change");
#if defined ( VSEC_DEBUG )
	status = mlx_pci_gw_free_ownership(utils);
	MLX_CHECK_STATUS(utils, status, free_err,
						"mlx_pci_gw_free_ownership failed");
free_err:
mlx_utils_release_lock(utils);
	return status;
#endif
read_error:
space_error:
#if defined ( VSEC_DEBUG )
	mlx_pci_gw_free_ownership(utils);
ownership_err:
#endif
mlx_utils_release_lock(utils);
bad_param:
	return status;
}




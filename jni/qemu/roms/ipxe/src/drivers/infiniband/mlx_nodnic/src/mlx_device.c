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

#include "../include/mlx_device.h"
#include "../include/mlx_cmd.h"
#include "../../mlx_utils/include/public/mlx_bail.h"
#include "../../mlx_utils/include/public/mlx_pci.h"
#include "../../mlx_utils/include/public/mlx_memory.h"
#include "../../mlx_utils/include/public/mlx_logging.h"

#define CHECK_BIT(field, offset)	(((field) & ((mlx_uint32)1 << (offset))) != 0)

static
mlx_status
check_nodnic_interface_supported(
							IN nodnic_device_priv* device_priv,
							OUT mlx_boolean *out
							)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32	output = 0;
	status = nodnic_cmd_read(device_priv, NODNIC_NIC_INTERFACE_SUPPORTED_OFFSET,
			&output);
	MLX_FATAL_CHECK_STATUS(status, read_error, "failed to read nic_interface_supported");
	*out = CHECK_BIT(output, NODNIC_NIC_INTERFACE_SUPPORTED_BIT);
read_error:
	return status;
}

static
mlx_status
wait_for_device_initialization(
							IN nodnic_device_priv* device_priv
							)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint8 try = 0;
	mlx_uint32			buffer = 0;

#define CHECK_DEVICE_INIT_TRIES 10
	for( ; try < CHECK_DEVICE_INIT_TRIES ; try++){
		status = nodnic_cmd_read(device_priv, NODNIC_INITIALIZING_OFFSET, &buffer);
		MLX_CHECK_STATUS(device_priv, status, read_error, "failed to read initializing");
		if( !CHECK_BIT(buffer, NODNIC_INITIALIZING_BIT)){
			goto init_done;
		}
		mlx_utils_delay_in_ms(100);
	}
	status = MLX_FAILED;
read_error:
init_done:
	return status;
}

static
mlx_status
disable_nodnic_inteface(
						IN nodnic_device_priv *device_priv
						)
{
	mlx_status 			status = MLX_SUCCESS;
	mlx_uint32			buffer = 0;

	buffer = (1 << NODNIC_DISABLE_INTERFACE_BIT);
	status = nodnic_cmd_write(device_priv, NODNIC_CMDQ_PHY_ADDR_LOW_OFFSET, buffer);
	MLX_FATAL_CHECK_STATUS(status, write_err, "failed to write cmdq_phy_addr + nic_interface");

	status = wait_for_device_initialization(device_priv);
	MLX_FATAL_CHECK_STATUS(status, init_err, "failed to initialize device");
init_err:
write_err:
	return status;
}
static
mlx_status
nodnic_device_start_nodnic(
				IN nodnic_device_priv *device_priv
				)
{
	mlx_status 			status = MLX_SUCCESS;
	mlx_uint32			buffer = 0;
	mlx_boolean			nodnic_supported = 0;

	status = wait_for_device_initialization(device_priv);
	MLX_FATAL_CHECK_STATUS(status, wait_for_fw_err, "failed to initialize device");

	status = check_nodnic_interface_supported(device_priv, &nodnic_supported);
	MLX_FATAL_CHECK_STATUS(status, read_err,"failed to check nic_interface_supported");

	if(	nodnic_supported == 0 ){
		status = MLX_UNSUPPORTED;
		goto nodnic_unsupported;
	}
	buffer =  (1 << NODNIC_NIC_INTERFACE_BIT);
	status = nodnic_cmd_write(device_priv, NODNIC_NIC_INTERFACE_OFFSET, buffer);
	MLX_FATAL_CHECK_STATUS(status, write_err, "failed to write cmdq_phy_addr + nic_interface");

	status = wait_for_device_initialization(device_priv);
	MLX_FATAL_CHECK_STATUS(status, init_err, "failed to initialize device");
init_err:
read_err:
write_err:
nodnic_unsupported:
wait_for_fw_err:
	return status;
}

static
mlx_status
nodnic_device_get_nodnic_data(
				IN nodnic_device_priv *device_priv
				)
{
	mlx_status 			status = MLX_SUCCESS;
	mlx_uint32			buffer = 0;

	status = nodnic_cmd_read(device_priv, NODNIC_LOCATION_OFFSET, &device_priv->device_offset);
	MLX_FATAL_CHECK_STATUS(status, nodnic_offset_read_err, "failed to read nodnic offset");

	status = nodnic_cmd_read(device_priv,
			device_priv->device_offset + NODNIC_REVISION_OFFSET, &buffer);
	MLX_FATAL_CHECK_STATUS(status, nodnic_revision_read_err, "failed to read nodnic revision");

	device_priv->nodnic_revision = (buffer >> 24) & 0xFF;
	if( device_priv->nodnic_revision != NODIC_SUPPORTED_REVISION ){
		MLX_DEBUG_ERROR(device_priv, "nodnic revision not supported\n");
		status = MLX_UNSUPPORTED;
		goto unsupported_revision;
	}

	status = nodnic_cmd_read(device_priv,
			device_priv->device_offset + NODNIC_HARDWARE_FORMAT_OFFSET, &buffer);
	MLX_FATAL_CHECK_STATUS(status, nodnic_hardware_format_read_err, "failed to read nodnic revision");
	device_priv->hardware_format = (buffer >> 16) & 0xFF;

	return status;

unsupported_revision:
nodnic_hardware_format_read_err:
nodnic_offset_read_err:
nodnic_revision_read_err:
	disable_nodnic_inteface(device_priv);
	return status;
}

mlx_status
nodnic_device_clear_int (
				IN nodnic_device_priv *device_priv
				)
{
	mlx_status 			status = MLX_SUCCESS;
	mlx_uint32			disable = 1;
#ifndef DEVICE_CX3
	status = nodnic_cmd_write(device_priv, NODNIC_NIC_DISABLE_INT_OFFSET, disable);
	MLX_CHECK_STATUS(device_priv, status, clear_int_done, "failed writing to disable_bit");
#else
	mlx_utils *utils = device_priv->utils;
	mlx_uint64 clear_int = (mlx_uint64)(device_priv->crspace_clear_int);
	mlx_uint32 swapped = 0;

	if (device_priv->device_cap.crspace_doorbells == 0) {
		status = nodnic_cmd_write(device_priv, NODNIC_NIC_DISABLE_INT_OFFSET, disable);
		MLX_CHECK_STATUS(device_priv, status, clear_int_done, "failed writing to disable_bit");
	} else {
		/* Write the new index and update FW that new data was submitted */
		disable = 0x80000000;
		mlx_memory_cpu_to_be32(utils, disable, &swapped);
		mlx_pci_mem_write (utils, MlxPciWidthUint32, 0, clear_int, 1, &swapped);
		mlx_pci_mem_read (utils, MlxPciWidthUint32, 0, clear_int, 1, &swapped);
	}
#endif
clear_int_done:
	return status;
}

mlx_status
nodnic_device_init(
				IN nodnic_device_priv *device_priv
				)
{
	mlx_status 			status = MLX_SUCCESS;

	if( device_priv == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto parm_err;
	}
	status = nodnic_device_start_nodnic(device_priv);
	MLX_FATAL_CHECK_STATUS(status, start_nodnic_err, "nodnic_device_start_nodnic failed");

	status = nodnic_device_get_nodnic_data(device_priv);
	MLX_FATAL_CHECK_STATUS(status, data_err, "nodnic_device_get_nodnic_data failed");
	return status;
data_err:
start_nodnic_err:
parm_err:
	return status;
}

mlx_status
nodnic_device_teardown(
				IN nodnic_device_priv *device_priv
				)
{
	mlx_status 			status = MLX_SUCCESS;
	status = disable_nodnic_inteface(device_priv);
	MLX_FATAL_CHECK_STATUS(status, disable_failed, "failed to disable nodnic interface");
disable_failed:
	return status;
}

mlx_status
nodnic_device_get_cap(
				IN nodnic_device_priv *device_priv
				)
{
	mlx_status 					status = MLX_SUCCESS;
	nodnic_device_capabilites 	*device_cap = NULL;
	mlx_uint32					buffer = 0;
	mlx_uint64					guid_l = 0;
	mlx_uint64					guid_h = 0;
	if( device_priv == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto parm_err;
	}

	device_cap = &device_priv->device_cap;

	//get device capabilities
	status = nodnic_cmd_read(device_priv, device_priv->device_offset + 0x0, &buffer);
	MLX_FATAL_CHECK_STATUS(status, read_err, "failed to read nodnic first dword");

#define NODNIC_DEVICE_SUPPORT_MAC_FILTERS_OFFSET 15
#define NODNIC_DEVICE_SUPPORT_PROMISC_FILTER_OFFSET 14
#define NODNIC_DEVICE_SUPPORT_PROMISC_MULT_FILTER_OFFSET 13
#define NODNIC_DEVICE_LOG_WORKING_BUFFER_SIZE_OFFSET 8
#define NODNIC_DEVICE_LOG_WORKING_BUFFER_SIZE_MASK 0x7
#define NODNIC_DEVICE_LOG_PKEY_TABLE_SIZE_OFFSET 4
#define NODNIC_DEVICE_LOG_PKEY_TABLE_SIZE_MASK 0xF
#define NODNIC_DEVICE_NUM_PORTS_OFFSET 0
	device_cap->support_mac_filters = CHECK_BIT(buffer, NODNIC_DEVICE_SUPPORT_MAC_FILTERS_OFFSET);

	device_cap->support_promisc_filter = CHECK_BIT(buffer, NODNIC_DEVICE_SUPPORT_PROMISC_FILTER_OFFSET);

	device_cap->support_promisc_multicast_filter = CHECK_BIT(buffer, NODNIC_DEVICE_SUPPORT_PROMISC_MULT_FILTER_OFFSET);

	device_cap->log_working_buffer_size =
			(buffer >> NODNIC_DEVICE_LOG_WORKING_BUFFER_SIZE_OFFSET) & NODNIC_DEVICE_LOG_WORKING_BUFFER_SIZE_MASK;

	device_cap->log_pkey_table_size =
			(buffer >> NODNIC_DEVICE_LOG_PKEY_TABLE_SIZE_OFFSET) & NODNIC_DEVICE_LOG_PKEY_TABLE_SIZE_MASK;

	device_cap->num_ports = CHECK_BIT(buffer, NODNIC_DEVICE_NUM_PORTS_OFFSET) + 1;

#ifdef DEVICE_CX3
#define NODNIC_DEVICE_CRSPACE_DB_OFFSET 12
	device_cap->crspace_doorbells = CHECK_BIT(buffer, NODNIC_DEVICE_CRSPACE_DB_OFFSET);
#endif

	status = nodnic_cmd_read(device_priv, device_priv->device_offset + 0x4, &buffer);
	MLX_FATAL_CHECK_STATUS(status, read_err, "failed to read nodnic second dword");

#define NODNIC_DEVICE_LOG_MAX_RING_SIZE_OFFSET 24
#define NODNIC_DEVICE_LOG_MAX_RING_SIZE_MASK 0x3F
#define NODNIC_DEVICE_PD_MASK 0xFFFFFF
	device_cap->log_max_ring_size =
			(buffer >> NODNIC_DEVICE_LOG_MAX_RING_SIZE_OFFSET) & NODNIC_DEVICE_LOG_MAX_RING_SIZE_MASK;

	//get device magic numbers
	device_priv->pd = buffer & NODNIC_DEVICE_PD_MASK;

	status = nodnic_cmd_read(device_priv, device_priv->device_offset + 0x8, &buffer);
	MLX_FATAL_CHECK_STATUS(status, read_err, "failed to read nodnic third dword");
	device_priv->lkey = buffer;

#ifdef DEVICE_CX3
	if ( device_cap->crspace_doorbells ) {
		status = nodnic_cmd_read(device_priv, device_priv->device_offset + 0x18, &buffer);
		MLX_FATAL_CHECK_STATUS(status, read_err, "failed to read nodnic_crspace_clear_int address");
		device_priv->crspace_clear_int = device_priv->utils->config + buffer;
	}
#endif

	status = nodnic_cmd_read(device_priv, device_priv->device_offset + 0x10, (mlx_uint32*)&guid_h);
	MLX_FATAL_CHECK_STATUS(status, read_err, "failed to read nodnic guid_h");
	status = nodnic_cmd_read(device_priv, device_priv->device_offset + 0x14, (mlx_uint32*)&guid_l);
	MLX_FATAL_CHECK_STATUS(status, read_err, "failed to read nodnic guid_l");
	device_priv->device_guid = guid_l | (guid_h << 32);
read_err:
parm_err:
	return status;
}

mlx_status
nodnic_device_get_fw_version(
				IN nodnic_device_priv *device_priv,
				OUT mlx_uint16		*fw_ver_minor,
				OUT mlx_uint16  	*fw_ver_sub_minor,
				OUT mlx_uint16  	*fw_ver_major
				){
	mlx_status 		status = MLX_SUCCESS;
	mlx_uint32		buffer = 0;

	if( device_priv == NULL ){
		status = MLX_INVALID_PARAMETER;
		goto parm_err;
	}

	status = nodnic_cmd_read(device_priv, 0x0, &buffer);
	MLX_CHECK_STATUS(device_priv, status, read_err, "failed to read fw revision major and minor");

	*fw_ver_minor = (mlx_uint16)(buffer >> 16);
	*fw_ver_major = (mlx_uint16)buffer;

	status = nodnic_cmd_read(device_priv, 0x4, &buffer);
	MLX_CHECK_STATUS(device_priv, status, read_err, "failed to read fw revision sub minor");

	*fw_ver_sub_minor = (mlx_uint16)buffer;
read_err:
parm_err:
	return status;
}

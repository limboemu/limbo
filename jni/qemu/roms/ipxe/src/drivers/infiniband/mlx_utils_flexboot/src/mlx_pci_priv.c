/*
 * MlxPciPriv.c
 *
 *  Created on: Jan 21, 2015
 *      Author: maord
 */

#include <ipxe/pci.h>
#include "../../mlx_utils/include/private/mlx_pci_priv.h"


static
mlx_status
mlx_pci_config_byte(
		IN mlx_utils *utils,
		IN mlx_boolean read,
		IN mlx_uint32 offset,
		IN OUT mlx_uint8 *buffer
		)
{
	mlx_status status = MLX_SUCCESS;
	if (read) {
		status = pci_read_config_byte(utils->pci, offset, buffer);
	}else {
		status = pci_write_config_byte(utils->pci, offset, *buffer);
	}
	return status;
}

static
mlx_status
mlx_pci_config_word(
		IN mlx_utils *utils,
		IN mlx_boolean read,
		IN mlx_uint32 offset,
		IN OUT mlx_uint16 *buffer
		)
{
	mlx_status status = MLX_SUCCESS;
	if (read) {
		status = pci_read_config_word(utils->pci, offset, buffer);
	}else {
		status = pci_write_config_word(utils->pci, offset, *buffer);
	}
	return status;
}

static
mlx_status
mlx_pci_config_dword(
		IN mlx_utils *utils,
		IN mlx_boolean read,
		IN mlx_uint32 offset,
		IN OUT mlx_uint32 *buffer
		)
{
	mlx_status status = MLX_SUCCESS;
	if (read) {
		status = pci_read_config_dword(utils->pci, offset, buffer);
	}else {
		status = pci_write_config_dword(utils->pci, offset, *buffer);
	}
	return status;
}
static
mlx_status
mlx_pci_config(
		IN mlx_utils *utils,
		IN mlx_boolean read,
		IN mlx_pci_width width,
		IN mlx_uint32 offset,
		IN mlx_uintn count,
		IN OUT mlx_void *buffer
		)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint8 *tmp =  (mlx_uint8*)buffer;
	mlx_uintn iteration = 0;
	if( width == MlxPciWidthUint64) {
		width = MlxPciWidthUint32;
		count = count * 2;
	}

	for(;iteration < count ; iteration++) {
		switch (width){
		case MlxPciWidthUint8:
			status = mlx_pci_config_byte(utils, read , offset++, tmp++);
			break;
		case MlxPciWidthUint16:
			status = mlx_pci_config_word(utils, read , offset, (mlx_uint16*)tmp);
			tmp += 2;
			offset += 2;
			break;
		case MlxPciWidthUint32:
			status = mlx_pci_config_dword(utils, read , offset, (mlx_uint32*)tmp);
			tmp += 4;
			offset += 4;
			break;
		default:
			status = MLX_INVALID_PARAMETER;
		}
		if(status != MLX_SUCCESS) {
			goto config_error;
		}
	}
config_error:
	return status;
}
mlx_status
mlx_pci_init_priv(
			IN mlx_utils *utils
			)
{
	mlx_status status = MLX_SUCCESS;
	adjust_pci_device ( utils->pci );
#ifdef DEVICE_CX3
	utils->config = ioremap ( pci_bar_start ( utils->pci, PCI_BASE_ADDRESS_0),
			0x100000 );
#endif
	return status;
}

mlx_status
mlx_pci_read_priv(
			IN mlx_utils *utils,
			IN mlx_pci_width width,
			IN mlx_uint32 offset,
			IN mlx_uintn count,
			OUT mlx_void *buffer
			)
{
	mlx_status status = MLX_SUCCESS;
	status = mlx_pci_config(utils, TRUE, width, offset, count, buffer);
	return status;
}

mlx_status
mlx_pci_write_priv(
			IN mlx_utils *utils,
			IN mlx_pci_width width,
			IN mlx_uint32 offset,
			IN mlx_uintn count,
			IN mlx_void *buffer
			)
{
	mlx_status status = MLX_SUCCESS;
	status = mlx_pci_config(utils, FALSE, width, offset, count, buffer);
	return status;
}

mlx_status
mlx_pci_mem_read_priv(
				IN mlx_utils *utils  __attribute__ ((unused)),
				IN mlx_pci_width width  __attribute__ ((unused)),
				IN mlx_uint8 bar_index  __attribute__ ((unused)),
				IN mlx_uint64 offset,
				IN mlx_uintn count  __attribute__ ((unused)),
				OUT mlx_void *buffer
				)
{
	if (buffer == NULL || width != MlxPciWidthUint32)
		return MLX_INVALID_PARAMETER;
	*((mlx_uint32 *)buffer) = readl(offset);
	return MLX_SUCCESS;
}

mlx_status
mlx_pci_mem_write_priv(
				IN mlx_utils *utils  __attribute__ ((unused)),
				IN mlx_pci_width width  __attribute__ ((unused)),
				IN mlx_uint8 bar_index  __attribute__ ((unused)),
				IN mlx_uint64 offset,
				IN mlx_uintn count  __attribute__ ((unused)),
				IN mlx_void *buffer
				)
{
	if (buffer == NULL || width != MlxPciWidthUint32)
		return MLX_INVALID_PARAMETER;
	barrier();
	writel(*((mlx_uint32 *)buffer), offset);
	return MLX_SUCCESS;
}

#ifndef MLX_NVCONFIG_H_
#define MLX_NVCONFIG_H_

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

#include "../mlx_reg_access/mlx_reg_access.h"
#include "../../include/public/mlx_utils.h"

typedef enum {
	NVRAM_TLV_CLASS_GLOBAL = 0,
	NVRAM_TLV_CLASS_PHYSICAL_PORT = 1,
	NVRAM_TLV_CLASS_HOST = 3,
} NVRAM_CLASS_CODE;

struct nvconfig_tlv_type_per_port {
	 mlx_uint32 param_idx	:16;
	 mlx_uint32 port		:8;
	 mlx_uint32 param_class	:8;
};

struct nvconfig_tlv_type_per_host {
	mlx_uint32 param_idx	:10;
	mlx_uint32 function		:8;
	mlx_uint32 host			:6;
	mlx_uint32 param_class	:8;
};

struct nvconfig_tlv_type_global {
	mlx_uint32 param_idx	:24;
	mlx_uint32 param_class	:8;
};

struct nvconfig_tlv_mapping{
	mlx_uint16	tlv_type;
	mlx_uint16	real_tlv_type;
	NVRAM_CLASS_CODE class_code;
	mlx_boolean fw_reset_needed;
};

union nvconfig_tlv_type {
	struct nvconfig_tlv_type_per_port per_port;
	struct nvconfig_tlv_type_per_host per_host;
	struct nvconfig_tlv_type_global global;
};


struct nvconfig_nvqc {
	union nvconfig_tlv_type tlv_type;
/* -------------- */
	 mlx_uint32 support_rd	:1; /*the configuration item is supported and can be read */
	 mlx_uint32 support_wr	:1; /*the configuration item is supported and can be updated */
	 mlx_uint32 reserved1	:2;
	 mlx_uint32 version		:4; /*The maximum version of the configuration item currently supported by the firmware. */
	 mlx_uint32 reserved2	:24;
};


struct nvconfig_header {
	 mlx_uint32 length		:9; /*Size of configuration item data in bytes between 0..256 */
	 mlx_uint32 reserved0	:3;
	 mlx_uint32 version		:4; /* Configuration item version */
	 mlx_uint32 reserved1	:7;

	 mlx_uint32 def_en		:1; /*Choose whether to access the default value or the user-defined value.
									0x0 Read or write the user-defined value.
									0x1 Read the default value (only valid for reads).*/

	 mlx_uint32 rd_en		:1; /*enables reading the TLV by lower priorities
									0 - TLV can be read by the subsequent lifecycle priorities.
									1 - TLV cannot be read by the subsequent lifecycle priorities. */
	 mlx_uint32 over_en		:1; /*enables overwriting the TLV by lower priorities
									0 - Can only be overwritten by the current lifecycle priority
									1 - Allowed to be overwritten by subsequent lifecycle priorities */
	 mlx_uint32 header_type	:2;
	 mlx_uint32 priority		:2;
	 mlx_uint32 valid	:2;
/* -------------- */
	 union nvconfig_tlv_type tlv_type;;
/* -------------- */
	mlx_uint32 crc			:16;
	mlx_uint32 reserved		:16;
};

#define NVCONFIG_MAX_TLV_SIZE 256

struct nvconfig_nvda {
	struct nvconfig_header nv_header;
	mlx_uint8 data[NVCONFIG_MAX_TLV_SIZE];
};


mlx_status
nvconfig_query_capability(
		IN mlx_utils *utils,
		IN mlx_uint8 port,
		IN mlx_uint16 tlv_type,
		OUT mlx_boolean *read_supported,
		OUT mlx_boolean *write_supported
		);


mlx_status
nvconfig_nvdata_invalidate(
		IN mlx_utils *utils,
		IN mlx_uint8 port,
		IN mlx_uint16 tlv_type
		);

mlx_status
nvconfig_nvdata_access(
		IN mlx_utils *utils,
		IN mlx_uint8 port,
		IN mlx_uint16 tlv_type,
		IN REG_ACCESS_OPT opt,
		IN mlx_size data_size,
		IN NV_DEFAULT_OPT def_en,
		IN OUT mlx_uint8 *version,
		IN OUT mlx_void *data
		);

#endif /* MLX_NVCONFIG_H_ */

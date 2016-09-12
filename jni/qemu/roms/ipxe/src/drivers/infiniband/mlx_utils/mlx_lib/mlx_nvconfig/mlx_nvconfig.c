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

#include "../../mlx_lib/mlx_nvconfig/mlx_nvconfig.h"
#include "../../include/public/mlx_memory.h"
#include "../../include/public/mlx_bail.h"

#define TlvMappingEntry( _tlv_type, _real_tlv_type, _class_code, _fw_reset_needed) { \
  .tlv_type = _tlv_type,                     \
  .real_tlv_type = _real_tlv_type,                   \
  .class_code = _class_code,                  \
  .fw_reset_needed = _fw_reset_needed,        \
  }

struct nvconfig_tlv_mapping nvconfig_tlv_mapping[] = {
		TlvMappingEntry(0x10, 0x10, NVRAM_TLV_CLASS_HOST, TRUE),
		TlvMappingEntry(0x12, 0x12, NVRAM_TLV_CLASS_PHYSICAL_PORT, TRUE),
		TlvMappingEntry(0x80, 0x80, NVRAM_TLV_CLASS_GLOBAL, TRUE),
		TlvMappingEntry(0x81, 0x81, NVRAM_TLV_CLASS_GLOBAL, TRUE),
		TlvMappingEntry(0x100, 0x100, NVRAM_TLV_CLASS_GLOBAL, TRUE),
		TlvMappingEntry(0x2001, 0x195, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2010, 0x210, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2011, 0x211, NVRAM_TLV_CLASS_GLOBAL, FALSE),
		TlvMappingEntry(0x2020, 0x2020, NVRAM_TLV_CLASS_PHYSICAL_PORT, FALSE),
		TlvMappingEntry(0x2021, 0x221, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2023, 0x223, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2100, 0x230, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2101, 0x231, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2102, 0x232, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2103, 0x233, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2104, 0x234, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2105, 0x235, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2106, 0x236, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2107, 0x237, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2108, 0x238, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2109, 0x239, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x210A, 0x23A, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2200, 0x240, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2201, 0x241, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2202, 0x242, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2203, 0x243, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2204, 0x244, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2205, 0x245, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0x2207, 0x247, NVRAM_TLV_CLASS_HOST, FALSE),
		TlvMappingEntry(0, 0, 0, 0),
};

static
mlx_status
nvconfig_set_fw_reset_level(
		IN mlx_utils *utils,
		IN	mlx_uint16	tlv_type
		)
{
#define WARM_REBOOT_RESET ((mlx_uint64)0x1 << 38)
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 reg_status;
	mlx_uint64 mfrl = WARM_REBOOT_RESET ;
	mlx_uint8 index = 0;
	mlx_boolean reset_needed = FALSE;

	for (index = 0 ; nvconfig_tlv_mapping[index].tlv_type != 0 ; index++) {
		if (nvconfig_tlv_mapping[index].tlv_type == tlv_type) {
			reset_needed = nvconfig_tlv_mapping[index].fw_reset_needed;
		}
	}

	if (reset_needed == FALSE) {
		goto no_fw_reset_needed;
	}
	status = mlx_reg_access(utils, REG_ID_MFRL, REG_ACCESS_WRITE, &mfrl, sizeof(mfrl),
				&reg_status);
	MLX_CHECK_STATUS(utils, status, reg_err, "mlx_reg_access failed ");

	if (reg_status != 0) {
		MLX_DEBUG_ERROR(utils,"nvconfig_set_fw_reset_level failed with status = %d\n", reg_status);
		status = MLX_FAILED;
		goto reg_err;
	}
reg_err:
no_fw_reset_needed:
	return status;
}


static
mlx_status
nvconfig_get_tlv_type_and_class(
		IN	mlx_uint16	tlv_type,
		OUT mlx_uint16	*real_tlv_type,
		OUT NVRAM_CLASS_CODE *class_code
		)
{
	mlx_uint8 index = 0;
	for ( ; nvconfig_tlv_mapping[index].tlv_type != 0 ; index ++) {
		if ( nvconfig_tlv_mapping[index].tlv_type == tlv_type) {
			*real_tlv_type = nvconfig_tlv_mapping[index].real_tlv_type;
			*class_code = nvconfig_tlv_mapping[index].class_code;
			return MLX_SUCCESS;
		}
	}
	return MLX_NOT_FOUND;
}
static
void
nvconfig_fill_tlv_type(
		IN mlx_uint8 port,
		IN NVRAM_CLASS_CODE class_code,
		IN mlx_uint16 tlv_type,
		OUT union nvconfig_tlv_type *nvconfig_tlv_type
		)
{
	switch (class_code) {
	case NVRAM_TLV_CLASS_GLOBAL:
		nvconfig_tlv_type->global.param_class = NVRAM_TLV_CLASS_GLOBAL;
		nvconfig_tlv_type->global.param_idx = tlv_type;
		break;
	case NVRAM_TLV_CLASS_HOST:
		nvconfig_tlv_type->per_host.param_class = NVRAM_TLV_CLASS_HOST;
		nvconfig_tlv_type->per_host.param_idx = tlv_type;
		break;
	case NVRAM_TLV_CLASS_PHYSICAL_PORT:
		nvconfig_tlv_type->per_port.param_class = NVRAM_TLV_CLASS_PHYSICAL_PORT;
		nvconfig_tlv_type->per_port.param_idx = tlv_type;
		nvconfig_tlv_type->per_port.port = port;
		break;
	}
}
mlx_status
nvconfig_query_capability(
		IN mlx_utils *utils,
		IN mlx_uint8 port,
		IN mlx_uint16 tlv_type,
		OUT mlx_boolean *read_supported,
		OUT mlx_boolean *write_supported
		)
{
	mlx_status status = MLX_SUCCESS;
	struct nvconfig_nvqc nvqc;
	mlx_uint32 reg_status;
	NVRAM_CLASS_CODE class_code;
	mlx_uint16 real_tlv_type;

	if (utils == NULL || read_supported == NULL || write_supported == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = nvconfig_get_tlv_type_and_class(tlv_type, &real_tlv_type, &class_code);
	MLX_CHECK_STATUS(utils, status, tlv_not_supported, "tlv not supported");

	mlx_memory_set(utils, &nvqc, 0, sizeof(nvqc));
	nvconfig_fill_tlv_type(port, class_code, real_tlv_type, &nvqc.tlv_type);

	status = mlx_reg_access(utils, REG_ID_NVQC, REG_ACCESS_READ, &nvqc, sizeof(nvqc),
			&reg_status);
	MLX_CHECK_STATUS(utils, status, reg_err, "mlx_reg_access failed ");
	if (reg_status != 0) {
		MLX_DEBUG_ERROR(utils,"mlx_reg_access failed with status = %d\n", reg_status);
		status = MLX_FAILED;
		goto reg_err;
	}
	*read_supported = nvqc.support_rd;
	*write_supported = nvqc.support_wr;
reg_err:
tlv_not_supported:
bad_param:
	return status;
}

mlx_status
nvconfig_nvdata_invalidate(
		IN mlx_utils *utils,
		IN mlx_uint8 port,
		IN mlx_uint16 tlv_type
		)
{
	mlx_status status = MLX_SUCCESS;
	struct nvconfig_header nv_header;
	mlx_uint32 reg_status;
	NVRAM_CLASS_CODE class_code;
	mlx_uint16 real_tlv_type;

	if (utils == NULL) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = nvconfig_get_tlv_type_and_class(tlv_type, &real_tlv_type, &class_code);
	MLX_CHECK_STATUS(utils, status, tlv_not_supported, "tlv not supported");

	mlx_memory_set(utils, &nv_header, 0, sizeof(nv_header));
	nvconfig_fill_tlv_type(port, class_code, real_tlv_type, &nv_header.tlv_type);

	status = mlx_reg_access(utils, REG_ID_NVDI, REG_ACCESS_WRITE, &nv_header, sizeof(nv_header),
			&reg_status);
	MLX_CHECK_STATUS(utils, status, reg_err, "mlx_reg_access failed ");
	if (reg_status != 0) {
		MLX_DEBUG_ERROR(utils,"mlx_reg_access failed with status = %d\n", reg_status);
		status = MLX_FAILED;
		goto reg_err;
	}
reg_err:
tlv_not_supported:
bad_param:
	return status;
}

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
		)
{
	mlx_status status = MLX_SUCCESS;
	struct nvconfig_nvda nvda;
	mlx_uint32 reg_status;
	mlx_uint32 real_size_to_read;
	mlx_uint32 index;
	NVRAM_CLASS_CODE class_code;
	mlx_uint16 real_tlv_type;
	mlx_size data_size_align_to_dword;

	if (utils == NULL || data == NULL || data_size > NVCONFIG_MAX_TLV_SIZE) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	status = nvconfig_get_tlv_type_and_class(tlv_type, &real_tlv_type, &class_code);
	MLX_CHECK_STATUS(utils, status, tlv_not_supported, "tlv not supported");

	data_size_align_to_dword = ((data_size + 3) / sizeof(mlx_uint32)) * sizeof(mlx_uint32);
	mlx_memory_set(utils, &nvda, 0, sizeof(nvda));
	nvda.nv_header.length = data_size_align_to_dword;
	nvda.nv_header.rd_en = 0;
	nvda.nv_header.def_en = def_en;
	nvda.nv_header.over_en = 1;
	nvda.nv_header.version = *version;

	nvconfig_fill_tlv_type(port, class_code, real_tlv_type, &nvda.nv_header.tlv_type);

	mlx_memory_cpy(utils, nvda.data, data, data_size);
	for (index = 0 ; index * 4 < NVCONFIG_MAX_TLV_SIZE ; index++) {
		mlx_memory_be32_to_cpu(utils,(((mlx_uint32 *)nvda.data)[index]), ((mlx_uint32 *)nvda.data) + index);
	}
	status = mlx_reg_access(utils, REG_ID_NVDA, opt, &nvda,
			data_size_align_to_dword + sizeof(nvda.nv_header), &reg_status);
	MLX_CHECK_STATUS(utils, status, reg_err, "mlx_reg_access failed ");
	if (reg_status != 0) {
		MLX_DEBUG_ERROR(utils,"mlx_reg_access failed with status = %d\n", reg_status);
		status = MLX_FAILED;
		goto reg_err;
	}
	for (index = 0 ; index * 4 < NVCONFIG_MAX_TLV_SIZE ; index++) {
		mlx_memory_cpu_to_be32(utils,(((mlx_uint32 *)nvda.data)[index]), ((mlx_uint32 *)nvda.data) + index);
	}
	if (opt == REG_ACCESS_READ) {
		real_size_to_read = (nvda.nv_header.length > data_size) ? data_size :
				nvda.nv_header.length;
		mlx_memory_cpy(utils, data, nvda.data, real_size_to_read);
		*version = nvda.nv_header.version;
	} else {
		nvconfig_set_fw_reset_level(utils, tlv_type);
	}
reg_err:
tlv_not_supported:
bad_param:
	return status;
}



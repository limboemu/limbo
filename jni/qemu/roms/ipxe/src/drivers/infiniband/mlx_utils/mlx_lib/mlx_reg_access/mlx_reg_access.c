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

#include "../../mlx_lib/mlx_reg_access/mlx_reg_access.h"
#include "../../include/public/mlx_icmd.h"
#include "../../include/public/mlx_bail.h"
#include "../../include/public/mlx_memory.h"

static
mlx_status
init_operation_tlv(
		IN struct mail_box_tlv *mail_box_tlv,
		IN mlx_uint16  reg_id,
		IN REG_ACCESS_OPT reg_opt
		)
{
#define TLV_OPERATION 1
	mail_box_tlv->operation_tlv.Type			= TLV_OPERATION;
#define MAD_CLASS_REG_ACCESS 1
	mail_box_tlv->operation_tlv.cls			= MAD_CLASS_REG_ACCESS;
#define TLV_OPERATION_SIZE 4
	mail_box_tlv->operation_tlv.len			= TLV_OPERATION_SIZE;
	mail_box_tlv->operation_tlv.method			= reg_opt;
	mail_box_tlv->operation_tlv.register_id	= reg_id;
	return MLX_SUCCESS;
}

mlx_status
mlx_reg_access(
		IN mlx_utils *utils,
		IN mlx_uint16  reg_id,
		IN REG_ACCESS_OPT reg_opt,
		IN OUT mlx_void	*reg_data,
        IN mlx_size reg_size,
        OUT mlx_uint32 *reg_status
		)
{
	mlx_status status = MLX_SUCCESS;
	struct mail_box_tlv mail_box_tlv;

	if (utils == NULL || reg_data == NULL || reg_status == NULL
			|| reg_size > REG_ACCESS_MAX_REG_SIZE) {
		status = MLX_INVALID_PARAMETER;
		goto bad_param;
	}

	mlx_memory_set(utils, &mail_box_tlv, 0, sizeof(mail_box_tlv));

	init_operation_tlv(&mail_box_tlv, reg_id, reg_opt);

#define REG_ACCESS_TLV_REG 3
#define REG_TLV_HEADER_LEN 4
#define OP_TLV_SIZE 16
	mail_box_tlv.reg_tlv.Type = REG_ACCESS_TLV_REG;
	mail_box_tlv.reg_tlv.len  = ((reg_size + REG_TLV_HEADER_LEN + 3) >> 2); // length is in dwords round up
	mlx_memory_cpy(utils, &mail_box_tlv.reg_tlv.data, reg_data, reg_size);

	reg_size += OP_TLV_SIZE + REG_TLV_HEADER_LEN;

	status = mlx_icmd_send_command(utils, FLASH_REG_ACCESS, &mail_box_tlv, reg_size, reg_size);
	MLX_CHECK_STATUS(utils, status, icmd_err, "failed to send icmd");

	mlx_memory_cpy(utils, reg_data, &mail_box_tlv.reg_tlv.data,
			reg_size - (OP_TLV_SIZE + REG_TLV_HEADER_LEN));

	*reg_status = mail_box_tlv.operation_tlv.status;
icmd_err:
bad_param:
	return status;
}



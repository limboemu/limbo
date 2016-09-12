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

#ifndef MLX_REG_ACCESS_H_
#define MLX_REG_ACCESS_H_

#include "../../include/public/mlx_icmd.h"

#define REG_ACCESS_MAX_REG_SIZE 236

typedef enum {
  REG_ACCESS_READ = 1,
  REG_ACCESS_WRITE = 2,
} REG_ACCESS_OPT;

typedef enum {
  TLV_ACCESS_DEFAULT_DIS = 0,
  TLV_ACCESS_DEFAULT_EN = 1,
} NV_DEFAULT_OPT;

#define REG_ID_NVDA  0x9024
#define REG_ID_NVDI  0x9025
#define REG_ID_NVIA 0x9029
#define REG_ID_MLCR  0x902b
#define REG_ID_NVQC  0x9030
#define REG_ID_MFRL 0x9028
#define REG_ID_PTYS 0x5004
#define REG_ID_PMTU 0x5003

struct operation_tlv {
    mlx_uint32	reserved0	:8;    /* bit_offset:0 */    /* element_size: 8 */
    mlx_uint32	status		:7;    /* bit_offset:8 */    /* element_size: 7 */
    mlx_uint32	dr			:1;    /* bit_offset:15 */    /* element_size: 1 */
    mlx_uint32	len			:11;    /* bit_offset:16 */    /* element_size: 11 */
    mlx_uint32	Type		:5;    /* bit_offset:27 */    /* element_size: 5 */
    mlx_uint32	cls			:8;    /* bit_offset:32 */    /* element_size: 8 */
    mlx_uint32	method		:7;    /* bit_offset:40 */    /* element_size: 7 */
    mlx_uint32	r			:1;    /* bit_offset:47 */    /* element_size: 1 */
    mlx_uint32	register_id	:16;    /* bit_offset:48 */    /* element_size: 16 */
    mlx_uint64	tid			;    /* bit_offset:64 */    /* element_size: 64 */
};

struct reg_tlv {
	mlx_uint32	reserved0	:16;    /* bit_offset:0 */    /* element_size: 16 */
	mlx_uint32	len		:11;    /* bit_offset:16 */    /* element_size: 11 */
	mlx_uint32	Type		:5;    /* bit_offset:27 */    /* element_size: 5 */
	mlx_uint8	data[REG_ACCESS_MAX_REG_SIZE];
};

struct mail_box_tlv {
	struct operation_tlv operation_tlv;
	struct reg_tlv reg_tlv;
};
mlx_status
mlx_reg_access(
		IN mlx_utils *utils,
		IN mlx_uint16  reg_id,
		IN REG_ACCESS_OPT reg_opt,
		IN OUT mlx_void	*reg_data,
        IN mlx_size reg_size,
        OUT mlx_uint32 *reg_status
		);

#endif /* MLX_REG_ACCESS_H_ */

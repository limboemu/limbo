#ifndef MLX_LINK_SPEED_H_
#define MLX_LINK_SPEED_H_

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
#include "../../include/public/mlx_utils.h"

#define LINK_SPEED_100GB_MASK        (ETH_SPEED_ENABLE_MASK_100GBASECR4 | ETH_SPEED_ENABLE_MASK_100GBASESR4 | ETH_SPEED_ENABLE_MASK_100GBASEKR4 | ETH_SPEED_ENABLE_MASK_100GBASELR4)
#define LINK_SPEED_56GB_MASK         (ETH_SPEED_ENABLE_MASK_56GBASER4)
#define LINK_SPEED_50GB_MASK         (ETH_SPEED_ENABLE_MASK_50GBASECR2 | ETH_SPEED_ENABLE_MASK_50GBASEKR2)
#define LINK_SPEED_40GB_MASK         (ETH_SPEED_ENABLE_MASK_40GBASECR4 | ETH_SPEED_ENABLE_MASK_40GBASEKR4 | ETH_SPEED_ENABLE_MASK_40GBASESR4 | ETH_SPEED_ENABLE_MASK_40GBASELR4)
#define LINK_SPEED_25GB_MASK         (ETH_SPEED_ENABLE_MASK_25GBASECR | ETH_SPEED_ENABLE_MASK_25GBASEKR | ETH_SPEED_ENABLE_MASK_25GBASESR)
#define LINK_SPEED_20GB_MASK         (ETH_SPEED_ENABLE_MASK_20GBASER2)
#define LINK_SPEED_10GB_MASK         (ETH_SPEED_ENABLE_MASK_10GBASECR | ETH_SPEED_ENABLE_MASK_10GBASESR | ETH_SPEED_ENABLE_MASK_10GBASELR | ETH_SPEED_ENABLE_MASK_10GBASEKR)
#define LINK_SPEED_1GB_MASK          (ETH_SPEED_ENABLE_MASK_1000BASECX | ETH_SPEED_ENABLE_MASK_1000BASEKX | ETH_SPEED_ENABLE_MASK_100BaseTX | ETH_SPEED_ENABLE_MASK_1000BASET)

#define LINK_SPEED_SDR_MASK 0x1
#define LINK_SPEED_DDR_MASK 0x2
#define LINK_SPEED_QDR_MASK 0xC
#define LINK_SPEED_FDR_MASK 0x10
#define LINK_SPEED_EDR20_MASK 0x200
#define LINK_SPEED_EDR_MASK 0x20

#define LINK_SPEED_WITDH_1_MASK 0x1
#define LINK_SPEED_WITDH_2_MASK 0x2
#define LINK_SPEED_WITDH_4_MASK 0x4
#define LINK_SPEED_WITDH_8_MASK 0x8
#define LINK_SPEED_WITDH_12_MASK 0x10

#define GIGA_TO_BIT 0x40000000

enum {
    ETH_SPEED_ENABLE_MASK_1000BASECX  = 0x0001,
    ETH_SPEED_ENABLE_MASK_1000BASEKX  = 0x0002,
    ETH_SPEED_ENABLE_MASK_10GBASECX4  = 0x0004,
    ETH_SPEED_ENABLE_MASK_10GBASEKX4  = 0x0008,
    ETH_SPEED_ENABLE_MASK_10GBASEKR   = 0x0010,
    ETH_SPEED_ENABLE_MASK_20GBASER2   = 0x0020,
    ETH_SPEED_ENABLE_MASK_40GBASECR4  = 0x0040,
    ETH_SPEED_ENABLE_MASK_40GBASEKR4  = 0x0080,
    ETH_SPEED_ENABLE_MASK_56GBASER4   = 0x0100,
    ETH_SPEED_ENABLE_MASK_10GBASECR   = 0x1000,
    ETH_SPEED_ENABLE_MASK_10GBASESR   = 0x2000,
    ETH_SPEED_ENABLE_MASK_10GBASELR   = 0x4000,
    ETH_SPEED_ENABLE_MASK_40GBASESR4  = 0x8000,
    ETH_SPEED_ENABLE_MASK_40GBASELR4  = 0x10000,
    ETH_SPEED_ENABLE_MASK_50GBASEKR4  = 0x80000,
    ETH_SPEED_ENABLE_MASK_100GBASECR4 = 0x100000,
    ETH_SPEED_ENABLE_MASK_100GBASESR4 = 0x200000,
    ETH_SPEED_ENABLE_MASK_100GBASEKR4 = 0x400000,
    ETH_SPEED_ENABLE_MASK_100GBASELR4 = 0x800000,
    ETH_SPEED_ENABLE_MASK_100BaseTX   = 0x1000000,
    ETH_SPEED_ENABLE_MASK_1000BASET   = 0x2000000,
    ETH_SPEED_ENABLE_MASK_10GBASET    = 0x4000000,
    ETH_SPEED_ENABLE_MASK_25GBASECR   = 0x8000000,
    ETH_SPEED_ENABLE_MASK_25GBASEKR   = 0x10000000,
    ETH_SPEED_ENABLE_MASK_25GBASESR   = 0x20000000,
    ETH_SPEED_ENABLE_MASK_50GBASECR2  = 0x40000000,
    ETH_SPEED_ENABLE_MASK_50GBASEKR2  = 0x80000000,
    ETH_SPEED_ENABLE_MASK_BAD         = 0xffff,
};


typedef enum {
	LINK_SPEED_IB = 0,
	LINK_SPEED_FC,
	LINK_SPEED_ETH,
} LINK_SPEED_TYPE;

typedef enum {
	LINK_SPEED_1GB = 0,
	LINK_SPEED_10GB,
	LINK_SPEED_40GB,
	LINK_SPEED_100GB,
	LINK_SPEED_SDR,
	LINK_SPEED_DEFAULT,
} LINK_SPEED;

struct mlx_link_speed {
	mlx_uint32 proto_mask	:3;
	mlx_uint32 reserved1	:13;
	mlx_uint32 loacl_port	:8;
	mlx_uint32 reserved2	:8;
	/* -------------- */
	mlx_uint32 reserved3	:32;
	/* -------------- */
	mlx_uint32 reserved4	:32;
	/* -------------- */
	mlx_uint32 eth_proto_capability	:32;
	/* -------------- */
	mlx_uint32 ib_proto_capability	:16;
	mlx_uint32 ib_link_width_capability	:16;
	/* -------------- */
	mlx_uint32 reserved5	:32;
	/* -------------- */
	mlx_uint32 eth_proto_admin	:32;
	/* -------------- */
	mlx_uint32 ib_proto_admin	:16;
	mlx_uint32 ib_link_width_admin	:16;
	/* -------------- */
	mlx_uint32 reserved6	:32;
	/* -------------- */
	mlx_uint32 eth_proto_oper	:32;
	/* -------------- */
	mlx_uint32 ib_proto_oper	:16;
	mlx_uint32 ib_link_width_oper	:16;
};

mlx_status
mlx_set_link_speed(
		IN mlx_utils *utils,
		IN mlx_uint8 port_num,
		IN LINK_SPEED_TYPE type,
		IN LINK_SPEED speed
		);

mlx_status
mlx_get_max_speed(
		IN mlx_utils *utils,
		IN mlx_uint8 port_num,
		IN LINK_SPEED_TYPE type,
		OUT mlx_uint64 *speed
		);

#endif /* MLX_LINK_SPEED_H_ */

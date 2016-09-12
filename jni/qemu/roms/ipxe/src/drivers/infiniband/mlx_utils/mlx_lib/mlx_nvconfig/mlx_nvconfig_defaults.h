#ifndef MLX_NVCONFIG_DEFAULTS_H_
#define MLX_NVCONFIG_DEFAULTS_H_

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
#include "mlx_nvconfig_prm.h"
/*
 * Default values
 */
#define DEFAULT_FLEXBOOT_MENU_TO 4
#define DEFAULT_MAX_VFS 8
#define DEFAULT_BOOT_PROTOCOL 1
#define DEFAULT_OPTION_ROM_EN 1
#define DEFAULT_BOOT_VLAN 1
#define DEFAULT_ISCSI_DHCP_PARAM_EN 1
#define DEFAULT_ISCSI_IPV4_DHCP_EN 1
#define DEFAULT_OCSD_OCBB_EN 1
#define DEFAULT_BOOT_IP_VER 0
#define DEFAULT_BOOT_LINK_UP_TO 0

struct mlx_nvconfig_port_conf_defaults {
	mlx_uint8 pptx;
	mlx_uint8 pprx;
	mlx_boolean boot_option_rom_en;
	mlx_boolean boot_vlan_en;
	mlx_uint8 boot_retry_count;
	mlx_uint8 boot_protocol;
	mlx_uint8 boot_vlan;
	mlx_uint8 boot_pkey;
	mlx_boolean en_wol_magic;
	mlx_uint8 network_link_type;
	mlx_uint8 iscsi_boot_to_target;
	mlx_boolean iscsi_vlan_en;
	mlx_boolean iscsi_tcp_timestamps_en;
	mlx_boolean iscsi_chap_mutual_auth_en;
	mlx_boolean iscsi_chap_auth_en;
	mlx_boolean iscsi_dhcp_params_en;
	mlx_boolean iscsi_ipv4_dhcp_en;
	mlx_uint8 iscsi_lun_busy_retry_count;
	mlx_uint8 iscsi_link_up_delay_time;
	mlx_uint8 client_identifier;
	mlx_uint8 mac_admin_bit;
	mlx_uint8 default_link_type;
	mlx_uint8 linkup_timeout;
	mlx_uint8 ip_ver;
};

struct mlx_nvconfig_conf_defaults  {
	mlx_uint8 max_vfs;
	mlx_uint8 total_vfs;
	mlx_uint8 sriov_en;
	mlx_uint8 maximum_uar_bar_size;
	mlx_uint8 uar_bar_size;
	mlx_uint8 flexboot_menu_to;
	mlx_boolean ocsd_ocbb_en;
};

mlx_status
nvconfig_read_port_default_values(
		IN mlx_utils *utils,
		IN mlx_uint8 port,
		OUT struct mlx_nvconfig_port_conf_defaults *port_conf_def
		);

mlx_status
nvconfig_read_general_default_values(
		IN mlx_utils *utils,
		OUT struct mlx_nvconfig_conf_defaults *conf_def
		);

mlx_status
nvconfig_read_rom_ini_values(
		IN mlx_utils *utils,
		OUT struct mlx_nvcofnig_romini *rom_ini
		);
#endif /* MLX_NVCONFIG_DEFAULTS_H_ */

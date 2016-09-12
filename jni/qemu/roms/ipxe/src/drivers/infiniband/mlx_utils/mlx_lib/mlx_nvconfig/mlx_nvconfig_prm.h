#ifndef MLX_NVCONFIG_PRM_H_
#define MLX_NVCONFIG_PRM_H_

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

#include "../../include/public/mlx_types.h"

enum {
	WAKE_ON_LAN_TYPE				= 0x10,
	VIRTUALIZATION_TYPE				= 0x11,
	VPI_LINK_TYPE					= 0x12,
	BOOT_SETTINGS_EXT_TYPE			= 0x2001,
	BANNER_TO_TYPE 					= 0x2010,
	OCSD_OCBB_TYPE 					= 0x2011,
	FLOW_CONTROL_TYPE				= 0x2020,
	BOOT_SETTINGS_TYPE				= 0x2021,
	ISCSI_GENERAL_SETTINGS_TYPE		= 0x2100,
	IB_BOOT_SETTING_TYPE			= 0x2022,
	IB_DHCP_SETTINGS_TYPE			= 0x2023,
	GLOPAL_PCI_SETTINGS_TYPE		= 0x80,
	GLOPAL_PCI_CAPS_TYPE			= 0x81,
	GLOBAL_ROM_INI_TYPE				= 0x100,

	// Types for iSCSI strings
	DHCP_VEND_ID					= 0x2101,
	ISCSI_INITIATOR_IPV4_ADDR		= 0x2102,
	ISCSI_INITIATOR_SUBNET			= 0x2103,
	ISCSI_INITIATOR_IPV4_GATEWAY	= 0x2104,
	ISCSI_INITIATOR_IPV4_PRIM_DNS	= 0x2105,
	ISCSI_INITIATOR_IPV4_SECDNS		= 0x2106,
	ISCSI_INITIATOR_NAME			= 0x2107,
	ISCSI_INITIATOR_CHAP_ID			= 0x2108,
	ISCSI_INITIATOR_CHAP_PWD		= 0x2109,
	ISCSI_INITIATOR_DHCP_CONF_TYPE	= 0x210a,

	CONNECT_FIRST_TGT				= 0x2200,
	FIRST_TGT_IP_ADDRESS			= 0x2201,
	FIRST_TGT_TCP_PORT				= 0x2202,
	FIRST_TGT_BOOT_LUN				= 0x2203,
	FIRST_TGT_ISCSI_NAME			= 0x2204,
	FIRST_TGT_CHAP_ID				= 0x2205,
	FIRST_TGT_CHAP_PWD				= 0x2207,
};

union mlx_nvconfig_nic_boot_conf {
	struct {
		mlx_uint32	vlan_id				: 12;
		mlx_uint32	link_speed			: 4;
		mlx_uint32	legacy_boot_prot	: 8;
		mlx_uint32	boot_retry_count	: 3;
		mlx_uint32	boot_strap_type		: 3;
		mlx_uint32	en_vlan				: 1;
		mlx_uint32	en_option_rom		: 1;
	};
	mlx_uint32 dword;
};

union mlx_nvconfig_nic_boot_ext_conf {
	struct {
		mlx_uint32	linkup_timeout	: 8;
		mlx_uint32	ip_ver			: 2;
		mlx_uint32	reserved0		: 22;
	};
	mlx_uint32 dword;
};

union mlx_nvconfig_rom_banner_timeout_conf {
	struct {
		mlx_uint32	rom_banner_to	: 4;
		mlx_uint32	reserved		: 28;
	};
	mlx_uint32 dword;
};

union mlx_nvconfig_virt_conf {
	struct {
		mlx_uint32 reserved0				:24;
		mlx_uint32 pf_bar_size_valid		:1;
		mlx_uint32 vf_bar_size_valid		:1;
		mlx_uint32 num_pf_msix_valid		:1;
		mlx_uint32 num_vf_msix_valid		:1;
		mlx_uint32 num_pfs_valid			:1;
		mlx_uint32 fpp_valid				:1;
		mlx_uint32 full_vf_qos_valid		:1;
		mlx_uint32 sriov_valid			:1;
		/*-------------------*/
		mlx_uint32 num_of_vfs				:16;
		mlx_uint32 num_of_pfs				:4;
		mlx_uint32 reserved1				:9;
		mlx_uint32 fpp_en					:1;
		mlx_uint32 full_vf_qos			:1;
		mlx_uint32 virt_mode				:1; //sriov_en
		/*-------------------*/
		mlx_uint32 log_pf_uar_bar_size	:6;
		mlx_uint32 log_vf_uar_bar_size	:6;
		mlx_uint32 num_pf_msix			:10;
		mlx_uint32 num_vf_msix			:10;
	};
	mlx_uint32 dword[3];
};

union mlx_nvconfig_virt_caps {
	struct {
		mlx_uint32 reserved0				:24;
		mlx_uint32 max_vfs_per_pf_valid	:1;
		mlx_uint32 max_total_msix_valid	:1;
		mlx_uint32 max_total_bar_valid	:1;
		mlx_uint32 num_pfs_supported		:1;
		mlx_uint32 num_vf_msix_supported	:1;
		mlx_uint32 num_pf_msix_supported	:1;
		mlx_uint32 vf_bar_size_supported	:1;
		mlx_uint32 pf_bar_size_supported	:1;
		/*-------------------*/
		mlx_uint32 max_vfs_per_pf			:16;
		mlx_uint32 max_num_pfs			:4;
		mlx_uint32 reserved1				:9;
		mlx_uint32 fpp_support			:1;
		mlx_uint32 vf_qos_control_support	:1;
		mlx_uint32 sriov_support			:1;
		/*-------------------*/
		mlx_uint32 max_log_pf_uar_bar_size	:6;
		mlx_uint32 max_log_vf_uar_bar_size	:6;
		mlx_uint32 max_num_pf_msix			:10;
		mlx_uint32 max_num_vf_msix			:10;
		/*-------------------*/
		mlx_uint32 max_total_msix;
		/*-------------------*/
		mlx_uint32 max_total_bar;
	};
	mlx_uint32 dword[5];
};

union mlx_nvconfig_iscsi_init_dhcp_conf {
	struct {
		mlx_uint32 reserved0		:30;
		mlx_uint32 dhcp_iscsi_en	:1;
		mlx_uint32 ipv4_dhcp_en	:1;

	};
	mlx_uint32 dword;
};

union mlx_nvconfig_nic_ib_boot_conf {
	struct {
		mlx_uint32	boot_pkey			: 16;
		mlx_uint32	reserved0			: 16;
	};
	mlx_uint32 dword;
};

union mlx_nvconfig_wol_conf {
	struct {
		mlx_uint32	reserved0		:9;
		mlx_uint32	en_wol_passwd	:1;
		mlx_uint32	en_wol_magic	:1;
		mlx_uint32	reserved1		:21;
		mlx_uint32	reserved2		:32;
	};
	mlx_uint32 dword[2];
};

union mlx_nvconfig_iscsi_general {
	struct {
		mlx_uint32	reserved0			:22;
		mlx_uint32	boot_to_target		:2;
		mlx_uint32	reserved1			:2;
		mlx_uint32	vlan_en				:1;
		mlx_uint32	tcp_timestamps_en	:1;
		mlx_uint32	chap_mutual_auth_en	:1;
		mlx_uint32	chap_auth_en		:1;
		mlx_uint32	reserved2			:2;
		/*-------------------*/
		mlx_uint32	vlan				:12;
		mlx_uint32	reserved3			:20;
		/*-------------------*/
		mlx_uint32	lun_busy_retry_count:8;
		mlx_uint32	link_up_delay_time	:8;
		mlx_uint32	reserved4			:16;
	};
	mlx_uint32 dword[3];
};

union mlx_nvconfig_ib_dhcp_conf {
	struct {
		mlx_uint32 reserved			:24;
		mlx_uint32 client_identifier	:4;
		mlx_uint32 mac_admin_bit		:4;
	};
	mlx_uint32 dword;
};

union mlx_nvconfig_ocsd_ocbb_conf {
	struct {
		mlx_uint32	reserved		:31;
		mlx_uint32	ocsd_ocbb_en	:1;
	};
	mlx_uint32 dword;
};

union mlx_nvconfig_vpi_link_conf {
	struct {
		mlx_uint32	network_link_type	:2;
		mlx_uint32	default_link_type	:2;
		mlx_uint32	reserved		:28;
	};
	mlx_uint32 dword;
};

struct  mlx_nvcofnig_romini {
	mlx_uint32 reserved0    :1;
	mlx_uint32 shared_memory_en     :1;
	mlx_uint32 hii_vpi_en   :1;
	mlx_uint32 tech_enum    :1;
	mlx_uint32 reserved1    :4;
	mlx_uint32 static_component_name_string :1;
	mlx_uint32 hii_iscsi_configuration      :1;
	mlx_uint32 hii_ibm_aim  :1;
	mlx_uint32 hii_platform_setup   :1;
	mlx_uint32 hii_bdf_decimal      :1;
	mlx_uint32 hii_read_only        :1;
	mlx_uint32 reserved2    :10;
	mlx_uint32 mac_enum             :1;
	mlx_uint32 port_enum    :1;
	mlx_uint32 flash_en             :1;
	mlx_uint32 fmp_en               :1;
	mlx_uint32 bofm_en              :1;
	mlx_uint32 platform_to_driver_en                :1;
	mlx_uint32 hii_en               :1;
	mlx_uint32 undi_en              :1;
	/* -------------- */
	mlx_uint64 dhcp_user_class;
	/* -------------- */
	mlx_uint32 reserved3    :22;
	mlx_uint32 uri_boot_retry_delay :4;
	mlx_uint32 uri_boot_retry       :4;
	mlx_uint32 option_rom_debug     :1;
	mlx_uint32 promiscuous_vlan     :1;
};

#endif /* MLX_NVCONFIG_PRM_H_ */

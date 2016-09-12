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

FILE_LICENCE( GPL2_OR_LATER);

#include "../../mlx_lib/mlx_nvconfig/mlx_nvconfig.h"
#include "../../include/public/mlx_memory.h"
#include "../../include/public/mlx_bail.h"
#include "../../mlx_lib/mlx_nvconfig/mlx_nvconfig_defaults.h"

struct tlv_default {
	mlx_uint16 tlv_type;
	mlx_size data_size;
	mlx_status (*set_defaults)( IN void *data, IN int status,
			OUT void *def_struct);
};

#define TlvDefaultEntry( _tlv_type, _data_size, _set_defaults) { \
  .tlv_type = _tlv_type,                     \
  .data_size = sizeof ( _data_size ),                   \
  .set_defaults = _set_defaults,                  \
  }

static
mlx_status
nvconfig_get_boot_default_conf(
		IN void *data,
		IN int status,
		OUT void *def_struct
		)
{
	union mlx_nvconfig_nic_boot_conf *nic_boot_conf =
			(union mlx_nvconfig_nic_boot_conf *) data;
	struct mlx_nvconfig_port_conf_defaults *port_conf_def =
			(struct mlx_nvconfig_port_conf_defaults *) def_struct;

	/* boot_option_rom_en is deprecated - enabled always */
	port_conf_def->boot_option_rom_en = DEFAULT_OPTION_ROM_EN;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"TLV not found. Using hard-coded defaults ");
	port_conf_def->boot_vlan = nic_boot_conf->vlan_id;
	port_conf_def->boot_protocol = nic_boot_conf->legacy_boot_prot;
	port_conf_def->boot_retry_count = nic_boot_conf->boot_retry_count;
	port_conf_def->boot_vlan_en = nic_boot_conf->en_vlan;

	return MLX_SUCCESS;

nvdata_access_err:
	port_conf_def->boot_vlan = DEFAULT_BOOT_VLAN;
	port_conf_def->boot_protocol = DEFAULT_BOOT_PROTOCOL;

	return status;
}

static
mlx_status
nvconfig_get_boot_ext_default_conf(
		IN void *data,
		IN int status,
		OUT void *def_struct
		)
{
	union mlx_nvconfig_nic_boot_ext_conf *nic_boot_ext_conf =
			(union mlx_nvconfig_nic_boot_ext_conf *) data;
	struct mlx_nvconfig_port_conf_defaults *port_conf_def =
			(struct mlx_nvconfig_port_conf_defaults *) def_struct;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"TLV not found. Using hard-coded defaults ");
	port_conf_def->linkup_timeout = nic_boot_ext_conf->linkup_timeout;
	port_conf_def->ip_ver = nic_boot_ext_conf->ip_ver;

	return MLX_SUCCESS;

nvdata_access_err:
	port_conf_def->linkup_timeout = DEFAULT_BOOT_LINK_UP_TO;
	port_conf_def->ip_ver = DEFAULT_BOOT_IP_VER;

	return status;
}

static
mlx_status
nvconfig_get_iscsi_init_dhcp_default_conf(
		IN void *data,
		IN int status,
		OUT void *def_struct
		)
{
	union mlx_nvconfig_iscsi_init_dhcp_conf *iscsi_init_dhcp_conf =
			(union mlx_nvconfig_iscsi_init_dhcp_conf *) data;
	struct mlx_nvconfig_port_conf_defaults *port_conf_def =
			(struct mlx_nvconfig_port_conf_defaults *) def_struct;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"TLV not found. Using hard-coded defaults ");
	port_conf_def->iscsi_dhcp_params_en = iscsi_init_dhcp_conf->dhcp_iscsi_en;
	port_conf_def->iscsi_ipv4_dhcp_en = iscsi_init_dhcp_conf->ipv4_dhcp_en;

	return MLX_SUCCESS;

nvdata_access_err:
	port_conf_def->iscsi_dhcp_params_en = DEFAULT_ISCSI_DHCP_PARAM_EN;
	port_conf_def->iscsi_ipv4_dhcp_en = DEFAULT_ISCSI_IPV4_DHCP_EN;

	return status;
}

static
mlx_status
nvconfig_get_ib_boot_default_conf(
		IN void *data,
		IN int status,
		OUT void *def_struct
		)
{
	union mlx_nvconfig_nic_ib_boot_conf *ib_boot_conf =
			(union mlx_nvconfig_nic_ib_boot_conf *) data;
	struct mlx_nvconfig_port_conf_defaults *port_conf_def =
			(struct mlx_nvconfig_port_conf_defaults *) def_struct;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"nvconfig_nvdata_default_access failed ");
	port_conf_def->boot_pkey = ib_boot_conf->boot_pkey;

nvdata_access_err:
	return status;
}

static
mlx_status
nvconfig_get_wol_default_conf(
		IN void *data,
		IN int status,
		OUT void *def_struct
		)
{
	union mlx_nvconfig_wol_conf *wol_conf = (union mlx_nvconfig_wol_conf *) data;
	struct mlx_nvconfig_port_conf_defaults *port_conf_def =
			(struct mlx_nvconfig_port_conf_defaults *) def_struct;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"nvconfig_nvdata_default_access failed ");
	port_conf_def->en_wol_magic = wol_conf->en_wol_magic;

nvdata_access_err:
	return status;
}

static
mlx_status
nvconfig_get_iscsi_gen_default_conf(
		IN void *data,
		IN int status,
		OUT void *def_struct)
{
	union mlx_nvconfig_iscsi_general *iscsi_gen =
			(union mlx_nvconfig_iscsi_general *) data;
	struct mlx_nvconfig_port_conf_defaults *port_conf_def =
			(struct mlx_nvconfig_port_conf_defaults *) def_struct;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"nvconfig_nvdata_default_access failed ");
	port_conf_def->iscsi_boot_to_target = iscsi_gen->boot_to_target;
	port_conf_def->iscsi_vlan_en = iscsi_gen->vlan_en;
	port_conf_def->iscsi_tcp_timestamps_en = iscsi_gen->tcp_timestamps_en;
	port_conf_def->iscsi_chap_mutual_auth_en = iscsi_gen->chap_mutual_auth_en;
	port_conf_def->iscsi_chap_auth_en = iscsi_gen->chap_auth_en;
	port_conf_def->iscsi_lun_busy_retry_count = iscsi_gen->lun_busy_retry_count;
	port_conf_def->iscsi_link_up_delay_time = iscsi_gen->link_up_delay_time;

nvdata_access_err:
	return status;
}

static
mlx_status
nvconfig_get_ib_dhcp_default_conf(
		IN void *data,
		IN int status,
		OUT void *def_struct
		)
{
	union mlx_nvconfig_ib_dhcp_conf *ib_dhcp =
			(union mlx_nvconfig_ib_dhcp_conf *) data;
	struct mlx_nvconfig_port_conf_defaults *port_conf_def =
			(struct mlx_nvconfig_port_conf_defaults *) def_struct;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"nvconfig_nvdata_default_access failed ");
	port_conf_def->client_identifier = ib_dhcp->client_identifier;
	port_conf_def->mac_admin_bit = ib_dhcp->mac_admin_bit;

nvdata_access_err:
	return status;
}

static
mlx_status
nvconfig_get_ocsd_ocbb_default_conf( IN void *data,
		IN int status, OUT void *def_struct) {
	union mlx_nvconfig_ocsd_ocbb_conf *ocsd_ocbb =
			(union mlx_nvconfig_ocsd_ocbb_conf *) data;
	struct mlx_nvconfig_conf_defaults *conf_def =
			(struct mlx_nvconfig_conf_defaults *) def_struct;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"TLV not found. Using hard-coded defaults ");
	conf_def->ocsd_ocbb_en = ocsd_ocbb->ocsd_ocbb_en;

	return MLX_SUCCESS;

nvdata_access_err:
	conf_def->ocsd_ocbb_en = DEFAULT_OCSD_OCBB_EN;

	return status;
}

static
mlx_status
nvconfig_get_vpi_link_default_conf(
		IN void *data,
		IN int status,
		OUT void *def_struct
		)
{
	union mlx_nvconfig_vpi_link_conf *vpi_link =
			(union mlx_nvconfig_vpi_link_conf *) data;
	struct mlx_nvconfig_port_conf_defaults *port_conf_def =
			(struct mlx_nvconfig_port_conf_defaults *) def_struct;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"nvconfig_nvdata_default_access failed ");
	port_conf_def->network_link_type = vpi_link->network_link_type;
	port_conf_def->default_link_type = vpi_link->default_link_type;

nvdata_access_err:
	return status;
}

static
mlx_status
nvconfig_get_rom_banner_to_default_conf(
		IN void *data,
		IN int status,
		OUT void *def_struct
		)
{
	union mlx_nvconfig_rom_banner_timeout_conf *rom_banner_timeout_conf =
			(union mlx_nvconfig_rom_banner_timeout_conf *) data;
	struct mlx_nvconfig_conf_defaults *conf_def =
			(struct mlx_nvconfig_conf_defaults *) def_struct;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"TLV not found. Using hard-coded defaults ");
	conf_def->flexboot_menu_to = rom_banner_timeout_conf->rom_banner_to;

	return MLX_SUCCESS;

nvdata_access_err:
	conf_def->flexboot_menu_to = DEFAULT_FLEXBOOT_MENU_TO;

	return status;
}

static
mlx_status
nvconfig_get_nv_virt_caps_default_conf(
		IN void *data,
		IN int status,
		OUT void *def_struct
		)
{
	union mlx_nvconfig_virt_caps *nv_virt_caps =
			(union mlx_nvconfig_virt_caps *) data;
	struct mlx_nvconfig_conf_defaults *conf_def =
			(struct mlx_nvconfig_conf_defaults *) def_struct;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"TLV not found. Using hard-coded defaults ");
	conf_def->max_vfs = nv_virt_caps->max_vfs_per_pf;

	return MLX_SUCCESS;

nvdata_access_err:
	conf_def->max_vfs = DEFAULT_MAX_VFS;

	return status;
}

static
mlx_status
nvconfig_get_nv_virt_default_conf(
		IN void *data,
		IN int status,
		OUT void *def_struct
		)
{
	union mlx_nvconfig_virt_conf *nv_virt_conf =
			(union mlx_nvconfig_virt_conf *) data;
	struct mlx_nvconfig_conf_defaults *conf_def =
			(struct mlx_nvconfig_conf_defaults *) def_struct;

	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
			"nvconfig_nvdata_default_access failed ");
	conf_def->total_vfs = nv_virt_conf->num_of_vfs;
	conf_def->sriov_en = nv_virt_conf->virt_mode;

nvdata_access_err:
	return status;
}

static struct tlv_default tlv_port_defaults[] = {
	TlvDefaultEntry(BOOT_SETTINGS_TYPE, union mlx_nvconfig_nic_boot_conf, &nvconfig_get_boot_default_conf),
	TlvDefaultEntry(BOOT_SETTINGS_EXT_TYPE, union mlx_nvconfig_nic_boot_ext_conf, &nvconfig_get_boot_ext_default_conf),
	TlvDefaultEntry(ISCSI_INITIATOR_DHCP_CONF_TYPE, union mlx_nvconfig_iscsi_init_dhcp_conf, &nvconfig_get_iscsi_init_dhcp_default_conf),
	TlvDefaultEntry(IB_BOOT_SETTING_TYPE, union mlx_nvconfig_nic_ib_boot_conf, &nvconfig_get_ib_boot_default_conf),
	TlvDefaultEntry(WAKE_ON_LAN_TYPE, union mlx_nvconfig_wol_conf, &nvconfig_get_wol_default_conf),
	TlvDefaultEntry(ISCSI_GENERAL_SETTINGS_TYPE, union mlx_nvconfig_iscsi_general, &nvconfig_get_iscsi_gen_default_conf),
	TlvDefaultEntry(IB_DHCP_SETTINGS_TYPE, union mlx_nvconfig_ib_dhcp_conf, &nvconfig_get_ib_dhcp_default_conf),
	TlvDefaultEntry(VPI_LINK_TYPE, union mlx_nvconfig_vpi_link_conf, &nvconfig_get_vpi_link_default_conf),
};

static struct tlv_default tlv_general_defaults[] = {
	TlvDefaultEntry(BANNER_TO_TYPE, union mlx_nvconfig_rom_banner_timeout_conf, &nvconfig_get_rom_banner_to_default_conf),
	TlvDefaultEntry(GLOPAL_PCI_CAPS_TYPE, union mlx_nvconfig_virt_caps, &nvconfig_get_nv_virt_caps_default_conf),
	TlvDefaultEntry(GLOPAL_PCI_SETTINGS_TYPE, union mlx_nvconfig_virt_conf, &nvconfig_get_nv_virt_default_conf),
	TlvDefaultEntry(OCSD_OCBB_TYPE, union mlx_nvconfig_ocsd_ocbb_conf, &nvconfig_get_ocsd_ocbb_default_conf),
};

static
mlx_status
nvconfig_nvdata_default_access(
		IN mlx_utils *utils,
		IN mlx_uint8 port,
		IN mlx_uint16 tlv_type,
		IN mlx_size data_size,
		OUT mlx_void *data
		)
{
	mlx_status status = MLX_SUCCESS;
	mlx_uint32 index;
	mlx_uint8 version = 0;

	status = nvconfig_nvdata_access(utils, port, tlv_type, REG_ACCESS_READ,
			data_size, TLV_ACCESS_DEFAULT_EN, &version, data);
	MLX_CHECK_STATUS(NULL, status, nvdata_access_err,
				"nvconfig_nvdata_access failed ");
	for (index = 0; index * 4 < data_size; index++) {
		mlx_memory_be32_to_cpu(utils, (((mlx_uint32 *) data)[index]),
				((mlx_uint32 *) data) + index);
	}

nvdata_access_err:
	return status;
}

static
mlx_status
nvconfig_nvdata_read_default_value(
		IN mlx_utils *utils,
		IN mlx_uint8 modifier,
		IN struct tlv_default *def,
		OUT void *def_struct
		)
{
	mlx_status status = MLX_SUCCESS;
	void *data = NULL;

	status = mlx_memory_zalloc(utils, def->data_size,&data);
	MLX_CHECK_STATUS(utils, status, memory_err,
				"mlx_memory_zalloc failed ");
	status = nvconfig_nvdata_default_access(utils, modifier, def->tlv_type,
			def->data_size, data);
	def->set_defaults(data, status, def_struct);
	mlx_memory_free(utils, &data);

memory_err:
	return status;
}

static
void
nvconfig_nvdata_read_default_values(
		IN mlx_utils *utils,
		IN mlx_uint8 modifier,
		IN struct tlv_default defaults_table[],
		IN mlx_uint8 defaults_table_size,
		OUT void *def_strct
		)
{
	struct tlv_default *defs;
	unsigned int i;

	for (i = 0; i < defaults_table_size; i++) {
		defs = &defaults_table[i];
		nvconfig_nvdata_read_default_value(utils, modifier, defs, def_strct);
	}
}

mlx_status
nvconfig_read_port_default_values(
		IN mlx_utils *utils,
		IN mlx_uint8 port,
		OUT struct mlx_nvconfig_port_conf_defaults *port_conf_def
		)
{
	mlx_status status = MLX_SUCCESS;

	if (utils == NULL || port_conf_def == NULL) {
		status = MLX_INVALID_PARAMETER;
		MLX_DEBUG_ERROR(utils,"bad params.");
		goto bad_param;
	}
	mlx_memory_set(utils, port_conf_def, 0, sizeof(*port_conf_def));
	nvconfig_nvdata_read_default_values(utils, port, tlv_port_defaults,
				(sizeof(tlv_port_defaults)/sizeof(tlv_port_defaults[0])),
				port_conf_def);

bad_param:
	return status;
}

mlx_status
nvconfig_read_general_default_values(
		IN mlx_utils *utils,
		OUT struct mlx_nvconfig_conf_defaults *conf_def
		)
{
	mlx_status status = MLX_SUCCESS;

	if (utils == NULL || conf_def == NULL) {
		status = MLX_INVALID_PARAMETER;
		MLX_DEBUG_ERROR(utils,"bad params.");
		goto bad_param;
	}
	mlx_memory_set(utils, conf_def, 0, sizeof(*conf_def));
	nvconfig_nvdata_read_default_values(utils, 0, tlv_general_defaults,
			(sizeof(tlv_general_defaults)/sizeof(tlv_general_defaults[0])),
			conf_def);

bad_param:
	return status;
}

mlx_status
nvconfig_read_rom_ini_values(
		IN mlx_utils *utils,
		OUT struct mlx_nvcofnig_romini *rom_ini
		)
{
	mlx_status status = MLX_SUCCESS;

	if (utils == NULL || rom_ini == NULL) {
		status = MLX_INVALID_PARAMETER;
		MLX_DEBUG_ERROR(utils,"bad params.");
		goto bad_param;
	}
	mlx_memory_set(utils, rom_ini, 0, sizeof(*rom_ini));

	status = nvconfig_nvdata_default_access(utils, 0, GLOBAL_ROM_INI_TYPE,
			sizeof(*rom_ini), rom_ini);
bad_param:
	return status;
}

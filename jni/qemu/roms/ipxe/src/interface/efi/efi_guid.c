/*
 * Copyright (C) 2014 Michael Brown <mbrown@fensystems.co.uk>.
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
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/efi/efi.h>
#include <ipxe/efi/Protocol/AbsolutePointer.h>
#include <ipxe/efi/Protocol/AppleNetBoot.h>
#include <ipxe/efi/Protocol/Arp.h>
#include <ipxe/efi/Protocol/BlockIo.h>
#include <ipxe/efi/Protocol/BusSpecificDriverOverride.h>
#include <ipxe/efi/Protocol/ComponentName.h>
#include <ipxe/efi/Protocol/ComponentName2.h>
#include <ipxe/efi/Protocol/ConsoleControl/ConsoleControl.h>
#include <ipxe/efi/Protocol/DevicePath.h>
#include <ipxe/efi/Protocol/DevicePathToText.h>
#include <ipxe/efi/Protocol/Dhcp4.h>
#include <ipxe/efi/Protocol/DiskIo.h>
#include <ipxe/efi/Protocol/DriverBinding.h>
#include <ipxe/efi/Protocol/GraphicsOutput.h>
#include <ipxe/efi/Protocol/HiiConfigAccess.h>
#include <ipxe/efi/Protocol/HiiFont.h>
#include <ipxe/efi/Protocol/Ip4.h>
#include <ipxe/efi/Protocol/Ip4Config.h>
#include <ipxe/efi/Protocol/LoadFile.h>
#include <ipxe/efi/Protocol/LoadFile2.h>
#include <ipxe/efi/Protocol/LoadedImage.h>
#include <ipxe/efi/Protocol/ManagedNetwork.h>
#include <ipxe/efi/Protocol/Mtftp4.h>
#include <ipxe/efi/Protocol/NetworkInterfaceIdentifier.h>
#include <ipxe/efi/Protocol/PciIo.h>
#include <ipxe/efi/Protocol/PciRootBridgeIo.h>
#include <ipxe/efi/Protocol/PxeBaseCode.h>
#include <ipxe/efi/Protocol/SerialIo.h>
#include <ipxe/efi/Protocol/SimpleFileSystem.h>
#include <ipxe/efi/Protocol/SimpleNetwork.h>
#include <ipxe/efi/Protocol/SimplePointer.h>
#include <ipxe/efi/Protocol/SimpleTextIn.h>
#include <ipxe/efi/Protocol/SimpleTextInEx.h>
#include <ipxe/efi/Protocol/SimpleTextOut.h>
#include <ipxe/efi/Protocol/TcgService.h>
#include <ipxe/efi/Protocol/Tcp4.h>
#include <ipxe/efi/Protocol/Udp4.h>
#include <ipxe/efi/Protocol/UgaDraw.h>
#include <ipxe/efi/Protocol/UnicodeCollation.h>
#include <ipxe/efi/Protocol/UsbHostController.h>
#include <ipxe/efi/Protocol/Usb2HostController.h>
#include <ipxe/efi/Protocol/UsbIo.h>
#include <ipxe/efi/Protocol/VlanConfig.h>
#include <ipxe/efi/Guid/FileInfo.h>
#include <ipxe/efi/Guid/FileSystemInfo.h>

/** @file
 *
 * EFI GUIDs
 *
 */

/* TrEE protocol GUID definition in EDK2 headers is broken (missing braces) */
#define EFI_TREE_PROTOCOL_GUID						\
	{ 0x607f766c, 0x7455, 0x42be,					\
	  { 0x93, 0x0b, 0xe4, 0xd7, 0x6d, 0xb2, 0x72, 0x0f } }

/** Absolute pointer protocol GUID */
EFI_GUID efi_absolute_pointer_protocol_guid
	= EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;

/** Apple NetBoot protocol GUID */
EFI_GUID efi_apple_net_boot_protocol_guid
	= EFI_APPLE_NET_BOOT_PROTOCOL_GUID;

/** ARP protocol GUID */
EFI_GUID efi_arp_protocol_guid
	= EFI_ARP_PROTOCOL_GUID;

/** ARP service binding protocol GUID */
EFI_GUID efi_arp_service_binding_protocol_guid
	= EFI_ARP_SERVICE_BINDING_PROTOCOL_GUID;

/** Block I/O protocol GUID */
EFI_GUID efi_block_io_protocol_guid
	= EFI_BLOCK_IO_PROTOCOL_GUID;

/** Bus specific driver override protocol GUID */
EFI_GUID efi_bus_specific_driver_override_protocol_guid
	= EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL_GUID;

/** Component name protocol GUID */
EFI_GUID efi_component_name_protocol_guid
	= EFI_COMPONENT_NAME_PROTOCOL_GUID;

/** Component name 2 protocol GUID */
EFI_GUID efi_component_name2_protocol_guid
	= EFI_COMPONENT_NAME2_PROTOCOL_GUID;

/** Console control protocol GUID */
EFI_GUID efi_console_control_protocol_guid
	= EFI_CONSOLE_CONTROL_PROTOCOL_GUID;

/** Device path protocol GUID */
EFI_GUID efi_device_path_protocol_guid
	= EFI_DEVICE_PATH_PROTOCOL_GUID;

/** DHCPv4 protocol GUID */
EFI_GUID efi_dhcp4_protocol_guid
	= EFI_DHCP4_PROTOCOL_GUID;

/** DHCPv4 service binding protocol GUID */
EFI_GUID efi_dhcp4_service_binding_protocol_guid
	= EFI_DHCP4_SERVICE_BINDING_PROTOCOL_GUID;

/** Disk I/O protocol GUID */
EFI_GUID efi_disk_io_protocol_guid
	= EFI_DISK_IO_PROTOCOL_GUID;

/** Driver binding protocol GUID */
EFI_GUID efi_driver_binding_protocol_guid
	= EFI_DRIVER_BINDING_PROTOCOL_GUID;

/** Graphics output protocol GUID */
EFI_GUID efi_graphics_output_protocol_guid
	= EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;

/** HII configuration access protocol GUID */
EFI_GUID efi_hii_config_access_protocol_guid
	= EFI_HII_CONFIG_ACCESS_PROTOCOL_GUID;

/** HII font protocol GUID */
EFI_GUID efi_hii_font_protocol_guid
	= EFI_HII_FONT_PROTOCOL_GUID;

/** IPv4 protocol GUID */
EFI_GUID efi_ip4_protocol_guid
	= EFI_IP4_PROTOCOL_GUID;

/** IPv4 configuration protocol GUID */
EFI_GUID efi_ip4_config_protocol_guid
	= EFI_IP4_CONFIG_PROTOCOL_GUID;

/** IPv4 service binding protocol GUID */
EFI_GUID efi_ip4_service_binding_protocol_guid
	= EFI_IP4_SERVICE_BINDING_PROTOCOL_GUID;

/** Load file protocol GUID */
EFI_GUID efi_load_file_protocol_guid
	= EFI_LOAD_FILE_PROTOCOL_GUID;

/** Load file 2 protocol GUID */
EFI_GUID efi_load_file2_protocol_guid
	= EFI_LOAD_FILE2_PROTOCOL_GUID;

/** Loaded image protocol GUID */
EFI_GUID efi_loaded_image_protocol_guid
	= EFI_LOADED_IMAGE_PROTOCOL_GUID;

/** Loaded image device path protocol GUID */
EFI_GUID efi_loaded_image_device_path_protocol_guid
	= EFI_LOADED_IMAGE_DEVICE_PATH_PROTOCOL_GUID;

/** Managed network protocol GUID */
EFI_GUID efi_managed_network_protocol_guid
	= EFI_MANAGED_NETWORK_PROTOCOL_GUID;

/** Managed network service binding protocol GUID */
EFI_GUID efi_managed_network_service_binding_protocol_guid
	= EFI_MANAGED_NETWORK_SERVICE_BINDING_PROTOCOL_GUID;

/** MTFTPv4 protocol GUID */
EFI_GUID efi_mtftp4_protocol_guid
	= EFI_MTFTP4_PROTOCOL_GUID;

/** MTFTPv4 service binding protocol GUID */
EFI_GUID efi_mtftp4_service_binding_protocol_guid
	= EFI_MTFTP4_SERVICE_BINDING_PROTOCOL_GUID;

/** Network interface identifier protocol GUID (old version) */
EFI_GUID efi_nii_protocol_guid
	= EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID;

/** Network interface identifier protocol GUID (new version) */
EFI_GUID efi_nii31_protocol_guid
	= EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID_31;

/** PCI I/O protocol GUID */
EFI_GUID efi_pci_io_protocol_guid
	= EFI_PCI_IO_PROTOCOL_GUID;

/** PCI root bridge I/O protocol GUID */
EFI_GUID efi_pci_root_bridge_io_protocol_guid
	= EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_GUID;

/** PXE base code protocol GUID */
EFI_GUID efi_pxe_base_code_protocol_guid
	= EFI_PXE_BASE_CODE_PROTOCOL_GUID;

/** Serial I/O protocol GUID */
EFI_GUID efi_serial_io_protocol_guid
	= EFI_SERIAL_IO_PROTOCOL_GUID;

/** Simple file system protocol GUID */
EFI_GUID efi_simple_file_system_protocol_guid
	= EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;

/** Simple network protocol GUID */
EFI_GUID efi_simple_network_protocol_guid
	= EFI_SIMPLE_NETWORK_PROTOCOL_GUID;

/** Simple pointer protocol GUID */
EFI_GUID efi_simple_pointer_protocol_guid
	= EFI_SIMPLE_POINTER_PROTOCOL_GUID;

/** Simple text input protocol GUID */
EFI_GUID efi_simple_text_input_protocol_guid
	= EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID;

/** Simple text input extension protocol GUID */
EFI_GUID efi_simple_text_input_ex_protocol_guid
	= EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;

/** Simple text output protocol GUID */
EFI_GUID efi_simple_text_output_protocol_guid
	= EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID;

/** TCG protocol GUID */
EFI_GUID efi_tcg_protocol_guid
	= EFI_TCG_PROTOCOL_GUID;

/** TCPv4 protocol GUID */
EFI_GUID efi_tcp4_protocol_guid
	= EFI_TCP4_PROTOCOL_GUID;

/** TCPv4 service binding protocol GUID */
EFI_GUID efi_tcp4_service_binding_protocol_guid
	= EFI_TCP4_SERVICE_BINDING_PROTOCOL_GUID;

/** TrEE protocol GUID */
EFI_GUID efi_tree_protocol_guid
	= EFI_TREE_PROTOCOL_GUID;

/** UDPv4 protocol GUID */
EFI_GUID efi_udp4_protocol_guid
	= EFI_UDP4_PROTOCOL_GUID;

/** UDPv4 service binding protocol GUID */
EFI_GUID efi_udp4_service_binding_protocol_guid
	= EFI_UDP4_SERVICE_BINDING_PROTOCOL_GUID;

/** UGA draw protocol GUID */
EFI_GUID efi_uga_draw_protocol_guid
	= EFI_UGA_DRAW_PROTOCOL_GUID;

/** Unicode collation protocol GUID */
EFI_GUID efi_unicode_collation_protocol_guid
	= EFI_UNICODE_COLLATION_PROTOCOL_GUID;

/** USB host controller protocol GUID */
EFI_GUID efi_usb_hc_protocol_guid
	= EFI_USB_HC_PROTOCOL_GUID;

/** USB2 host controller protocol GUID */
EFI_GUID efi_usb2_hc_protocol_guid
	= EFI_USB2_HC_PROTOCOL_GUID;

/** USB I/O protocol GUID */
EFI_GUID efi_usb_io_protocol_guid
	= EFI_USB_IO_PROTOCOL_GUID;

/** VLAN configuration protocol GUID */
EFI_GUID efi_vlan_config_protocol_guid
	= EFI_VLAN_CONFIG_PROTOCOL_GUID;

/** File information GUID */
EFI_GUID efi_file_info_id = EFI_FILE_INFO_ID;

/** File system information GUID */
EFI_GUID efi_file_system_info_id = EFI_FILE_SYSTEM_INFO_ID;

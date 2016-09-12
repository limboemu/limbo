#ifndef _IPXE_EFI_PXE_H
#define _IPXE_EFI_PXE_H

/** @file
 *
 * EFI PXE base code protocol
 */

#include <ipxe/efi/efi.h>
#include <ipxe/netdevice.h>

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

extern int efi_pxe_install ( EFI_HANDLE handle, struct net_device *netdev );
extern void efi_pxe_uninstall ( EFI_HANDLE handle );

#endif /* _IPXE_EFI_PXE_H */

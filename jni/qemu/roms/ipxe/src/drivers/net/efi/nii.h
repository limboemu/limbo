#ifndef _NII_H
#define _NII_H

/** @file
 *
 * NII driver
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

struct efi_device;

extern int nii_start ( struct efi_device *efidev );
extern void nii_stop ( struct efi_device *efidev );

#endif /* _NII_H */

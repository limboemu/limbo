#ifndef _IPXE_EFIARM_NAP_H
#define _IPXE_EFIARM_NAP_H

/** @file
 *
 * EFI CPU sleeping
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#ifdef NAP_EFIARM
#define NAP_PREFIX_efiarm
#else
#define NAP_PREFIX_efiarm __efiarm_
#endif

#endif /* _IPXE_EFIARM_NAP_H */

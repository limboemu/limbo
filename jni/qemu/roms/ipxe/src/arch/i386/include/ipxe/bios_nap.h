#ifndef _IPXE_BIOS_NAP_H
#define _IPXE_BIOS_NAP_H

/** @file
 *
 * BIOS CPU sleeping
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#ifdef NAP_PCBIOS
#define NAP_PREFIX_pcbios
#else
#define NAP_PREFIX_pcbios __pcbios_
#endif

#endif /* _IPXE_BIOS_NAP_H */

#ifndef _IPXE_ELF_H
#define _IPXE_ELF_H

/**
 * @file
 *
 * ELF image format
 *
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <elf.h>

extern int elf_load ( struct image *image, physaddr_t *entry, physaddr_t *max );

#endif /* _IPXE_ELF_H */

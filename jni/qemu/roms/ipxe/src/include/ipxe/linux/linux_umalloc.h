#ifndef _IPXE_LINUX_UMALLOC_H
#define _IPXE_LINUX_UMALLOC_H

FILE_LICENCE(GPL2_OR_LATER);

/** @file
 *
 * iPXE user memory allocation API for linux
 *
 */

#ifdef UMALLOC_LINUX
#define UMALLOC_PREFIX_linux
#else
#define UMALLOC_PREFIX_linux __linux_
#endif

#endif /* _IPXE_LINUX_UMALLOC_H */

#ifndef _IPXE_LINUX_ENTROPY_H
#define _IPXE_LINUX_ENTROPY_H

/** @file
 *
 * iPXE entropy API for linux
 *
 */

FILE_LICENCE(GPL2_OR_LATER);

#ifdef ENTROPY_LINUX
#define ENTROPY_PREFIX_linux
#else
#define ENTROPY_PREFIX_linux __linux_
#endif

/**
 * min-entropy per sample
 *
 * @ret min_entropy	min-entropy of each sample
 */
static inline __always_inline double
ENTROPY_INLINE ( linux, min_entropy_per_sample ) ( void ) {

	/* We read single bytes from /dev/random and assume that each
	 * contains full entropy.
	 */
	return 8;
}

#endif /* _IPXE_LINUX_ENTROPY_H */

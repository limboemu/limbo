#ifndef ETHERBOOT_BYTESWAP_H
#define ETHERBOOT_BYTESWAP_H

FILE_LICENCE ( GPL2_OR_LATER );

#include "endian.h"
#include "bits/byteswap.h"

#define __bswap_constant_16(x) \
	((uint16_t)((((uint16_t)(x) & 0x00ff) << 8) | \
		    (((uint16_t)(x) & 0xff00) >> 8)))

#define __bswap_constant_32(x) \
	((uint32_t)((((uint32_t)(x) & 0x000000ffU) << 24) | \
		    (((uint32_t)(x) & 0x0000ff00U) <<  8) | \
		    (((uint32_t)(x) & 0x00ff0000U) >>  8) | \
		    (((uint32_t)(x) & 0xff000000U) >> 24)))

#define __bswap_constant_64(x) \
	((uint64_t)((((uint64_t)(x) & 0x00000000000000ffULL) << 56) | \
		    (((uint64_t)(x) & 0x000000000000ff00ULL) << 40) | \
		    (((uint64_t)(x) & 0x0000000000ff0000ULL) << 24) | \
		    (((uint64_t)(x) & 0x00000000ff000000ULL) <<  8) | \
		    (((uint64_t)(x) & 0x000000ff00000000ULL) >>  8) | \
		    (((uint64_t)(x) & 0x0000ff0000000000ULL) >> 24) | \
		    (((uint64_t)(x) & 0x00ff000000000000ULL) >> 40) | \
		    (((uint64_t)(x) & 0xff00000000000000ULL) >> 56)))

#define __bswap_16(x) \
	((uint16_t)(__builtin_constant_p(x) ? \
	__bswap_constant_16(x) : \
	__bswap_variable_16(x)))

#define __bswap_32(x) \
	((uint32_t)(__builtin_constant_p(x) ? \
	__bswap_constant_32(x) : \
	__bswap_variable_32(x)))

#define __bswap_64(x) \
	((uint64_t)(__builtin_constant_p(x) ? \
	__bswap_constant_64(x) : \
	__bswap_variable_64(x)))

#if __BYTE_ORDER == __LITTLE_ENDIAN
#include "little_bswap.h"
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
#include "big_bswap.h"
#endif

/* Make routines available to all */
#define swap64(x)	__bswap_64(x)
#define swap32(x)	__bswap_32(x)
#define swap16(x)	__bswap_16(x)
#define bswap_64(x)	__bswap_64(x)
#define bswap_32(x)	__bswap_32(x)
#define bswap_16(x)	__bswap_16(x)
	
#endif /* ETHERBOOT_BYTESWAP_H */

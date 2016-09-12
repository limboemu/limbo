#ifndef _BITS_ENDIAN_H
#define _BITS_ENDIAN_H

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/* ARM may be either little-endian or big-endian */
#ifdef __ARM_BIG_ENDIAN
#define __BYTE_ORDER __BIG_ENDIAN
#else
#define __BYTE_ORDER __LITTLE_ENDIAN
#endif

#endif /* _BITS_ENDIAN_H */

#ifndef _IPXE_BITOPS_H
#define _IPXE_BITOPS_H

/** @file
 *
 * Bit operations
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <bits/bitops.h>

void set_bit ( unsigned int bit, volatile void *bits );
void clear_bit ( unsigned int bit, volatile void *bits );
int test_and_set_bit ( unsigned int bit, volatile void *bits );
int test_and_clear_bit ( unsigned int bit, volatile void *bits );

#endif /* _IPXE_BITOPS_H */

#ifndef _BITS_TCPIP_H
#define _BITS_TCPIP_H

/** @file
 *
 * Transport-network layer interface
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

static inline __attribute__ (( always_inline )) uint16_t
tcpip_continue_chksum ( uint16_t partial, const void *data, size_t len ) {

	/* Not yet optimised */
	return generic_tcpip_continue_chksum ( partial, data, len );
}

#endif /* _BITS_TCPIP_H */

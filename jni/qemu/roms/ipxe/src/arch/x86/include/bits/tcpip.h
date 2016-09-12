#ifndef _BITS_TCPIP_H
#define _BITS_TCPIP_H

/** @file
 *
 * Transport-network layer interface
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

extern uint16_t tcpip_continue_chksum ( uint16_t partial, const void *data,
					size_t len );

#endif /* _BITS_TCPIP_H */

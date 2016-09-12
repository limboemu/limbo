#ifndef _IPXE_TCPIP_H
#define _IPXE_TCPIP_H

/** @file
 *
 * Transport-network layer interface
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <ipxe/socket.h>
#include <ipxe/in.h>
#include <ipxe/tables.h>

extern uint16_t generic_tcpip_continue_chksum ( uint16_t partial,
						const void *data, size_t len );

#include <bits/tcpip.h>

struct io_buffer;
struct net_device;
struct ip_statistics;

/** Positive zero checksum value */
#define TCPIP_POSITIVE_ZERO_CSUM 0x0000

/** Negative zero checksum value */
#define TCPIP_NEGATIVE_ZERO_CSUM 0xffff

/** Empty checksum value
 *
 * All of our TCP/IP checksum algorithms will return only the positive
 * representation of zero (0x0000) for a zero checksum over non-zero
 * input data.  This property arises since the end-around carry used
 * to mimic one's complement addition using unsigned arithmetic
 * prevents the running total from ever returning to 0x0000.  The
 * running total will therefore use only the negative representation
 * of zero (0xffff).  Since the return value is the one's complement
 * negation of the running total (calculated by simply bit-inverting
 * the running total), the return value will therefore use only the
 * positive representation of zero (0x0000).
 *
 * It is a very common misconception (found in many places such as
 * RFC1624) that this is a property guaranteed by the underlying
 * mathematics.  It is not; the choice of which zero representation is
 * used is merely an artifact of the software implementation of the
 * checksum algorithm.
 *
 * For consistency, we choose to use the positive representation of
 * zero (0x0000) for the checksum of a zero-length block of data.
 * This ensures that all of our TCP/IP checksum algorithms will return
 * only the positive representation of zero (0x0000) for a zero
 * checksum (regardless of the input data).
 */
#define TCPIP_EMPTY_CSUM TCPIP_POSITIVE_ZERO_CSUM

/** TCP/IP address flags */
enum tcpip_st_flags {
	/** Bind to a privileged port (less than 1024)
	 *
	 * This value is chosen as 1024 to optimise the calculations
	 * in tcpip_bind().
	 */
	TCPIP_BIND_PRIVILEGED = 0x0400,
};

/**
 * TCP/IP socket address
 *
 * This contains the fields common to socket addresses for all TCP/IP
 * address families.
 */
struct sockaddr_tcpip {
	/** Socket address family (part of struct @c sockaddr) */
	sa_family_t st_family;
	/** Flags */
	uint16_t st_flags;
	/** TCP/IP port */
	uint16_t st_port;
	/** Scope ID
	 *
	 * For link-local or multicast addresses, this is the network
	 * device index.
	 */
        uint16_t st_scope_id;
	/** Padding
	 *
	 * This ensures that a struct @c sockaddr_tcpip is large
	 * enough to hold a socket address for any TCP/IP address
	 * family.
	 */
	char pad[ sizeof ( struct sockaddr ) -
		  ( sizeof ( sa_family_t ) /* st_family */ +
		    sizeof ( uint16_t ) /* st_flags */ +
		    sizeof ( uint16_t ) /* st_port */ +
		    sizeof ( uint16_t ) /* st_scope_id */ ) ];
} __attribute__ (( packed, may_alias ));

/** 
 * A transport-layer protocol of the TCP/IP stack (eg. UDP, TCP, etc)
 */
struct tcpip_protocol {
	/** Protocol name */
	const char *name;
       	/**
         * Process received packet
         *
         * @v iobuf		I/O buffer
	 * @v netdev		Network device
	 * @v st_src		Partially-filled source address
	 * @v st_dest		Partially-filled destination address
	 * @v pshdr_csum	Pseudo-header checksum
	 * @ret rc		Return status code
         *
         * This method takes ownership of the I/O buffer.
         */
        int ( * rx ) ( struct io_buffer *iobuf, struct net_device *netdev,
		       struct sockaddr_tcpip *st_src,
		       struct sockaddr_tcpip *st_dest, uint16_t pshdr_csum );
	/** Preferred zero checksum value
	 *
	 * The checksum is a one's complement value: zero may be
	 * represented by either positive zero (0x0000) or negative
	 * zero (0xffff).
	 */
	uint16_t zero_csum;
        /** 
	 * Transport-layer protocol number
	 *
	 * This is a constant of the type IP_XXX
         */
        uint8_t tcpip_proto;
};

/**
 * A network-layer protocol of the TCP/IP stack (eg. IPV4, IPv6, etc)
 */
struct tcpip_net_protocol {
	/** Protocol name */
	const char *name;
	/** Network address family */
	sa_family_t sa_family;
	/** Fixed header length */
	size_t header_len;
	/** Network-layer protocol */
	struct net_protocol *net_protocol;
	/**
	 * Transmit packet
	 *
	 * @v iobuf		I/O buffer
	 * @v tcpip_protocol	Transport-layer protocol
	 * @v st_src		Source address, or NULL to use default
	 * @v st_dest		Destination address
	 * @v netdev		Network device (or NULL to route automatically)
	 * @v trans_csum	Transport-layer checksum to complete, or NULL
	 * @ret rc		Return status code
	 *
	 * This function takes ownership of the I/O buffer.
	 */
	int ( * tx ) ( struct io_buffer *iobuf,
		       struct tcpip_protocol *tcpip_protocol,
		       struct sockaddr_tcpip *st_src,
		       struct sockaddr_tcpip *st_dest,
		       struct net_device *netdev,
		       uint16_t *trans_csum );
	/**
	 * Determine transmitting network device
	 *
	 * @v st_dest		Destination address
	 * @ret netdev		Network device, or NULL
	 */
	struct net_device * ( * netdev ) ( struct sockaddr_tcpip *dest );
};

/** TCP/IP transport-layer protocol table */
#define TCPIP_PROTOCOLS __table ( struct tcpip_protocol, "tcpip_protocols" )

/** Declare a TCP/IP transport-layer protocol */
#define __tcpip_protocol __table_entry ( TCPIP_PROTOCOLS, 01 )

/** TCP/IP network-layer protocol table */
#define TCPIP_NET_PROTOCOLS \
	__table ( struct tcpip_net_protocol, "tcpip_net_protocols" )

/** Declare a TCP/IP network-layer protocol */
#define __tcpip_net_protocol __table_entry ( TCPIP_NET_PROTOCOLS, 01 )

extern int tcpip_rx ( struct io_buffer *iobuf, struct net_device *netdev,
		      uint8_t tcpip_proto, struct sockaddr_tcpip *st_src,
		      struct sockaddr_tcpip *st_dest, uint16_t pshdr_csum,
		      struct ip_statistics *stats );
extern int tcpip_tx ( struct io_buffer *iobuf, struct tcpip_protocol *tcpip,
		      struct sockaddr_tcpip *st_src,
		      struct sockaddr_tcpip *st_dest,
		      struct net_device *netdev,
		      uint16_t *trans_csum );
extern struct tcpip_net_protocol * tcpip_net_protocol ( sa_family_t sa_family );
extern struct net_device * tcpip_netdev ( struct sockaddr_tcpip *st_dest );
extern size_t tcpip_mtu ( struct sockaddr_tcpip *st_dest );
extern uint16_t tcpip_chksum ( const void *data, size_t len );
extern int tcpip_bind ( struct sockaddr_tcpip *st_local,
			int ( * available ) ( int port ) );

#endif /* _IPXE_TCPIP_H */

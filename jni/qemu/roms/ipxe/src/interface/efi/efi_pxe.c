/*
 * Copyright (C) 2015 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <string.h>
#include <errno.h>
#include <ipxe/refcnt.h>
#include <ipxe/list.h>
#include <ipxe/netdevice.h>
#include <ipxe/fakedhcp.h>
#include <ipxe/process.h>
#include <ipxe/uri.h>
#include <ipxe/in.h>
#include <ipxe/socket.h>
#include <ipxe/tcpip.h>
#include <ipxe/xferbuf.h>
#include <ipxe/open.h>
#include <ipxe/dhcppkt.h>
#include <ipxe/udp.h>
#include <ipxe/efi/efi.h>
#include <ipxe/efi/efi_snp.h>
#include <ipxe/efi/efi_pxe.h>
#include <ipxe/efi/Protocol/PxeBaseCode.h>
#include <ipxe/efi/Protocol/AppleNetBoot.h>
#include <usr/ifmgmt.h>
#include <config/general.h>

/** @file
 *
 * EFI PXE base code protocol
 *
 */

/* Downgrade user experience if configured to do so
 *
 * See comments in efi_snp.c
 */
#ifdef EFI_DOWNGRADE_UX
static EFI_GUID dummy_pxe_base_code_protocol_guid = {
	0x70647523, 0x2320, 0x7477,
	{ 0x66, 0x20, 0x23, 0x6d, 0x6f, 0x72, 0x6f, 0x6e }
};
#define efi_pxe_base_code_protocol_guid dummy_pxe_base_code_protocol_guid
#endif

/** A PXE base code */
struct efi_pxe {
	/** Reference count */
	struct refcnt refcnt;
	/** Underlying network device */
	struct net_device *netdev;
	/** Name */
	const char *name;
	/** List of PXE base codes */
	struct list_head list;

	/** Installed handle */
	EFI_HANDLE handle;
	/** PXE base code protocol */
	EFI_PXE_BASE_CODE_PROTOCOL base;
	/** PXE base code mode */
	EFI_PXE_BASE_CODE_MODE mode;
	/** Apple NetBoot protocol */
	EFI_APPLE_NET_BOOT_PROTOCOL apple;

	/** TCP/IP network-layer protocol */
	struct tcpip_net_protocol *tcpip;
	/** Network-layer protocol */
	struct net_protocol *net;

	/** Data transfer buffer */
	struct xfer_buffer buf;

	/** (M)TFTP download interface */
	struct interface tftp;
	/** Block size (for TFTP) */
	size_t blksize;
	/** Overall return status */
	int rc;

	/** UDP interface */
	struct interface udp;
	/** List of received UDP packets */
	struct list_head queue;
	/** UDP interface closer process */
	struct process process;
};

/**
 * Free PXE base code
 *
 * @v refcnt		Reference count
 */
static void efi_pxe_free ( struct refcnt *refcnt ) {
	struct efi_pxe *pxe = container_of ( refcnt, struct efi_pxe, refcnt );

	netdev_put ( pxe->netdev );
	free ( pxe );
}

/** List of PXE base codes */
static LIST_HEAD ( efi_pxes );

/**
 * Locate PXE base code
 *
 * @v handle		EFI handle
 * @ret pxe		PXE base code, or NULL
 */
static struct efi_pxe * efi_pxe_find ( EFI_HANDLE handle ) {
	struct efi_pxe *pxe;

	/* Locate base code */
	list_for_each_entry ( pxe, &efi_pxes, list ) {
		if ( pxe->handle == handle )
			return pxe;
	}

	return NULL;
}

/******************************************************************************
 *
 * IP addresses
 *
 ******************************************************************************
 */

/**
 * An EFI socket address
 *
 */
struct sockaddr_efi {
	/** Socket address family (part of struct @c sockaddr) */
	sa_family_t se_family;
	/** Flags (part of struct @c sockaddr_tcpip) */
	uint16_t se_flags;
	/** TCP/IP port (part of struct @c sockaddr_tcpip) */
	uint16_t se_port;
	/** Scope ID (part of struct @c sockaddr_tcpip)
	 *
	 * For link-local or multicast addresses, this is the network
	 * device index.
	 */
        uint16_t se_scope_id;
	/** IP address */
	EFI_IP_ADDRESS se_addr;
	/** Padding
	 *
	 * This ensures that a struct @c sockaddr_tcpip is large
	 * enough to hold a socket address for any TCP/IP address
	 * family.
	 */
	char pad[ sizeof ( struct sockaddr ) -
		  ( sizeof ( sa_family_t ) /* se_family */ +
		    sizeof ( uint16_t ) /* se_flags */ +
		    sizeof ( uint16_t ) /* se_port */ +
		    sizeof ( uint16_t ) /* se_scope_id */ +
		    sizeof ( EFI_IP_ADDRESS ) /* se_addr */ ) ];
} __attribute__ (( packed, may_alias ));

/**
 * Populate socket address from EFI IP address
 *
 * @v pxe		PXE base code
 * @v ip		EFI IP address
 * @v sa		Socket address to fill in
 */
static void efi_pxe_ip_sockaddr ( struct efi_pxe *pxe, EFI_IP_ADDRESS *ip,
				  struct sockaddr *sa ) {
	union {
		struct sockaddr sa;
		struct sockaddr_efi se;
	} *sockaddr = container_of ( sa, typeof ( *sockaddr ), sa );

	/* Initialise socket address */
	memset ( sockaddr, 0, sizeof ( *sockaddr ) );
	sockaddr->sa.sa_family = pxe->tcpip->sa_family;
	memcpy ( &sockaddr->se.se_addr, ip, pxe->net->net_addr_len );
	sockaddr->se.se_scope_id = pxe->netdev->index;
}

/**
 * Transcribe EFI IP address (for debugging)
 *
 * @v pxe		PXE base code
 * @v ip		EFI IP address
 * @ret text		Transcribed IP address
 */
static const char * efi_pxe_ip_ntoa ( struct efi_pxe *pxe,
				      EFI_IP_ADDRESS *ip ) {

	return pxe->net->ntoa ( ip );
}

/**
 * Populate local IP address
 *
 * @v pxe		PXE base code
 * @ret rc		Return status code
 */
static int efi_pxe_ip ( struct efi_pxe *pxe ) {
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;
	struct in_addr address;
	struct in_addr netmask;

	/* It's unclear which of the potentially many IPv6 addresses
	 * is supposed to be used.
	 */
	if ( mode->UsingIpv6 )
		return -ENOTSUP;

	/* Fetch IP address and subnet mask */
	fetch_ipv4_setting ( netdev_settings ( pxe->netdev ), &ip_setting,
			     &address );
	fetch_ipv4_setting ( netdev_settings ( pxe->netdev ), &netmask_setting,
			     &netmask );

	/* Populate IP address and subnet mask */
	memset ( &mode->StationIp, 0, sizeof ( mode->StationIp ) );
	memcpy ( &mode->StationIp, &address, sizeof ( address ) );
	memset ( &mode->SubnetMask, 0, sizeof ( mode->SubnetMask ) );
	memcpy ( &mode->SubnetMask, &netmask, sizeof ( netmask ) );

	return 0;
}

/**
 * Check if IP address matches filter
 *
 * @v pxe		PXE base code
 * @v ip		EFI IP address
 * @ret is_match	IP address matches filter
 */
static int efi_pxe_ip_filter ( struct efi_pxe *pxe, EFI_IP_ADDRESS *ip ) {
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;
	EFI_PXE_BASE_CODE_IP_FILTER *filter = &mode->IpFilter;
	uint8_t filters = filter->Filters;
	union {
		EFI_IP_ADDRESS ip;
		struct in_addr in;
		struct in6_addr in6;
	} *u = container_of ( ip, typeof ( *u ), ip );
	size_t addr_len = pxe->net->net_addr_len;
	unsigned int i;

	/* Match everything, if applicable */
	if ( filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS )
		return 1;

	/* Match all multicasts, if applicable */
	if ( filters & EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS_MULTICAST ) {
		if ( mode->UsingIpv6 ) {
			if ( IN6_IS_ADDR_MULTICAST ( &u->in6 ) )
				return 1;
		} else {
			if ( IN_IS_MULTICAST ( u->in.s_addr ) )
				return 1;
		}
	}

	/* Match IPv4 broadcasts, if applicable */
	if ( filters & EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST ) {
		if ( ( ! mode->UsingIpv6 ) &&
		     ( u->in.s_addr == INADDR_BROADCAST ) )
			return 1;
	}

	/* Match station address, if applicable */
	if ( filters & EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP ) {
		if ( memcmp ( ip, &mode->StationIp, addr_len ) == 0 )
			return 1;
	}

	/* Match explicit addresses, if applicable */
	for ( i = 0 ; i < filter->IpCnt ; i++ ) {
		if ( memcmp ( ip, &filter->IpList[i], addr_len ) == 0 )
			return 1;
	}

	return 0;
}

/******************************************************************************
 *
 * Data transfer buffer
 *
 ******************************************************************************
 */

/**
 * Reallocate PXE data transfer buffer
 *
 * @v xferbuf		Data transfer buffer
 * @v len		New length (or zero to free buffer)
 * @ret rc		Return status code
 */
static int efi_pxe_buf_realloc ( struct xfer_buffer *xferbuf __unused,
				 size_t len __unused ) {

	/* Can never reallocate: return EFI_BUFFER_TOO_SMALL */
	return -ERANGE;
}

/**
 * Write data to PXE data transfer buffer
 *
 * @v xferbuf		Data transfer buffer
 * @v offset		Starting offset
 * @v data		Data to copy
 * @v len		Length of data
 */
static void efi_pxe_buf_write ( struct xfer_buffer *xferbuf, size_t offset,
				const void *data, size_t len ) {

	/* Copy data to buffer */
	memcpy ( ( xferbuf->data + offset ), data, len );
}

/** PXE data transfer buffer operations */
static struct xfer_buffer_operations efi_pxe_buf_operations = {
	.realloc = efi_pxe_buf_realloc,
	.write = efi_pxe_buf_write,
};

/******************************************************************************
 *
 * (M)TFTP download interface
 *
 ******************************************************************************
 */

/**
 * Close PXE (M)TFTP download interface
 *
 * @v pxe		PXE base code
 * @v rc		Reason for close
 */
static void efi_pxe_tftp_close ( struct efi_pxe *pxe, int rc ) {

	/* Restart interface */
	intf_restart ( &pxe->tftp, rc );

	/* Record overall status */
	pxe->rc = rc;
}

/**
 * Check PXE (M)TFTP download flow control window
 *
 * @v pxe		PXE base code
 * @ret len		Length of window
 */
static size_t efi_pxe_tftp_window ( struct efi_pxe *pxe ) {

	/* Return requested blocksize */
	return pxe->blksize;
}

/**
 * Receive new PXE (M)TFTP download data
 *
 * @v pxe		PXE base code
 * @v iobuf		I/O buffer
 * @v meta		Transfer metadata
 * @ret rc		Return status code
 */
static int efi_pxe_tftp_deliver ( struct efi_pxe *pxe,
				  struct io_buffer *iobuf,
				  struct xfer_metadata *meta ) {
	int rc;

	/* Deliver to data transfer buffer */
	if ( ( rc = xferbuf_deliver ( &pxe->buf, iob_disown ( iobuf ),
				      meta ) ) != 0 )
		goto err_deliver;

	return 0;

 err_deliver:
	efi_pxe_tftp_close ( pxe, rc );
	return rc;
}

/** PXE file data transfer interface operations */
static struct interface_operation efi_pxe_tftp_operations[] = {
	INTF_OP ( xfer_deliver, struct efi_pxe *, efi_pxe_tftp_deliver ),
	INTF_OP ( xfer_window, struct efi_pxe *, efi_pxe_tftp_window ),
	INTF_OP ( intf_close, struct efi_pxe *, efi_pxe_tftp_close ),
};

/** PXE file data transfer interface descriptor */
static struct interface_descriptor efi_pxe_tftp_desc =
	INTF_DESC ( struct efi_pxe, tftp, efi_pxe_tftp_operations );

/**
 * Open (M)TFTP download interface
 *
 * @v pxe		PXE base code
 * @v ip		EFI IP address
 * @v filename		Filename
 * @ret rc		Return status code
 */
static int efi_pxe_tftp_open ( struct efi_pxe *pxe, EFI_IP_ADDRESS *ip,
			       const char *filename ) {
	struct sockaddr server;
	struct uri *uri;
	int rc;

	/* Parse server address and filename */
	efi_pxe_ip_sockaddr ( pxe, ip, &server );
	uri = pxe_uri ( &server, filename );
	if ( ! uri ) {
		DBGC ( pxe, "PXE %s could not parse %s:%s\n", pxe->name,
		       efi_pxe_ip_ntoa ( pxe, ip ), filename );
		rc = -ENOTSUP;
		goto err_parse;
	}

	/* Open URI */
	if ( ( rc = xfer_open_uri ( &pxe->tftp, uri ) ) != 0 ) {
		DBGC ( pxe, "PXE %s could not open: %s\n",
		       pxe->name, strerror ( rc ) );
		goto err_open;
	}

 err_open:
	uri_put ( uri );
 err_parse:
	return rc;
}

/******************************************************************************
 *
 * UDP interface
 *
 ******************************************************************************
 */

/** EFI UDP pseudo-header */
struct efi_pxe_udp_pseudo_header {
	/** Network-layer protocol */
	struct net_protocol *net;
	/** Destination port */
	uint16_t dest_port;
	/** Source port */
	uint16_t src_port;
} __attribute__ (( packed ));

/**
 * Close UDP interface
 *
 * @v pxe		PXE base code
 * @v rc		Reason for close
 */
static void efi_pxe_udp_close ( struct efi_pxe *pxe, int rc ) {
	struct io_buffer *iobuf;
	struct io_buffer *tmp;

	/* Release our claim on SNP devices, if applicable */
	if ( process_running ( &pxe->process ) )
		efi_snp_release();

	/* Stop process */
	process_del ( &pxe->process );

	/* Restart UDP interface */
	intf_restart ( &pxe->udp, rc );

	/* Flush any received UDP packets */
	list_for_each_entry_safe ( iobuf, tmp, &pxe->queue, list ) {
		list_del ( &iobuf->list );
		free_iob ( iobuf );
	}
}

/**
 * Receive UDP packet
 *
 * @v pxe		PXE base code
 * @v iobuf		I/O buffer
 * @v meta		Data transfer metadata
 * @ret rc		Return status code
 */
static int efi_pxe_udp_deliver ( struct efi_pxe *pxe, struct io_buffer *iobuf,
				 struct xfer_metadata *meta ) {
	struct sockaddr_efi *se_src;
	struct sockaddr_efi *se_dest;
	struct tcpip_net_protocol *tcpip;
	struct net_protocol *net;
	struct efi_pxe_udp_pseudo_header *pshdr;
	size_t addr_len;
	size_t pshdr_len;
	int rc;

	/* Sanity checks */
	assert ( meta != NULL );
	se_src = ( ( struct sockaddr_efi * ) meta->src );
	assert ( se_src != NULL );
	se_dest = ( ( struct sockaddr_efi * ) meta->dest );
	assert ( se_dest != NULL );
	assert ( se_src->se_family == se_dest->se_family );

	/* Determine protocol */
	tcpip = tcpip_net_protocol ( se_src->se_family );
	if ( ! tcpip ) {
		rc = -ENOTSUP;
		goto err_unsupported;
	}
	net = tcpip->net_protocol;
	addr_len = net->net_addr_len;

	/* Construct pseudo-header */
	pshdr_len = ( sizeof ( *pshdr ) + ( 2 * addr_len ) );
	if ( ( rc = iob_ensure_headroom ( iobuf, pshdr_len ) ) != 0 )
		goto err_headroom;
	memcpy ( iob_push ( iobuf, addr_len ), &se_src->se_addr, addr_len );
	memcpy ( iob_push ( iobuf, addr_len ), &se_dest->se_addr, addr_len );
	pshdr = iob_push ( iobuf, sizeof ( *pshdr ) );
	pshdr->net = net;
	pshdr->dest_port = ntohs ( se_dest->se_port );
	pshdr->src_port = ntohs ( se_src->se_port );

	/* Add to queue */
	list_add_tail ( &iobuf->list, &pxe->queue );

	return 0;

 err_unsupported:
 err_headroom:
	free_iob ( iobuf );
	return rc;
}

/** PXE UDP interface operations */
static struct interface_operation efi_pxe_udp_operations[] = {
	INTF_OP ( xfer_deliver, struct efi_pxe *, efi_pxe_udp_deliver ),
	INTF_OP ( intf_close, struct efi_pxe *, efi_pxe_udp_close ),
};

/** PXE UDP interface descriptor */
static struct interface_descriptor efi_pxe_udp_desc =
	INTF_DESC ( struct efi_pxe, udp, efi_pxe_udp_operations );

/**
 * Open UDP interface
 *
 * @v pxe		PXE base code
 * @ret rc		Return status code
 */
static int efi_pxe_udp_open ( struct efi_pxe *pxe ) {
	int rc;

	/* If interface is already open, then cancel the scheduled close */
	if ( process_running ( &pxe->process ) ) {
		process_del ( &pxe->process );
		return 0;
	}

	/* Open promiscuous UDP interface */
	if ( ( rc = udp_open_promisc ( &pxe->udp ) ) != 0 ) {
		DBGC ( pxe, "PXE %s could not open UDP connection: %s\n",
		       pxe->name, strerror ( rc ) );
		return rc;
	}

	/* Claim network devices */
	efi_snp_claim();

	return 0;
}

/**
 * Schedule close of UDP interface
 *
 * @v pxe		PXE base code
 */
static void efi_pxe_udp_schedule_close ( struct efi_pxe *pxe ) {

	/* The EFI PXE base code protocol does not provide any
	 * explicit UDP open/close methods.  To avoid the overhead of
	 * reopening a socket for each read/write operation, we start
	 * a process which will close the socket immediately if the
	 * next call into iPXE is anything other than a UDP
	 * read/write.
	 */
	process_add ( &pxe->process );
}

/**
 * Scheduled close of UDP interface
 *
 * @v pxe		PXE base code
 */
static void efi_pxe_udp_scheduled_close ( struct efi_pxe *pxe ) {

	/* Close UDP interface */
	efi_pxe_udp_close ( pxe, 0 );
}

/** UDP close process descriptor */
static struct process_descriptor efi_pxe_process_desc =
	PROC_DESC_ONCE ( struct efi_pxe, process, efi_pxe_udp_scheduled_close );

/******************************************************************************
 *
 * Fake DHCP packets
 *
 ******************************************************************************
 */

/**
 * Name fake DHCP packet
 *
 * @v pxe		PXE base code
 * @v packet		Packet
 * @ret name		Name of packet
 */
static const char * efi_pxe_fake_name ( struct efi_pxe *pxe,
					EFI_PXE_BASE_CODE_PACKET *packet ) {
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;

	if ( packet == &mode->DhcpDiscover ) {
		return "DhcpDiscover";
	} else if ( packet == &mode->DhcpAck ) {
		return "DhcpAck";
	} else if ( packet == &mode->ProxyOffer ) {
		return "ProxyOffer";
	} else if ( packet == &mode->PxeDiscover ) {
		return "PxeDiscover";
	} else if ( packet == &mode->PxeReply ) {
		return "PxeReply";
	} else if ( packet == &mode->PxeBisReply ) {
		return "PxeBisReply";
	} else {
		return "<UNKNOWN>";
	}
}

/**
 * Construct fake DHCP packet and flag
 *
 * @v pxe		PXE base code
 * @v fake		Fake packet constructor
 * @v packet		Packet to fill in
 * @ret exists		Packet existence flag
 */
static BOOLEAN efi_pxe_fake ( struct efi_pxe *pxe,
			      int ( * fake ) ( struct net_device *netdev,
					       void *data, size_t len ),
			      EFI_PXE_BASE_CODE_PACKET *packet ) {
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;
	struct dhcp_packet dhcppkt;
	struct dhcphdr *dhcphdr;
	unsigned int len;
	int rc;

	/* The fake packet constructors do not support IPv6 */
	if ( mode->UsingIpv6 )
		return FALSE;

	/* Attempt to construct packet */
	if ( ( rc = fake ( pxe->netdev, packet, sizeof ( *packet ) ) != 0 ) ) {
		DBGC ( pxe, "PXE %s could not fake %s: %s\n", pxe->name,
		       efi_pxe_fake_name ( pxe, packet ), strerror ( rc ) );
		return FALSE;
	}

	/* The WDS bootstrap wdsmgfw.efi has a buggy DHCPv4 packet
	 * parser which does not correctly handle DHCP padding bytes.
	 * Specifically, if a padding byte (i.e. a zero) is
	 * encountered, the parse will first increment the pointer by
	 * one to skip over the padding byte but will then drop into
	 * the code path for handling normal options, which increments
	 * the pointer by two to skip over the (already-skipped) type
	 * field and the (non-existent) length field.
	 *
	 * The upshot of this bug in WDS is that the parser will fail
	 * with an error 0xc0000023 if the number of spare bytes after
	 * the end of the options is not an exact multiple of three.
	 *
	 * Work around this buggy parser by adding an explicit
	 * DHCP_END tag.
	 */
	dhcphdr = container_of ( &packet->Dhcpv4.BootpOpcode,
				 struct dhcphdr, op );
	dhcppkt_init ( &dhcppkt, dhcphdr, sizeof ( *packet ) );
	len = dhcppkt_len ( &dhcppkt );
	if ( len < sizeof ( *packet ) )
		packet->Raw[len] = DHCP_END;

	return TRUE;
}

/**
 * Construct fake DHCP packets
 *
 * @v pxe		PXE base code
 */
static void efi_pxe_fake_all ( struct efi_pxe *pxe ) {
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;

	/* Construct fake packets */
	mode->DhcpDiscoverValid =
		efi_pxe_fake ( pxe, create_fakedhcpdiscover,
			       &mode->DhcpDiscover );
	mode->DhcpAckReceived =
		efi_pxe_fake ( pxe, create_fakedhcpack,
			       &mode->DhcpAck );
	mode->PxeReplyReceived =
		efi_pxe_fake ( pxe, create_fakepxebsack,
			       &mode->PxeReply );
}

/******************************************************************************
 *
 * Base code protocol
 *
 ******************************************************************************
 */

/**
 * Start PXE base code
 *
 * @v base		PXE base code protocol
 * @v use_ipv6		Use IPv6
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI efi_pxe_start ( EFI_PXE_BASE_CODE_PROTOCOL *base,
					 BOOLEAN use_ipv6 ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;
	struct tcpip_net_protocol *ipv6 = tcpip_net_protocol ( AF_INET6 );
	sa_family_t family = ( use_ipv6 ? AF_INET6 : AF_INET );
	int rc;

	DBGC ( pxe, "PXE %s START %s\n", pxe->name, ( ipv6 ? "IPv6" : "IPv4" ));

	/* Initialise mode structure */
	memset ( mode, 0, sizeof ( *mode ) );
	mode->AutoArp = TRUE;
	mode->TTL = DEFAULT_TTL;
	mode->ToS = DEFAULT_ToS;
	mode->IpFilter.Filters =
		( EFI_PXE_BASE_CODE_IP_FILTER_STATION_IP |
		  EFI_PXE_BASE_CODE_IP_FILTER_BROADCAST |
		  EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS |
		  EFI_PXE_BASE_CODE_IP_FILTER_PROMISCUOUS_MULTICAST );

	/* Check for IPv4/IPv6 support */
	mode->Ipv6Supported = ( ipv6 != NULL );
	mode->Ipv6Available = ( ipv6 != NULL );
	pxe->tcpip = tcpip_net_protocol ( family );
	if ( ! pxe->tcpip ) {
		DBGC ( pxe, "PXE %s has no support for %s\n",
		       pxe->name, socket_family_name ( family ) );
		return EFI_UNSUPPORTED;
	}
	pxe->net = pxe->tcpip->net_protocol;
	mode->UsingIpv6 = use_ipv6;

	/* Populate station IP address */
	if ( ( rc = efi_pxe_ip ( pxe ) ) != 0 )
		return rc;

	/* Construct fake DHCP packets */
	efi_pxe_fake_all ( pxe );

	/* Record that base code is started */
	mode->Started = TRUE;
	DBGC ( pxe, "PXE %s using %s\n",
	       pxe->name, pxe->net->ntoa ( &mode->StationIp ) );

	return 0;
}

/**
 * Stop PXE base code
 *
 * @v base		PXE base code protocol
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI efi_pxe_stop ( EFI_PXE_BASE_CODE_PROTOCOL *base ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;

	DBGC ( pxe, "PXE %s STOP\n", pxe->name );

	/* Record that base code is stopped */
	mode->Started = FALSE;

	/* Close TFTP */
	efi_pxe_tftp_close ( pxe, 0 );

	/* Close UDP */
	efi_pxe_udp_close ( pxe, 0 );

	return 0;
}

/**
 * Perform DHCP
 *
 * @v base		PXE base code protocol
 * @v sort		Offers should be sorted
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI efi_pxe_dhcp ( EFI_PXE_BASE_CODE_PROTOCOL *base,
					BOOLEAN sort ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );
	struct net_device *netdev = pxe->netdev;
	int rc;

	DBGC ( pxe, "PXE %s DHCP %s\n",
	       pxe->name, ( sort ? "sorted" : "unsorted" ) );

	/* Claim network devices */
	efi_snp_claim();

	/* Initiate configuration */
	if ( ( rc = netdev_configure_all ( netdev ) ) != 0 ) {
		DBGC ( pxe, "PXE %s could not initiate configuration: %s\n",
		       pxe->name, strerror ( rc ) );
		goto err_configure;
	}

	/* Wait for configuration to complete (or time out) */
	while ( netdev_configuration_in_progress ( netdev ) )
		step();

	/* Report timeout if configuration failed */
	if ( ! netdev_configuration_ok ( netdev ) ) {
		rc = -ETIMEDOUT;
		goto err_timeout;
	}

	/* Update station IP address */
	if ( ( rc = efi_pxe_ip ( pxe ) ) != 0 )
		goto err_ip;

	/* Update faked DHCP packets */
	efi_pxe_fake_all ( pxe );

 err_ip:
 err_timeout:
 err_configure:
	efi_snp_release();
	return EFIRC ( rc );
}

/**
 * Perform boot server discovery
 *
 * @v base		PXE base code protocol
 * @v type		Boot server type
 * @v layer		Boot server layer
 * @v bis		Use boot integrity services
 * @v info		Additional information
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_pxe_discover ( EFI_PXE_BASE_CODE_PROTOCOL *base, UINT16 type, UINT16 *layer,
		   BOOLEAN bis, EFI_PXE_BASE_CODE_DISCOVER_INFO *info ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );
	EFI_IP_ADDRESS *ip;
	unsigned int i;

	DBGC ( pxe, "PXE %s DISCOVER type %d layer %d%s\n",
	       pxe->name, type, *layer, ( bis ? " bis" : "" ) );
	if ( info ) {
		DBGC ( pxe, "%s%s%s%s %s",
		       ( info->UseMCast ? " mcast" : "" ),
		       ( info->UseBCast ? " bcast" : "" ),
		       ( info->UseUCast ? " ucast" : "" ),
		       ( info->MustUseList ? " list" : "" ),
		       efi_pxe_ip_ntoa ( pxe, &info->ServerMCastIp ) );
		for ( i = 0 ; i < info->IpCnt ; i++ ) {
			ip = &info->SrvList[i].IpAddr;
			DBGC ( pxe, " %d%s:%s", info->SrvList[i].Type,
			       ( info->SrvList[i].AcceptAnyResponse ?
				 ":any" : "" ), efi_pxe_ip_ntoa ( pxe, ip ) );
		}
	}
	DBGC ( pxe, "\n" );

	/* Not used by any bootstrap I can find to test with */
	return EFI_UNSUPPORTED;
}

/**
 * Perform (M)TFTP
 *
 * @v base		PXE base code protocol
 * @v opcode		TFTP opcode
 * @v data		Data buffer
 * @v overwrite		Overwrite file
 * @v len		Length of data buffer
 * @v blksize		Block size
 * @v ip		Server address
 * @v filename		Filename
 * @v info		Additional information
 * @v callback		Pass packets to callback instead of data buffer
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_pxe_mtftp ( EFI_PXE_BASE_CODE_PROTOCOL *base,
		EFI_PXE_BASE_CODE_TFTP_OPCODE opcode, VOID *data,
		BOOLEAN overwrite, UINT64 *len, UINTN *blksize,
		EFI_IP_ADDRESS *ip, UINT8 *filename,
		EFI_PXE_BASE_CODE_MTFTP_INFO *info, BOOLEAN callback ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );
	int rc;

	DBGC ( pxe, "PXE %s MTFTP %d%s %p+%llx", pxe->name, opcode,
	       ( overwrite ? " overwrite" : "" ), data, *len );
	if ( blksize )
		DBGC ( pxe, " blksize %zd", ( ( size_t ) *blksize ) );
	DBGC ( pxe, " %s:%s", efi_pxe_ip_ntoa ( pxe, ip ), filename );
	if ( info ) {
		DBGC ( pxe, " %s:%d:%d:%d:%d",
		       efi_pxe_ip_ntoa ( pxe, &info->MCastIp ),
		       info->CPort, info->SPort, info->ListenTimeout,
		       info->TransmitTimeout );
	}
	DBGC ( pxe, "%s\n", ( callback ? " callback" : "" ) );

	/* Fail unless operation is supported */
	if ( ! ( ( opcode == EFI_PXE_BASE_CODE_TFTP_READ_FILE ) ||
		 ( opcode == EFI_PXE_BASE_CODE_MTFTP_READ_FILE ) ) ) {
		DBGC ( pxe, "PXE %s unsupported MTFTP opcode %d\n",
		       pxe->name, opcode );
		rc = -ENOTSUP;
		goto err_opcode;
	}

	/* Claim network devices */
	efi_snp_claim();

	/* Determine block size.  Ignore the requested block size
	 * unless we are using callbacks, since limiting HTTP to a
	 * 512-byte TCP window is not sensible.
	 */
	pxe->blksize = ( ( callback && blksize ) ? *blksize : -1UL );

	/* Initialise data transfer buffer */
	pxe->buf.data = data;
	pxe->buf.len = *len;

	/* Open download */
	if ( ( rc = efi_pxe_tftp_open ( pxe, ip,
					( ( const char * ) filename ) ) ) != 0 )
		goto err_open;

	/* Wait for download to complete */
	pxe->rc = -EINPROGRESS;
	while ( pxe->rc == -EINPROGRESS )
		step();
	if ( ( rc = pxe->rc ) != 0 ) {
		DBGC ( pxe, "PXE %s download failed: %s\n",
		       pxe->name, strerror ( rc ) );
		goto err_download;
	}

 err_download:
	efi_pxe_tftp_close ( pxe, rc );
 err_open:
	efi_snp_release();
 err_opcode:
	return EFIRC ( rc );
}

/**
 * Transmit UDP packet
 *
 * @v base		PXE base code protocol
 * @v flags		Operation flags
 * @v dest_ip		Destination address
 * @v dest_port		Destination port
 * @v gateway		Gateway address
 * @v src_ip		Source address
 * @v src_port		Source port
 * @v hdr_len		Header length
 * @v hdr		Header data
 * @v len		Length
 * @v data		Data
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_pxe_udp_write ( EFI_PXE_BASE_CODE_PROTOCOL *base, UINT16 flags,
		    EFI_IP_ADDRESS *dest_ip,
		    EFI_PXE_BASE_CODE_UDP_PORT *dest_port,
		    EFI_IP_ADDRESS *gateway, EFI_IP_ADDRESS *src_ip,
		    EFI_PXE_BASE_CODE_UDP_PORT *src_port,
		    UINTN *hdr_len, VOID *hdr, UINTN *len, VOID *data ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;
	struct io_buffer *iobuf;
	struct xfer_metadata meta;
	union {
		struct sockaddr_tcpip st;
		struct sockaddr sa;
	} dest;
	union {
		struct sockaddr_tcpip st;
		struct sockaddr sa;
	} src;
	int rc;

	DBGC2 ( pxe, "PXE %s UDP WRITE ", pxe->name );
	if ( src_ip )
		DBGC2 ( pxe, "%s", efi_pxe_ip_ntoa ( pxe, src_ip ) );
	DBGC2 ( pxe, ":" );
	if ( src_port &&
	     ( ! ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT ) ) ) {
		DBGC2 ( pxe, "%d", *src_port );
	} else {
		DBGC2 ( pxe, "*" );
	}
	DBGC2 ( pxe, "->%s:%d", efi_pxe_ip_ntoa ( pxe, dest_ip ), *dest_port );
	if ( gateway )
		DBGC2 ( pxe, " via %s", efi_pxe_ip_ntoa ( pxe, gateway ) );
	if ( hdr_len )
		DBGC2 ( pxe, " %p+%zx", hdr, ( ( size_t ) *hdr_len ) );
	DBGC2 ( pxe, " %p+%zx", data, ( ( size_t ) *len ) );
	if ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_MAY_FRAGMENT )
		DBGC2 ( pxe, " frag" );
	DBGC2 ( pxe, "\n" );

	/* Open UDP connection (if applicable) */
	if ( ( rc = efi_pxe_udp_open ( pxe ) ) != 0 )
		goto err_open;

	/* Construct destination address */
	efi_pxe_ip_sockaddr ( pxe, dest_ip, &dest.sa );
	dest.st.st_port = htons ( *dest_port );

	/* Construct source address */
	efi_pxe_ip_sockaddr ( pxe, ( src_ip ? src_ip : &mode->StationIp ),
			      &src.sa );
	if ( src_port &&
	     ( ! ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT ) ) ) {
		src.st.st_port = htons ( *src_port );
	} else {
		/* The API does not allow for a sensible concept of
		 * binding to a local port, so just use a random value.
		 */
		src.st.st_port = ( random() | htons ( 1024 ) );
		if ( src_port )
			*src_port = ntohs ( src.st.st_port );
	}

	/* Allocate I/O buffer */
	iobuf = xfer_alloc_iob ( &pxe->udp,
				 ( *len + ( hdr_len ? *hdr_len : 0 ) ) );
	if ( ! iobuf ) {
		rc = -ENOMEM;
		goto err_alloc;
	}

	/* Populate I/O buffer */
	if ( hdr_len )
		memcpy ( iob_put ( iobuf, *hdr_len ), hdr, *hdr_len );
	memcpy ( iob_put ( iobuf, *len ), data, *len );

	/* Construct metadata */
	memset ( &meta, 0, sizeof ( meta ) );
	meta.src = &src.sa;
	meta.dest = &dest.sa;
	meta.netdev = pxe->netdev;

	/* Deliver I/O buffer */
	if ( ( rc = xfer_deliver ( &pxe->udp, iob_disown ( iobuf ),
				   &meta ) ) != 0 ) {
		DBGC ( pxe, "PXE %s could not transmit: %s\n",
		       pxe->name, strerror ( rc ) );
		goto err_deliver;
	}

 err_deliver:
	free_iob ( iobuf );
 err_alloc:
	efi_pxe_udp_schedule_close ( pxe );
 err_open:
	return EFIRC ( rc );
}

/**
 * Receive UDP packet
 *
 * @v base		PXE base code protocol
 * @v flags		Operation flags
 * @v dest_ip		Destination address
 * @v dest_port		Destination port
 * @v src_ip		Source address
 * @v src_port		Source port
 * @v hdr_len		Header length
 * @v hdr		Header data
 * @v len		Length
 * @v data		Data
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_pxe_udp_read ( EFI_PXE_BASE_CODE_PROTOCOL *base, UINT16 flags,
		   EFI_IP_ADDRESS *dest_ip,
		   EFI_PXE_BASE_CODE_UDP_PORT *dest_port,
		   EFI_IP_ADDRESS *src_ip,
		   EFI_PXE_BASE_CODE_UDP_PORT *src_port,
		   UINTN *hdr_len, VOID *hdr, UINTN *len, VOID *data ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );
	struct io_buffer *iobuf;
	struct efi_pxe_udp_pseudo_header *pshdr;
	EFI_IP_ADDRESS *actual_dest_ip;
	EFI_IP_ADDRESS *actual_src_ip;
	size_t addr_len;
	size_t frag_len;
	int rc;

	DBGC2 ( pxe, "PXE %s UDP READ ", pxe->name );
	if ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_USE_FILTER ) {
		DBGC2 ( pxe, "(filter)" );
	} else if ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_IP ) {
		DBGC2 ( pxe, "*" );
	} else if ( dest_ip ) {
		DBGC2 ( pxe, "%s", efi_pxe_ip_ntoa ( pxe, dest_ip ) );
	}
	DBGC2 ( pxe, ":" );
	if ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_PORT ) {
		DBGC2 ( pxe, "*" );
	} else if ( dest_port ) {
		DBGC2 ( pxe, "%d", *dest_port );
	} else {
		DBGC2 ( pxe, "<NULL>" );
	}
	DBGC2 ( pxe, "<-" );
	if ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP ) {
		DBGC2 ( pxe, "*" );
	} else if ( src_ip ) {
		DBGC2 ( pxe, "%s", efi_pxe_ip_ntoa ( pxe, src_ip ) );
	} else {
		DBGC2 ( pxe, "<NULL>" );
	}
	DBGC2 ( pxe, ":" );
	if ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT ) {
		DBGC2 ( pxe, "*" );
	} else if ( src_port ) {
		DBGC2 ( pxe, "%d", *src_port );
	} else {
		DBGC2 ( pxe, "<NULL>" );
	}
	if ( hdr_len )
		DBGC2 ( pxe, " %p+%zx", hdr, ( ( size_t ) *hdr_len ) );
	DBGC2 ( pxe, " %p+%zx\n", data, ( ( size_t ) *len ) );

	/* Open UDP connection (if applicable) */
	if ( ( rc = efi_pxe_udp_open ( pxe ) ) != 0 )
		goto err_open;

	/* Try receiving a packet, if the queue is empty */
	if ( list_empty ( &pxe->queue ) )
		step();

	/* Remove first packet from the queue */
	iobuf = list_first_entry ( &pxe->queue, struct io_buffer, list );
	if ( ! iobuf ) {
		rc = -ETIMEDOUT; /* "no packet" */
		goto err_empty;
	}
	list_del ( &iobuf->list );

	/* Strip pseudo-header */
	pshdr = iobuf->data;
	addr_len = ( pshdr->net->net_addr_len );
	iob_pull ( iobuf, sizeof ( *pshdr ) );
	actual_dest_ip = iobuf->data;
	iob_pull ( iobuf, addr_len );
	actual_src_ip = iobuf->data;
	iob_pull ( iobuf, addr_len );
	DBGC2 ( pxe, "PXE %s UDP RX %s:%d", pxe->name,
		pshdr->net->ntoa ( actual_dest_ip ), pshdr->dest_port );
	DBGC2 ( pxe, "<-%s:%d len %#zx\n", pshdr->net->ntoa ( actual_src_ip ),
		pshdr->src_port, iob_len ( iobuf ) );

	/* Filter based on network-layer protocol */
	if ( pshdr->net != pxe->net ) {
		DBGC2 ( pxe, "PXE %s filtered out %s packet\n",
			pxe->name, pshdr->net->name );
		rc = -ETIMEDOUT; /* "no packet" */
		goto err_filter;
	}

	/* Filter based on port numbers */
	if ( ! ( ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_PORT ) ||
		 ( dest_port && ( *dest_port == pshdr->dest_port ) ) ) ) {
		DBGC2 ( pxe, "PXE %s filtered out destination port %d\n",
			pxe->name, pshdr->dest_port );
		rc = -ETIMEDOUT; /* "no packet" */
		goto err_filter;
	}
	if ( ! ( ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_PORT ) ||
		 ( src_port && ( *src_port == pshdr->src_port ) ) ) ) {
		DBGC2 ( pxe, "PXE %s filtered out source port %d\n",
			pxe->name, pshdr->src_port );
		rc = -ETIMEDOUT; /* "no packet" */
		goto err_filter;
	}

	/* Filter based on source IP address */
	if ( ! ( ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_SRC_IP ) ||
		 ( src_ip &&
		   ( memcmp ( src_ip, actual_src_ip, addr_len ) == 0 ) ) ) ) {
		DBGC2 ( pxe, "PXE %s filtered out source IP %s\n",
			pxe->name, pshdr->net->ntoa ( actual_src_ip ) );
		rc = -ETIMEDOUT; /* "no packet" */
		goto err_filter;
	}

	/* Filter based on destination IP address */
	if ( ! ( ( ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_USE_FILTER ) &&
		   efi_pxe_ip_filter ( pxe, actual_dest_ip ) ) ||
		 ( ( ! ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_USE_FILTER ) ) &&
		   ( ( flags & EFI_PXE_BASE_CODE_UDP_OPFLAGS_ANY_DEST_IP ) ||
			( dest_ip && ( memcmp ( dest_ip, actual_dest_ip,
						addr_len ) == 0 ) ) ) ) ) ) {
		DBGC2 ( pxe, "PXE %s filtered out destination IP %s\n",
			pxe->name, pshdr->net->ntoa ( actual_dest_ip ) );
		rc = -ETIMEDOUT; /* "no packet" */
		goto err_filter;
	}

	/* Fill in addresses and port numbers */
	if ( dest_ip )
		memcpy ( dest_ip, actual_dest_ip, addr_len );
	if ( dest_port )
		*dest_port = pshdr->dest_port;
	if ( src_ip )
		memcpy ( src_ip, actual_src_ip, addr_len );
	if ( src_port )
		*src_port = pshdr->src_port;

	/* Fill in header, if applicable */
	if ( hdr_len ) {
		frag_len = iob_len ( iobuf );
		if ( frag_len > *hdr_len )
			frag_len = *hdr_len;
		memcpy ( hdr, iobuf->data, frag_len );
		iob_pull ( iobuf, frag_len );
		*hdr_len = frag_len;
	}

	/* Fill in data buffer */
	frag_len = iob_len ( iobuf );
	if ( frag_len > *len )
		frag_len = *len;
	memcpy ( data, iobuf->data, frag_len );
	iob_pull ( iobuf, frag_len );
	*len = frag_len;

	/* Check for overflow */
	if ( iob_len ( iobuf ) ) {
		rc = -ERANGE;
		goto err_too_short;
	}

	/* Success */
	rc = 0;

 err_too_short:
 err_filter:
	free_iob ( iobuf );
 err_empty:
	efi_pxe_udp_schedule_close ( pxe );
 err_open:
	return EFIRC ( rc );
}

/**
 * Set receive filter
 *
 * @v base		PXE base code protocol
 * @v filter		Receive filter
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_pxe_set_ip_filter ( EFI_PXE_BASE_CODE_PROTOCOL *base,
			EFI_PXE_BASE_CODE_IP_FILTER *filter ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;
	unsigned int i;

	DBGC ( pxe, "PXE %s SET IP FILTER %02x",
	       pxe->name, filter->Filters );
	for ( i = 0 ; i < filter->IpCnt ; i++ ) {
		DBGC ( pxe, " %s",
		       efi_pxe_ip_ntoa ( pxe, &filter->IpList[i] ) );
	}
	DBGC ( pxe, "\n" );

	/* Update filter */
	memcpy ( &mode->IpFilter, filter, sizeof ( mode->IpFilter ) );

	return 0;
}

/**
 * Resolve MAC address
 *
 * @v base		PXE base code protocol
 * @v ip		IP address
 * @v mac		MAC address to fill in
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI efi_pxe_arp ( EFI_PXE_BASE_CODE_PROTOCOL *base,
				       EFI_IP_ADDRESS *ip,
				       EFI_MAC_ADDRESS *mac ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );

	DBGC ( pxe, "PXE %s ARP %s %p\n",
	       pxe->name, efi_pxe_ip_ntoa ( pxe, ip ), mac );

	/* Not used by any bootstrap I can find to test with */
	return EFI_UNSUPPORTED;
}

/**
 * Set parameters
 *
 * @v base		PXE base code protocol
 * @v autoarp		Automatic ARP packet generation
 * @v sendguid		Send GUID as client hardware address
 * @v ttl		IP time to live
 * @v tos		IP type of service
 * @v callback		Make callbacks
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_pxe_set_parameters ( EFI_PXE_BASE_CODE_PROTOCOL *base,
			 BOOLEAN *autoarp, BOOLEAN *sendguid, UINT8 *ttl,
			 UINT8 *tos, BOOLEAN *callback ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;

	DBGC ( pxe, "PXE %s SET PARAMETERS", pxe->name );
	if ( autoarp )
		DBGC ( pxe, " %s", ( *autoarp ? "autoarp" : "noautoarp" ) );
	if ( sendguid )
		DBGC ( pxe, " %s", ( *sendguid ? "sendguid" : "sendmac" ) );
	if ( ttl )
		DBGC ( pxe, " ttl %d", *ttl );
	if ( tos )
		DBGC ( pxe, " tos %d", *tos );
	if ( callback ) {
		DBGC ( pxe, " %s",
		       ( *callback ? "callback" : "nocallback" ) );
	}
	DBGC ( pxe, "\n" );

	/* Update parameters */
	if ( autoarp )
		mode->AutoArp = *autoarp;
	if ( sendguid )
		mode->SendGUID = *sendguid;
	if ( ttl )
		mode->TTL = *ttl;
	if ( tos )
		mode->ToS = *tos;
	if ( callback )
		mode->MakeCallbacks = *callback;

	return 0;
}

/**
 * Set IP address
 *
 * @v base		PXE base code protocol
 * @v ip		IP address
 * @v netmask		Subnet mask
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_pxe_set_station_ip ( EFI_PXE_BASE_CODE_PROTOCOL *base,
			 EFI_IP_ADDRESS *ip, EFI_IP_ADDRESS *netmask ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;

	DBGC ( pxe, "PXE %s SET STATION IP ", pxe->name );
	if ( ip )
		DBGC ( pxe, "%s", efi_pxe_ip_ntoa ( pxe, ip ) );
	if ( netmask )
		DBGC ( pxe, "/%s", efi_pxe_ip_ntoa ( pxe, netmask ) );
	DBGC ( pxe, "\n" );

	/* Update IP address and netmask */
	if ( ip )
		memcpy ( &mode->StationIp, ip, sizeof ( mode->StationIp ) );
	if ( netmask )
		memcpy ( &mode->SubnetMask, netmask, sizeof (mode->SubnetMask));

	return 0;
}

/**
 * Update cached DHCP packets
 *
 * @v base		PXE base code protocol
 * @v dhcpdisc_ok	DHCPDISCOVER is valid
 * @v dhcpack_ok	DHCPACK received
 * @v proxyoffer_ok	ProxyDHCPOFFER received
 * @v pxebsdisc_ok	PxeBsDISCOVER valid
 * @v pxebsack_ok	PxeBsACK received
 * @v pxebsbis_ok	PxeBsBIS received
 * @v dhcpdisc		DHCPDISCOVER packet
 * @v dhcpack		DHCPACK packet
 * @v proxyoffer	ProxyDHCPOFFER packet
 * @v pxebsdisc		PxeBsDISCOVER packet
 * @v pxebsack		PxeBsACK packet
 * @v pxebsbis		PxeBsBIS packet
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_pxe_set_packets ( EFI_PXE_BASE_CODE_PROTOCOL *base, BOOLEAN *dhcpdisc_ok,
		      BOOLEAN *dhcpack_ok, BOOLEAN *proxyoffer_ok,
		      BOOLEAN *pxebsdisc_ok, BOOLEAN *pxebsack_ok,
		      BOOLEAN *pxebsbis_ok, EFI_PXE_BASE_CODE_PACKET *dhcpdisc,
		      EFI_PXE_BASE_CODE_PACKET *dhcpack,
		      EFI_PXE_BASE_CODE_PACKET *proxyoffer,
		      EFI_PXE_BASE_CODE_PACKET *pxebsdisc,
		      EFI_PXE_BASE_CODE_PACKET *pxebsack,
		      EFI_PXE_BASE_CODE_PACKET *pxebsbis ) {
	struct efi_pxe *pxe = container_of ( base, struct efi_pxe, base );
	EFI_PXE_BASE_CODE_MODE *mode = &pxe->mode;

	DBGC ( pxe, "PXE %s SET PACKETS\n", pxe->name );

	/* Update fake packet flags */
	if ( dhcpdisc_ok )
		mode->DhcpDiscoverValid = *dhcpdisc_ok;
	if ( dhcpack_ok )
		mode->DhcpAckReceived = *dhcpack_ok;
	if ( proxyoffer_ok )
		mode->ProxyOfferReceived = *proxyoffer_ok;
	if ( pxebsdisc_ok )
		mode->PxeDiscoverValid = *pxebsdisc_ok;
	if ( pxebsack_ok )
		mode->PxeReplyReceived = *pxebsack_ok;
	if ( pxebsbis_ok )
		mode->PxeBisReplyReceived = *pxebsbis_ok;

	/* Update fake packet contents */
	if ( dhcpdisc )
		memcpy ( &mode->DhcpDiscover, dhcpdisc, sizeof ( *dhcpdisc ) );
	if ( dhcpack )
		memcpy ( &mode->DhcpAck, dhcpack, sizeof ( *dhcpack ) );
	if ( proxyoffer )
		memcpy ( &mode->ProxyOffer, proxyoffer, sizeof ( *proxyoffer ));
	if ( pxebsdisc )
		memcpy ( &mode->PxeDiscover, pxebsdisc, sizeof ( *pxebsdisc ) );
	if ( pxebsack )
		memcpy ( &mode->PxeReply, pxebsack, sizeof ( *pxebsack ) );
	if ( pxebsbis )
		memcpy ( &mode->PxeBisReply, pxebsbis, sizeof ( *pxebsbis ) );

	return 0;
}

/** PXE base code protocol */
static EFI_PXE_BASE_CODE_PROTOCOL efi_pxe_base_code_protocol = {
	.Revision	= EFI_PXE_BASE_CODE_PROTOCOL_REVISION,
	.Start		= efi_pxe_start,
	.Stop		= efi_pxe_stop,
	.Dhcp		= efi_pxe_dhcp,
	.Discover	= efi_pxe_discover,
	.Mtftp		= efi_pxe_mtftp,
	.UdpWrite	= efi_pxe_udp_write,
	.UdpRead	= efi_pxe_udp_read,
	.SetIpFilter	= efi_pxe_set_ip_filter,
	.Arp		= efi_pxe_arp,
	.SetParameters	= efi_pxe_set_parameters,
	.SetStationIp	= efi_pxe_set_station_ip,
	.SetPackets	= efi_pxe_set_packets,
};

/******************************************************************************
 *
 * Apple NetBoot protocol
 *
 ******************************************************************************
 */

/**
 * Get DHCP/BSDP response
 *
 * @v packet		Packet
 * @v len		Length of data buffer
 * @v data		Data buffer
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_apple_get_response ( EFI_PXE_BASE_CODE_PACKET *packet, UINTN *len,
			 VOID *data ) {

	/* Check length */
	if ( *len < sizeof ( *packet ) ) {
		*len = sizeof ( *packet );
		return EFI_BUFFER_TOO_SMALL;
	}

	/* Copy packet */
	memcpy ( data, packet, sizeof ( *packet ) );
	*len = sizeof ( *packet );

	return EFI_SUCCESS;
}

/**
 * Get DHCP response
 *
 * @v apple		Apple NetBoot protocol
 * @v len		Length of data buffer
 * @v data		Data buffer
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_apple_get_dhcp_response ( EFI_APPLE_NET_BOOT_PROTOCOL *apple,
			      UINTN *len, VOID *data ) {
	struct efi_pxe *pxe = container_of ( apple, struct efi_pxe, apple );

	return efi_apple_get_response ( &pxe->mode.DhcpAck, len, data );
}

/**
 * Get BSDP response
 *
 * @v apple		Apple NetBoot protocol
 * @v len		Length of data buffer
 * @v data		Data buffer
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_apple_get_bsdp_response ( EFI_APPLE_NET_BOOT_PROTOCOL *apple,
			      UINTN *len, VOID *data ) {
	struct efi_pxe *pxe = container_of ( apple, struct efi_pxe, apple );

	return efi_apple_get_response ( &pxe->mode.PxeReply, len, data );
}

/** Apple NetBoot protocol */
static EFI_APPLE_NET_BOOT_PROTOCOL efi_apple_net_boot_protocol = {
	.GetDhcpResponse = efi_apple_get_dhcp_response,
	.GetBsdpResponse = efi_apple_get_bsdp_response,
};

/******************************************************************************
 *
 * Installer
 *
 ******************************************************************************
 */

/**
 * Install PXE base code protocol
 *
 * @v handle		EFI handle
 * @v netdev		Underlying network device
 * @ret rc		Return status code
 */
int efi_pxe_install ( EFI_HANDLE handle, struct net_device *netdev ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct tcpip_net_protocol *ipv6 = tcpip_net_protocol ( AF_INET6 );
	struct efi_pxe *pxe;
	struct in_addr ip;
	BOOLEAN use_ipv6;
	EFI_STATUS efirc;
	int rc;

	/* Allocate and initialise structure */
	pxe = zalloc ( sizeof ( *pxe ) );
	if ( ! pxe ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	ref_init ( &pxe->refcnt, efi_pxe_free );
	pxe->netdev = netdev_get ( netdev );
	pxe->name = netdev->name;
	pxe->handle = handle;
	memcpy ( &pxe->base, &efi_pxe_base_code_protocol, sizeof ( pxe->base ));
	pxe->base.Mode = &pxe->mode;
	memcpy ( &pxe->apple, &efi_apple_net_boot_protocol,
		 sizeof ( pxe->apple ) );
	pxe->buf.op = &efi_pxe_buf_operations;
	intf_init ( &pxe->tftp, &efi_pxe_tftp_desc, &pxe->refcnt );
	intf_init ( &pxe->udp, &efi_pxe_udp_desc, &pxe->refcnt );
	INIT_LIST_HEAD ( &pxe->queue );
	process_init_stopped ( &pxe->process, &efi_pxe_process_desc,
			       &pxe->refcnt );

	/* Crude heuristic: assume that we prefer to use IPv4 if we
	 * have an IPv4 address for the network device, otherwise
	 * prefer IPv6 (if available).
	 */
	fetch_ipv4_setting ( netdev_settings ( netdev ), &ip_setting, &ip );
	use_ipv6 = ( ip.s_addr ? FALSE : ( ipv6 != NULL ) );

	/* Start base code */
	efi_pxe_start ( &pxe->base, use_ipv6 );

	/* Install PXE base code protocol */
	if ( ( efirc = bs->InstallMultipleProtocolInterfaces (
			&handle,
			&efi_pxe_base_code_protocol_guid, &pxe->base,
			&efi_apple_net_boot_protocol_guid, &pxe->apple,
			NULL ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( pxe, "PXE %s could not install base code protocol: %s\n",
		       pxe->name, strerror ( rc ) );
		goto err_install_protocol;
	}

	/* Transfer reference to list and return */
	list_add_tail ( &pxe->list, &efi_pxes );
	DBGC ( pxe, "PXE %s installed for %s\n",
	       pxe->name, efi_handle_name ( handle ) );
	return 0;

	bs->UninstallMultipleProtocolInterfaces (
			handle,
			&efi_pxe_base_code_protocol_guid, &pxe->base,
			&efi_apple_net_boot_protocol_guid, &pxe->apple,
			NULL );
 err_install_protocol:
	ref_put ( &pxe->refcnt );
 err_alloc:
	return rc;
}

/**
 * Uninstall PXE base code protocol
 *
 * @v handle		EFI handle
 */
void efi_pxe_uninstall ( EFI_HANDLE handle ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct efi_pxe *pxe;

	/* Locate PXE base code */
	pxe = efi_pxe_find ( handle );
	if ( ! handle ) {
		DBG ( "PXE could not find base code for %s\n",
		      efi_handle_name ( handle ) );
		return;
	}

	/* Stop base code */
	efi_pxe_stop ( &pxe->base );

	/* Uninstall PXE base code protocol */
	bs->UninstallMultipleProtocolInterfaces (
			handle,
			&efi_pxe_base_code_protocol_guid, &pxe->base,
			&efi_apple_net_boot_protocol_guid, &pxe->apple,
			NULL );

	/* Remove from list and drop list's reference */
	list_del ( &pxe->list );
	ref_put ( &pxe->refcnt );
}

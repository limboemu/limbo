/*
 * Copyright (C) 2008 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
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
 */

FILE_LICENCE ( GPL2_OR_LATER );

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <byteswap.h>
#include <ipxe/netdevice.h>
#include <ipxe/vlan.h>
#include <ipxe/iobuf.h>
#include <ipxe/in.h>
#include <ipxe/version.h>
#include <ipxe/console.h>
#include <ipxe/efi/efi.h>
#include <ipxe/efi/efi_driver.h>
#include <ipxe/efi/efi_strings.h>
#include <ipxe/efi/efi_utils.h>
#include <ipxe/efi/efi_watchdog.h>
#include <ipxe/efi/efi_snp.h>
#include <usr/autoboot.h>
#include <config/general.h>

/** List of SNP devices */
static LIST_HEAD ( efi_snp_devices );

/** Network devices are currently claimed for use by iPXE */
static int efi_snp_claimed;

/* Downgrade user experience if configured to do so
 *
 * The default UEFI user experience for network boot is somewhat
 * excremental: only TFTP is available as a download protocol, and if
 * anything goes wrong the user will be shown just a dot on an
 * otherwise blank screen.  (Some programmer was clearly determined to
 * win a bet that they could outshine Apple at producing uninformative
 * error messages.)
 *
 * For comparison, the default iPXE user experience provides the
 * option to use protocols designed more recently than 1980 (such as
 * HTTP and iSCSI), and if anything goes wrong the the user will be
 * shown one of over 1200 different error messages, complete with a
 * link to a wiki page describing that specific error.
 *
 * We default to upgrading the user experience to match that available
 * in a "legacy" BIOS environment, by installing our own instance of
 * EFI_LOAD_FILE_PROTOCOL.
 *
 * Note that unfortunately we can't sensibly provide the choice of
 * both options to the user in the same build, because the UEFI boot
 * menu ignores the multitude of ways in which a network device handle
 * can be described and opaquely labels both menu entries as just "EFI
 * Network".
 */
#ifdef EFI_DOWNGRADE_UX
static EFI_GUID dummy_load_file_protocol_guid = {
	0x6f6c7323, 0x2077, 0x7523,
	{ 0x6e, 0x68, 0x65, 0x6c, 0x70, 0x66, 0x75, 0x6c }
};
#define efi_load_file_protocol_guid dummy_load_file_protocol_guid
#endif

/**
 * Set EFI SNP mode state
 *
 * @v snp		SNP interface
 */
static void efi_snp_set_state ( struct efi_snp_device *snpdev ) {
	struct net_device *netdev = snpdev->netdev;
	EFI_SIMPLE_NETWORK_MODE *mode = &snpdev->mode;

	/* Calculate state */
	if ( ! snpdev->started ) {
		/* Start() method not called; report as Stopped */
		mode->State = EfiSimpleNetworkStopped;
	} else if ( ! netdev_is_open ( netdev ) ) {
		/* Network device not opened; report as Started */
		mode->State = EfiSimpleNetworkStarted;
	} else if ( efi_snp_claimed ) {
		/* Network device opened but claimed for use by iPXE; report
		 * as Started to inhibit receive polling.
		 */
		mode->State = EfiSimpleNetworkStarted;
	} else {
		/* Network device opened and available for use via SNP; report
		 * as Initialized.
		 */
		mode->State = EfiSimpleNetworkInitialized;
	}
}

/**
 * Set EFI SNP mode based on iPXE net device parameters
 *
 * @v snp		SNP interface
 */
static void efi_snp_set_mode ( struct efi_snp_device *snpdev ) {
	struct net_device *netdev = snpdev->netdev;
	EFI_SIMPLE_NETWORK_MODE *mode = &snpdev->mode;
	struct ll_protocol *ll_protocol = netdev->ll_protocol;
	unsigned int ll_addr_len = ll_protocol->ll_addr_len;

	mode->HwAddressSize = ll_addr_len;
	mode->MediaHeaderSize = ll_protocol->ll_header_len;
	mode->MaxPacketSize = netdev->max_pkt_len;
	mode->ReceiveFilterMask = ( EFI_SIMPLE_NETWORK_RECEIVE_UNICAST |
				    EFI_SIMPLE_NETWORK_RECEIVE_MULTICAST |
				    EFI_SIMPLE_NETWORK_RECEIVE_BROADCAST );
	assert ( ll_addr_len <= sizeof ( mode->CurrentAddress ) );
	memcpy ( &mode->CurrentAddress, netdev->ll_addr, ll_addr_len );
	memcpy ( &mode->BroadcastAddress, netdev->ll_broadcast, ll_addr_len );
	ll_protocol->init_addr ( netdev->hw_addr, &mode->PermanentAddress );
	mode->IfType = ntohs ( ll_protocol->ll_proto );
	mode->MacAddressChangeable = TRUE;
	mode->MediaPresentSupported = TRUE;
	mode->MediaPresent = ( netdev_link_ok ( netdev ) ? TRUE : FALSE );
}

/**
 * Flush transmit ring and receive queue
 *
 * @v snpdev		SNP device
 */
static void efi_snp_flush ( struct efi_snp_device *snpdev ) {
	struct io_buffer *iobuf;
	struct io_buffer *tmp;

	/* Reset transmit completion ring */
	snpdev->tx_prod = 0;
	snpdev->tx_cons = 0;

	/* Discard any queued receive buffers */
	list_for_each_entry_safe ( iobuf, tmp, &snpdev->rx, list ) {
		list_del ( &iobuf->list );
		free_iob ( iobuf );
	}
}

/**
 * Poll net device and count received packets
 *
 * @v snpdev		SNP device
 */
static void efi_snp_poll ( struct efi_snp_device *snpdev ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct io_buffer *iobuf;

	/* Poll network device */
	netdev_poll ( snpdev->netdev );

	/* Retrieve any received packets */
	while ( ( iobuf = netdev_rx_dequeue ( snpdev->netdev ) ) ) {
		list_add_tail ( &iobuf->list, &snpdev->rx );
		snpdev->interrupts |= EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT;
		bs->SignalEvent ( &snpdev->snp.WaitForPacket );
	}
}

/**
 * Change SNP state from "stopped" to "started"
 *
 * @v snp		SNP interface
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_start ( EFI_SIMPLE_NETWORK_PROTOCOL *snp ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );

	DBGC ( snpdev, "SNPDEV %p START\n", snpdev );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return EFI_NOT_READY;

	snpdev->started = 1;
	efi_snp_set_state ( snpdev );
	return 0;
}

/**
 * Change SNP state from "started" to "stopped"
 *
 * @v snp		SNP interface
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_stop ( EFI_SIMPLE_NETWORK_PROTOCOL *snp ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );

	DBGC ( snpdev, "SNPDEV %p STOP\n", snpdev );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return EFI_NOT_READY;

	snpdev->started = 0;
	efi_snp_set_state ( snpdev );

	return 0;
}

/**
 * Open the network device
 *
 * @v snp		SNP interface
 * @v extra_rx_bufsize	Extra RX buffer size, in bytes
 * @v extra_tx_bufsize	Extra TX buffer size, in bytes
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_initialize ( EFI_SIMPLE_NETWORK_PROTOCOL *snp,
		     UINTN extra_rx_bufsize, UINTN extra_tx_bufsize ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );
	int rc;

	DBGC ( snpdev, "SNPDEV %p INITIALIZE (%ld extra RX, %ld extra TX)\n",
	       snpdev, ( ( unsigned long ) extra_rx_bufsize ),
	       ( ( unsigned long ) extra_tx_bufsize ) );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return EFI_NOT_READY;

	if ( ( rc = netdev_open ( snpdev->netdev ) ) != 0 ) {
		DBGC ( snpdev, "SNPDEV %p could not open %s: %s\n",
		       snpdev, snpdev->netdev->name, strerror ( rc ) );
		return EFIRC ( rc );
	}
	efi_snp_set_state ( snpdev );

	return 0;
}

/**
 * Reset the network device
 *
 * @v snp		SNP interface
 * @v ext_verify	Extended verification required
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_reset ( EFI_SIMPLE_NETWORK_PROTOCOL *snp, BOOLEAN ext_verify ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );
	int rc;

	DBGC ( snpdev, "SNPDEV %p RESET (%s extended verification)\n",
	       snpdev, ( ext_verify ? "with" : "without" ) );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return EFI_NOT_READY;

	netdev_close ( snpdev->netdev );
	efi_snp_set_state ( snpdev );
	efi_snp_flush ( snpdev );

	if ( ( rc = netdev_open ( snpdev->netdev ) ) != 0 ) {
		DBGC ( snpdev, "SNPDEV %p could not reopen %s: %s\n",
		       snpdev, snpdev->netdev->name, strerror ( rc ) );
		return EFIRC ( rc );
	}
	efi_snp_set_state ( snpdev );

	return 0;
}

/**
 * Shut down the network device
 *
 * @v snp		SNP interface
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_shutdown ( EFI_SIMPLE_NETWORK_PROTOCOL *snp ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );

	DBGC ( snpdev, "SNPDEV %p SHUTDOWN\n", snpdev );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return EFI_NOT_READY;

	netdev_close ( snpdev->netdev );
	efi_snp_set_state ( snpdev );
	efi_snp_flush ( snpdev );

	return 0;
}

/**
 * Manage receive filters
 *
 * @v snp		SNP interface
 * @v enable		Receive filters to enable
 * @v disable		Receive filters to disable
 * @v mcast_reset	Reset multicast filters
 * @v mcast_count	Number of multicast filters
 * @v mcast		Multicast filters
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_receive_filters ( EFI_SIMPLE_NETWORK_PROTOCOL *snp, UINT32 enable,
			  UINT32 disable, BOOLEAN mcast_reset,
			  UINTN mcast_count, EFI_MAC_ADDRESS *mcast ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );
	unsigned int i;

	DBGC ( snpdev, "SNPDEV %p RECEIVE_FILTERS %08x&~%08x%s %ld mcast\n",
	       snpdev, enable, disable, ( mcast_reset ? " reset" : "" ),
	       ( ( unsigned long ) mcast_count ) );
	for ( i = 0 ; i < mcast_count ; i++ ) {
		DBGC2_HDA ( snpdev, i, &mcast[i],
			    snpdev->netdev->ll_protocol->ll_addr_len );
	}

	/* Lie through our teeth, otherwise MNP refuses to accept us.
	 *
	 * Return success even if the SNP device is currently claimed
	 * for use by iPXE, since otherwise Windows Deployment
	 * Services refuses to attempt to receive further packets via
	 * our EFI PXE Base Code protocol.
	 */
	return 0;
}

/**
 * Set station address
 *
 * @v snp		SNP interface
 * @v reset		Reset to permanent address
 * @v new		New station address
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_station_address ( EFI_SIMPLE_NETWORK_PROTOCOL *snp, BOOLEAN reset,
			  EFI_MAC_ADDRESS *new ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );
	struct ll_protocol *ll_protocol = snpdev->netdev->ll_protocol;

	DBGC ( snpdev, "SNPDEV %p STATION_ADDRESS %s\n", snpdev,
	       ( reset ? "reset" : ll_protocol->ntoa ( new ) ) );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return EFI_NOT_READY;

	/* Set the MAC address */
	if ( reset )
		new = &snpdev->mode.PermanentAddress;
	memcpy ( snpdev->netdev->ll_addr, new, ll_protocol->ll_addr_len );

	/* MAC address changes take effect only on netdev_open() */
	if ( netdev_is_open ( snpdev->netdev ) ) {
		DBGC ( snpdev, "SNPDEV %p MAC address changed while net "
		       "device open\n", snpdev );
	}

	return 0;
}

/**
 * Get (or reset) statistics
 *
 * @v snp		SNP interface
 * @v reset		Reset statistics
 * @v stats_len		Size of statistics table
 * @v stats		Statistics table
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_statistics ( EFI_SIMPLE_NETWORK_PROTOCOL *snp, BOOLEAN reset,
		     UINTN *stats_len, EFI_NETWORK_STATISTICS *stats ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );
	EFI_NETWORK_STATISTICS stats_buf;

	DBGC ( snpdev, "SNPDEV %p STATISTICS%s", snpdev,
	       ( reset ? " reset" : "" ) );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return EFI_NOT_READY;

	/* Gather statistics */
	memset ( &stats_buf, 0, sizeof ( stats_buf ) );
	stats_buf.TxGoodFrames = snpdev->netdev->tx_stats.good;
	stats_buf.TxDroppedFrames = snpdev->netdev->tx_stats.bad;
	stats_buf.TxTotalFrames = ( snpdev->netdev->tx_stats.good +
				    snpdev->netdev->tx_stats.bad );
	stats_buf.RxGoodFrames = snpdev->netdev->rx_stats.good;
	stats_buf.RxDroppedFrames = snpdev->netdev->rx_stats.bad;
	stats_buf.RxTotalFrames = ( snpdev->netdev->rx_stats.good +
				    snpdev->netdev->rx_stats.bad );
	if ( *stats_len > sizeof ( stats_buf ) )
		*stats_len = sizeof ( stats_buf );
	if ( stats )
		memcpy ( stats, &stats_buf, *stats_len );

	/* Reset statistics if requested to do so */
	if ( reset ) {
		memset ( &snpdev->netdev->tx_stats, 0,
			 sizeof ( snpdev->netdev->tx_stats ) );
		memset ( &snpdev->netdev->rx_stats, 0,
			 sizeof ( snpdev->netdev->rx_stats ) );
	}

	return 0;
}

/**
 * Convert multicast IP address to MAC address
 *
 * @v snp		SNP interface
 * @v ipv6		Address is IPv6
 * @v ip		IP address
 * @v mac		MAC address
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_mcast_ip_to_mac ( EFI_SIMPLE_NETWORK_PROTOCOL *snp, BOOLEAN ipv6,
			  EFI_IP_ADDRESS *ip, EFI_MAC_ADDRESS *mac ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );
	struct ll_protocol *ll_protocol = snpdev->netdev->ll_protocol;
	const char *ip_str;
	int rc;

	ip_str = ( ipv6 ? "(IPv6)" /* FIXME when we have inet6_ntoa() */ :
		   inet_ntoa ( *( ( struct in_addr * ) ip ) ) );
	DBGC ( snpdev, "SNPDEV %p MCAST_IP_TO_MAC %s\n", snpdev, ip_str );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return EFI_NOT_READY;

	/* Try to hash the address */
	if ( ( rc = ll_protocol->mc_hash ( ( ipv6 ? AF_INET6 : AF_INET ),
					   ip, mac ) ) != 0 ) {
		DBGC ( snpdev, "SNPDEV %p could not hash %s: %s\n",
		       snpdev, ip_str, strerror ( rc ) );
		return EFIRC ( rc );
	}

	return 0;
}

/**
 * Read or write non-volatile storage
 *
 * @v snp		SNP interface
 * @v read		Operation is a read
 * @v offset		Starting offset within NVRAM
 * @v len		Length of data buffer
 * @v data		Data buffer
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_nvdata ( EFI_SIMPLE_NETWORK_PROTOCOL *snp, BOOLEAN read,
		 UINTN offset, UINTN len, VOID *data ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );

	DBGC ( snpdev, "SNPDEV %p NVDATA %s %lx+%lx\n", snpdev,
	       ( read ? "read" : "write" ), ( ( unsigned long ) offset ),
	       ( ( unsigned long ) len ) );
	if ( ! read )
		DBGC2_HDA ( snpdev, offset, data, len );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return EFI_NOT_READY;

	return EFI_UNSUPPORTED;
}

/**
 * Read interrupt status and TX recycled buffer status
 *
 * @v snp		SNP interface
 * @v interrupts	Interrupt status, or NULL
 * @v txbuf		Recycled transmit buffer address, or NULL
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_get_status ( EFI_SIMPLE_NETWORK_PROTOCOL *snp,
		     UINT32 *interrupts, VOID **txbuf ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );

	DBGC2 ( snpdev, "SNPDEV %p GET_STATUS", snpdev );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed ) {
		DBGC2 ( snpdev, "\n" );
		return EFI_NOT_READY;
	}

	/* Poll the network device */
	efi_snp_poll ( snpdev );

	/* Interrupt status.  In practice, this seems to be used only
	 * to detect TX completions.
	 */
	if ( interrupts ) {
		*interrupts = snpdev->interrupts;
		DBGC2 ( snpdev, " INTS:%02x", *interrupts );
		snpdev->interrupts = 0;
	}

	/* TX completions */
	if ( txbuf ) {
		if ( snpdev->tx_prod != snpdev->tx_cons ) {
			*txbuf = snpdev->tx[snpdev->tx_cons++ % EFI_SNP_NUM_TX];
		} else {
			*txbuf = NULL;
		}
		DBGC2 ( snpdev, " TX:%p", *txbuf );
	}

	DBGC2 ( snpdev, "\n" );
	return 0;
}

/**
 * Start packet transmission
 *
 * @v snp		SNP interface
 * @v ll_header_len	Link-layer header length, if to be filled in
 * @v len		Length of data buffer
 * @v data		Data buffer
 * @v ll_src		Link-layer source address, if specified
 * @v ll_dest		Link-layer destination address, if specified
 * @v net_proto		Network-layer protocol (in host order)
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_transmit ( EFI_SIMPLE_NETWORK_PROTOCOL *snp,
		   UINTN ll_header_len, UINTN len, VOID *data,
		   EFI_MAC_ADDRESS *ll_src, EFI_MAC_ADDRESS *ll_dest,
		   UINT16 *net_proto ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );
	struct ll_protocol *ll_protocol = snpdev->netdev->ll_protocol;
	struct io_buffer *iobuf;
	size_t payload_len;
	unsigned int tx_fill;
	int rc;

	DBGC2 ( snpdev, "SNPDEV %p TRANSMIT %p+%lx", snpdev, data,
		( ( unsigned long ) len ) );
	if ( ll_header_len ) {
		if ( ll_src ) {
			DBGC2 ( snpdev, " src %s",
				ll_protocol->ntoa ( ll_src ) );
		}
		if ( ll_dest ) {
			DBGC2 ( snpdev, " dest %s",
				ll_protocol->ntoa ( ll_dest ) );
		}
		if ( net_proto ) {
			DBGC2 ( snpdev, " proto %04x", *net_proto );
		}
	}
	DBGC2 ( snpdev, "\n" );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return EFI_NOT_READY;

	/* Sanity checks */
	if ( ll_header_len ) {
		if ( ll_header_len != ll_protocol->ll_header_len ) {
			DBGC ( snpdev, "SNPDEV %p TX invalid header length "
			       "%ld\n", snpdev,
			       ( ( unsigned long ) ll_header_len ) );
			rc = -EINVAL;
			goto err_sanity;
		}
		if ( len < ll_header_len ) {
			DBGC ( snpdev, "SNPDEV %p invalid packet length %ld\n",
			       snpdev, ( ( unsigned long ) len ) );
			rc = -EINVAL;
			goto err_sanity;
		}
		if ( ! ll_dest ) {
			DBGC ( snpdev, "SNPDEV %p TX missing destination "
			       "address\n", snpdev );
			rc = -EINVAL;
			goto err_sanity;
		}
		if ( ! net_proto ) {
			DBGC ( snpdev, "SNPDEV %p TX missing network "
			       "protocol\n", snpdev );
			rc = -EINVAL;
			goto err_sanity;
		}
		if ( ! ll_src )
			ll_src = &snpdev->mode.CurrentAddress;
	}

	/* Allocate buffer */
	payload_len = ( len - ll_protocol->ll_header_len );
	iobuf = alloc_iob ( MAX_LL_HEADER_LEN + ( ( payload_len > IOB_ZLEN ) ?
						  payload_len : IOB_ZLEN ) );
	if ( ! iobuf ) {
		DBGC ( snpdev, "SNPDEV %p TX could not allocate %ld-byte "
		       "buffer\n", snpdev, ( ( unsigned long ) len ) );
		rc = -ENOMEM;
		goto err_alloc_iob;
	}
	iob_reserve ( iobuf, ( MAX_LL_HEADER_LEN -
			       ll_protocol->ll_header_len ) );
	memcpy ( iob_put ( iobuf, len ), data, len );

	/* Create link-layer header, if specified */
	if ( ll_header_len ) {
		iob_pull ( iobuf, ll_protocol->ll_header_len );
		if ( ( rc = ll_protocol->push ( snpdev->netdev,
						iobuf, ll_dest, ll_src,
						htons ( *net_proto ) )) != 0 ){
			DBGC ( snpdev, "SNPDEV %p TX could not construct "
			       "header: %s\n", snpdev, strerror ( rc ) );
			goto err_ll_push;
		}
	}

	/* Transmit packet */
	if ( ( rc = netdev_tx ( snpdev->netdev, iob_disown ( iobuf ) ) ) != 0){
		DBGC ( snpdev, "SNPDEV %p TX could not transmit: %s\n",
		       snpdev, strerror ( rc ) );
		goto err_tx;
	}

	/* Record in transmit completion ring.  If we run out of
	 * space, report the failure even though we have already
	 * transmitted the packet.
	 *
	 * This allows us to report completions only for packets for
	 * which we had reported successfully initiating transmission,
	 * while continuing to support clients that never poll for
	 * transmit completions.
	 */
	tx_fill = ( snpdev->tx_prod - snpdev->tx_cons );
	if ( tx_fill >= EFI_SNP_NUM_TX ) {
		DBGC ( snpdev, "SNPDEV %p TX completion ring full\n", snpdev );
		rc = -ENOBUFS;
		goto err_ring_full;
	}
	snpdev->tx[ snpdev->tx_prod++ % EFI_SNP_NUM_TX ] = data;
	snpdev->interrupts |= EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT;

	return 0;

 err_ring_full:
 err_tx:
 err_ll_push:
	free_iob ( iobuf );
 err_alloc_iob:
 err_sanity:
	return EFIRC ( rc );
}

/**
 * Receive packet
 *
 * @v snp		SNP interface
 * @v ll_header_len	Link-layer header length, if to be filled in
 * @v len		Length of data buffer
 * @v data		Data buffer
 * @v ll_src		Link-layer source address, if specified
 * @v ll_dest		Link-layer destination address, if specified
 * @v net_proto		Network-layer protocol (in host order)
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_receive ( EFI_SIMPLE_NETWORK_PROTOCOL *snp,
		  UINTN *ll_header_len, UINTN *len, VOID *data,
		  EFI_MAC_ADDRESS *ll_src, EFI_MAC_ADDRESS *ll_dest,
		  UINT16 *net_proto ) {
	struct efi_snp_device *snpdev =
		container_of ( snp, struct efi_snp_device, snp );
	struct ll_protocol *ll_protocol = snpdev->netdev->ll_protocol;
	struct io_buffer *iobuf;
	const void *iob_ll_dest;
	const void *iob_ll_src;
	uint16_t iob_net_proto;
	unsigned int iob_flags;
	int rc;

	DBGC2 ( snpdev, "SNPDEV %p RECEIVE %p(+%lx)", snpdev, data,
		( ( unsigned long ) *len ) );

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return EFI_NOT_READY;

	/* Poll the network device */
	efi_snp_poll ( snpdev );

	/* Dequeue a packet, if one is available */
	iobuf = list_first_entry ( &snpdev->rx, struct io_buffer, list );
	if ( ! iobuf ) {
		DBGC2 ( snpdev, "\n" );
		rc = -EAGAIN;
		goto out_no_packet;
	}
	list_del ( &iobuf->list );
	DBGC2 ( snpdev, "+%zx\n", iob_len ( iobuf ) );

	/* Return packet to caller */
	memcpy ( data, iobuf->data, iob_len ( iobuf ) );
	*len = iob_len ( iobuf );

	/* Attempt to decode link-layer header */
	if ( ( rc = ll_protocol->pull ( snpdev->netdev, iobuf, &iob_ll_dest,
					&iob_ll_src, &iob_net_proto,
					&iob_flags ) ) != 0 ) {
		DBGC ( snpdev, "SNPDEV %p could not parse header: %s\n",
		       snpdev, strerror ( rc ) );
		goto out_bad_ll_header;
	}

	/* Return link-layer header parameters to caller, if required */
	if ( ll_header_len )
		*ll_header_len = ll_protocol->ll_header_len;
	if ( ll_src )
		memcpy ( ll_src, iob_ll_src, ll_protocol->ll_addr_len );
	if ( ll_dest )
		memcpy ( ll_dest, iob_ll_dest, ll_protocol->ll_addr_len );
	if ( net_proto )
		*net_proto = ntohs ( iob_net_proto );

	rc = 0;

 out_bad_ll_header:
	free_iob ( iobuf );
 out_no_packet:
	return EFIRC ( rc );
}

/**
 * Poll event
 *
 * @v event		Event
 * @v context		Event context
 */
static VOID EFIAPI efi_snp_wait_for_packet ( EFI_EVENT event __unused,
					     VOID *context ) {
	struct efi_snp_device *snpdev = context;

	DBGCP ( snpdev, "SNPDEV %p WAIT_FOR_PACKET\n", snpdev );

	/* Do nothing unless the net device is open */
	if ( ! netdev_is_open ( snpdev->netdev ) )
		return;

	/* Do nothing if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed )
		return;

	/* Poll the network device */
	efi_snp_poll ( snpdev );
}

/** SNP interface */
static EFI_SIMPLE_NETWORK_PROTOCOL efi_snp_device_snp = {
	.Revision	= EFI_SIMPLE_NETWORK_PROTOCOL_REVISION,
	.Start		= efi_snp_start,
	.Stop		= efi_snp_stop,
	.Initialize	= efi_snp_initialize,
	.Reset		= efi_snp_reset,
	.Shutdown	= efi_snp_shutdown,
	.ReceiveFilters	= efi_snp_receive_filters,
	.StationAddress	= efi_snp_station_address,
	.Statistics	= efi_snp_statistics,
	.MCastIpToMac	= efi_snp_mcast_ip_to_mac,
	.NvData		= efi_snp_nvdata,
	.GetStatus	= efi_snp_get_status,
	.Transmit	= efi_snp_transmit,
	.Receive	= efi_snp_receive,
};

/******************************************************************************
 *
 * UNDI protocol
 *
 ******************************************************************************
 */

/** Union type for command parameter blocks */
typedef union {
	PXE_CPB_STATION_ADDRESS station_address;
	PXE_CPB_FILL_HEADER fill_header;
	PXE_CPB_FILL_HEADER_FRAGMENTED fill_header_fragmented;
	PXE_CPB_TRANSMIT transmit;
	PXE_CPB_RECEIVE receive;
} PXE_CPB_ANY;

/** Union type for data blocks */
typedef union {
	PXE_DB_GET_INIT_INFO get_init_info;
	PXE_DB_STATION_ADDRESS station_address;
	PXE_DB_GET_STATUS get_status;
	PXE_DB_RECEIVE receive;
} PXE_DB_ANY;

/**
 * Calculate UNDI byte checksum
 *
 * @v data		Data
 * @v len		Length of data
 * @ret sum		Checksum
 */
static uint8_t efi_undi_checksum ( void *data, size_t len ) {
	uint8_t *bytes = data;
	uint8_t sum = 0;

	while ( len-- )
		sum += *bytes++;
	return sum;
}

/**
 * Get UNDI SNP device interface number
 *
 * @v snpdev		SNP device
 * @ret ifnum		UNDI interface number
 */
static unsigned int efi_undi_ifnum ( struct efi_snp_device *snpdev ) {

	/* iPXE network device indexes are one-based (leaving zero
	 * meaning "unspecified").  UNDI interface numbers are
	 * zero-based.
	 */
	return ( snpdev->netdev->index - 1 );
}

/**
 * Identify UNDI SNP device
 *
 * @v ifnum		Interface number
 * @ret snpdev		SNP device, or NULL if not found
 */
static struct efi_snp_device * efi_undi_snpdev ( unsigned int ifnum ) {
	struct efi_snp_device *snpdev;

	list_for_each_entry ( snpdev, &efi_snp_devices, list ) {
		if ( efi_undi_ifnum ( snpdev ) == ifnum )
			return snpdev;
	}
	return NULL;
}

/**
 * Convert EFI status code to UNDI status code
 *
 * @v efirc		EFI status code
 * @ret statcode	UNDI status code
 */
static PXE_STATCODE efi_undi_statcode ( EFI_STATUS efirc ) {

	switch ( efirc ) {
	case EFI_INVALID_PARAMETER:	return PXE_STATCODE_INVALID_PARAMETER;
	case EFI_UNSUPPORTED:		return PXE_STATCODE_UNSUPPORTED;
	case EFI_OUT_OF_RESOURCES:	return PXE_STATCODE_BUFFER_FULL;
	case EFI_PROTOCOL_ERROR:	return PXE_STATCODE_DEVICE_FAILURE;
	case EFI_NOT_READY:		return PXE_STATCODE_NO_DATA;
	default:
		return PXE_STATCODE_INVALID_CDB;
	}
}

/**
 * Get state
 *
 * @v snpdev		SNP device
 * @v cdb		Command description block
 * @ret efirc		EFI status code
 */
static EFI_STATUS efi_undi_get_state ( struct efi_snp_device *snpdev,
				       PXE_CDB *cdb ) {
	EFI_SIMPLE_NETWORK_MODE *mode = &snpdev->mode;

	DBGC ( snpdev, "UNDI %p GET STATE\n", snpdev );

	/* Return current state */
	if ( mode->State == EfiSimpleNetworkInitialized ) {
		cdb->StatFlags |= PXE_STATFLAGS_GET_STATE_INITIALIZED;
	} else if ( mode->State == EfiSimpleNetworkStarted ) {
		cdb->StatFlags |= PXE_STATFLAGS_GET_STATE_STARTED;
	} else {
		cdb->StatFlags |= PXE_STATFLAGS_GET_STATE_STOPPED;
	}

	return 0;
}

/**
 * Start
 *
 * @v snpdev		SNP device
 * @ret efirc		EFI status code
 */
static EFI_STATUS efi_undi_start ( struct efi_snp_device *snpdev ) {
	EFI_STATUS efirc;

	DBGC ( snpdev, "UNDI %p START\n", snpdev );

	/* Start SNP device */
	if ( ( efirc = efi_snp_start ( &snpdev->snp ) ) != 0 )
		return efirc;

	return 0;
}

/**
 * Stop
 *
 * @v snpdev		SNP device
 * @ret efirc		EFI status code
 */
static EFI_STATUS efi_undi_stop ( struct efi_snp_device *snpdev ) {
	EFI_STATUS efirc;

	DBGC ( snpdev, "UNDI %p STOP\n", snpdev );

	/* Stop SNP device */
	if ( ( efirc = efi_snp_stop ( &snpdev->snp ) ) != 0 )
		return efirc;

	return 0;
}

/**
 * Get initialisation information
 *
 * @v snpdev		SNP device
 * @v cdb		Command description block
 * @v db		Data block
 * @ret efirc		EFI status code
 */
static EFI_STATUS efi_undi_get_init_info ( struct efi_snp_device *snpdev,
					   PXE_CDB *cdb,
					   PXE_DB_GET_INIT_INFO *db ) {
	struct net_device *netdev = snpdev->netdev;
	struct ll_protocol *ll_protocol = netdev->ll_protocol;

	DBGC ( snpdev, "UNDI %p GET INIT INFO\n", snpdev );

	/* Populate structure */
	memset ( db, 0, sizeof ( *db ) );
	db->FrameDataLen = ( netdev->max_pkt_len - ll_protocol->ll_header_len );
	db->MediaHeaderLen = ll_protocol->ll_header_len;
	db->HWaddrLen = ll_protocol->ll_addr_len;
	db->IFtype = ntohs ( ll_protocol->ll_proto );
	cdb->StatFlags |= ( PXE_STATFLAGS_CABLE_DETECT_SUPPORTED |
			    PXE_STATFLAGS_GET_STATUS_NO_MEDIA_SUPPORTED );

	return 0;
}

/**
 * Initialise
 *
 * @v snpdev		SNP device
 * @v cdb		Command description block
 * @v efirc		EFI status code
 */
static EFI_STATUS efi_undi_initialize ( struct efi_snp_device *snpdev,
					PXE_CDB *cdb ) {
	struct net_device *netdev = snpdev->netdev;
	EFI_STATUS efirc;

	DBGC ( snpdev, "UNDI %p INITIALIZE\n", snpdev );

	/* Reset SNP device */
	if ( ( efirc = efi_snp_initialize ( &snpdev->snp, 0, 0 ) ) != 0 )
		return efirc;

	/* Report link state */
	if ( ! netdev_link_ok ( netdev ) )
		cdb->StatFlags |= PXE_STATFLAGS_INITIALIZED_NO_MEDIA;

	return 0;
}

/**
 * Reset
 *
 * @v snpdev		SNP device
 * @v efirc		EFI status code
 */
static EFI_STATUS efi_undi_reset ( struct efi_snp_device *snpdev ) {
	EFI_STATUS efirc;

	DBGC ( snpdev, "UNDI %p RESET\n", snpdev );

	/* Reset SNP device */
	if ( ( efirc = efi_snp_reset ( &snpdev->snp, 0 ) ) != 0 )
		return efirc;

	return 0;
}

/**
 * Shutdown
 *
 * @v snpdev		SNP device
 * @v efirc		EFI status code
 */
static EFI_STATUS efi_undi_shutdown ( struct efi_snp_device *snpdev ) {
	EFI_STATUS efirc;

	DBGC ( snpdev, "UNDI %p SHUTDOWN\n", snpdev );

	/* Reset SNP device */
	if ( ( efirc = efi_snp_shutdown ( &snpdev->snp ) ) != 0 )
		return efirc;

	return 0;
}

/**
 * Get/set receive filters
 *
 * @v snpdev		SNP device
 * @v cdb		Command description block
 * @v efirc		EFI status code
 */
static EFI_STATUS efi_undi_receive_filters ( struct efi_snp_device *snpdev,
					     PXE_CDB *cdb ) {

	DBGC ( snpdev, "UNDI %p RECEIVE FILTERS\n", snpdev );

	/* Mark everything as supported */
	cdb->StatFlags |= ( PXE_STATFLAGS_RECEIVE_FILTER_UNICAST |
			    PXE_STATFLAGS_RECEIVE_FILTER_BROADCAST |
			    PXE_STATFLAGS_RECEIVE_FILTER_PROMISCUOUS |
			    PXE_STATFLAGS_RECEIVE_FILTER_ALL_MULTICAST );

	return 0;
}

/**
 * Get/set station address
 *
 * @v snpdev		SNP device
 * @v cdb		Command description block
 * @v cpb		Command parameter block
 * @v efirc		EFI status code
 */
static EFI_STATUS efi_undi_station_address ( struct efi_snp_device *snpdev,
					     PXE_CDB *cdb,
					     PXE_CPB_STATION_ADDRESS *cpb,
					     PXE_DB_STATION_ADDRESS *db ) {
	struct net_device *netdev = snpdev->netdev;
	struct ll_protocol *ll_protocol = netdev->ll_protocol;
	void *mac;
	int reset;
	EFI_STATUS efirc;

	DBGC ( snpdev, "UNDI %p STATION ADDRESS\n", snpdev );

	/* Update address if applicable */
	reset = ( cdb->OpFlags & PXE_OPFLAGS_STATION_ADDRESS_RESET );
	mac = ( cpb ? &cpb->StationAddr : NULL );
	if ( ( reset || mac ) &&
	     ( ( efirc = efi_snp_station_address ( &snpdev->snp, reset,
						   mac ) ) != 0 ) )
		return efirc;

	/* Fill in current addresses, if applicable */
	if ( db ) {
		memset ( db, 0, sizeof ( *db ) );
		memcpy ( &db->StationAddr, netdev->ll_addr,
			 ll_protocol->ll_addr_len );
		memcpy ( &db->BroadcastAddr, netdev->ll_broadcast,
			 ll_protocol->ll_addr_len );
		memcpy ( &db->PermanentAddr, netdev->hw_addr,
			 ll_protocol->hw_addr_len );
	}

	return 0;
}

/**
 * Get interrupt status
 *
 * @v snpdev		SNP device
 * @v cdb		Command description block
 * @v db		Data block
 * @v efirc		EFI status code
 */
static EFI_STATUS efi_undi_get_status ( struct efi_snp_device *snpdev,
					PXE_CDB *cdb, PXE_DB_GET_STATUS *db ) {
	UINT32 interrupts;
	VOID *txbuf;
	struct io_buffer *rxbuf;
	EFI_STATUS efirc;

	DBGC2 ( snpdev, "UNDI %p GET STATUS\n", snpdev );

	/* Get status */
	if ( ( efirc = efi_snp_get_status ( &snpdev->snp, &interrupts,
					    &txbuf ) ) != 0 )
		return efirc;

	/* Report status */
	memset ( db, 0, sizeof ( *db ) );
	if ( interrupts & EFI_SIMPLE_NETWORK_RECEIVE_INTERRUPT )
		cdb->StatFlags |= PXE_STATFLAGS_GET_STATUS_RECEIVE;
	if ( interrupts & EFI_SIMPLE_NETWORK_TRANSMIT_INTERRUPT )
		cdb->StatFlags |= PXE_STATFLAGS_GET_STATUS_TRANSMIT;
	if ( txbuf ) {
		db->TxBuffer[0] = ( ( intptr_t ) txbuf );
	} else {
		cdb->StatFlags |= PXE_STATFLAGS_GET_STATUS_NO_TXBUFS_WRITTEN;
		/* The specification states clearly that UNDI drivers
		 * should set TXBUF_QUEUE_EMPTY if all completed
		 * buffer addresses are written into the returned data
		 * block.  However, SnpDxe chooses to interpret
		 * TXBUF_QUEUE_EMPTY as a synonym for
		 * NO_TXBUFS_WRITTEN, thereby rendering it entirely
		 * pointless.  Work around this UEFI stupidity, as per
		 * usual.
		 */
		if ( snpdev->tx_prod == snpdev->tx_cons )
			cdb->StatFlags |=
				PXE_STATFLAGS_GET_STATUS_TXBUF_QUEUE_EMPTY;
	}
	rxbuf = list_first_entry ( &snpdev->rx, struct io_buffer, list );
	if ( rxbuf )
		db->RxFrameLen = iob_len ( rxbuf );
	if ( ! netdev_link_ok ( snpdev->netdev ) )
		cdb->StatFlags |= PXE_STATFLAGS_GET_STATUS_NO_MEDIA;

	return 0;
}

/**
 * Fill header
 *
 * @v snpdev		SNP device
 * @v cdb		Command description block
 * @v cpb		Command parameter block
 * @v efirc		EFI status code
 */
static EFI_STATUS efi_undi_fill_header ( struct efi_snp_device *snpdev,
					 PXE_CDB *cdb, PXE_CPB_ANY *cpb ) {
	struct net_device *netdev = snpdev->netdev;
	struct ll_protocol *ll_protocol = netdev->ll_protocol;
	PXE_CPB_FILL_HEADER *whole = &cpb->fill_header;
	PXE_CPB_FILL_HEADER_FRAGMENTED *fragged = &cpb->fill_header_fragmented;
	VOID *data;
	void *dest;
	void *src;
	uint16_t proto;
	struct io_buffer iobuf;
	int rc;

	/* SnpDxe will (pointlessly) use PXE_CPB_FILL_HEADER_FRAGMENTED
	 * even though we choose to explicitly not claim support for
	 * fragments via PXE_ROMID_IMP_FRAG_SUPPORTED.
	 */
	if ( cdb->OpFlags & PXE_OPFLAGS_FILL_HEADER_FRAGMENTED ) {
		data = ( ( void * ) ( intptr_t ) fragged->FragDesc[0].FragAddr);
		dest = &fragged->DestAddr;
		src = &fragged->SrcAddr;
		proto = fragged->Protocol;
	} else {
		data = ( ( void * ) ( intptr_t ) whole->MediaHeader );
		dest = &whole->DestAddr;
		src = &whole->SrcAddr;
		proto = whole->Protocol;
	}

	/* Construct link-layer header */
	iob_populate ( &iobuf, data, 0, ll_protocol->ll_header_len );
	iob_reserve ( &iobuf, ll_protocol->ll_header_len );
	if ( ( rc = ll_protocol->push ( netdev, &iobuf, dest, src,
					proto ) ) != 0 )
		return EFIRC ( rc );

	return 0;
}

/**
 * Transmit
 *
 * @v snpdev		SNP device
 * @v cpb		Command parameter block
 * @v efirc		EFI status code
 */
static EFI_STATUS efi_undi_transmit ( struct efi_snp_device *snpdev,
				      PXE_CPB_TRANSMIT *cpb ) {
	VOID *data = ( ( void * ) ( intptr_t ) cpb->FrameAddr );
	EFI_STATUS efirc;

	DBGC2 ( snpdev, "UNDI %p TRANSMIT\n", snpdev );

	/* Transmit packet */
	if ( ( efirc = efi_snp_transmit ( &snpdev->snp, 0, cpb->DataLen,
					  data, NULL, NULL, NULL ) ) != 0 )
		return efirc;

	return 0;
}

/**
 * Receive
 *
 * @v snpdev		SNP device
 * @v cpb		Command parameter block
 * @v efirc		EFI status code
 */
static EFI_STATUS efi_undi_receive ( struct efi_snp_device *snpdev,
				     PXE_CPB_RECEIVE *cpb,
				     PXE_DB_RECEIVE *db ) {
	struct net_device *netdev = snpdev->netdev;
	struct ll_protocol *ll_protocol = netdev->ll_protocol;
	VOID *data = ( ( void * ) ( intptr_t ) cpb->BufferAddr );
	UINTN hdr_len;
	UINTN len = cpb->BufferLen;
	EFI_MAC_ADDRESS src;
	EFI_MAC_ADDRESS dest;
	UINT16 proto;
	EFI_STATUS efirc;

	DBGC2 ( snpdev, "UNDI %p RECEIVE\n", snpdev );

	/* Receive packet */
	if ( ( efirc = efi_snp_receive ( &snpdev->snp, &hdr_len, &len, data,
					 &src, &dest, &proto ) ) != 0 )
		return efirc;

	/* Describe frame */
	memset ( db, 0, sizeof ( *db ) );
	memcpy ( &db->SrcAddr, &src, ll_protocol->ll_addr_len );
	memcpy ( &db->DestAddr, &dest, ll_protocol->ll_addr_len );
	db->FrameLen = len;
	db->Protocol = proto;
	db->MediaHeaderLen = ll_protocol->ll_header_len;
	db->Type = PXE_FRAME_TYPE_PROMISCUOUS;

	return 0;
}

/** UNDI entry point */
static EFIAPI VOID efi_undi_issue ( UINT64 cdb_phys ) {
	PXE_CDB *cdb = ( ( void * ) ( intptr_t ) cdb_phys );
	PXE_CPB_ANY *cpb = ( ( void * ) ( intptr_t ) cdb->CPBaddr );
	PXE_DB_ANY *db = ( ( void * ) ( intptr_t ) cdb->DBaddr );
	struct efi_snp_device *snpdev;
	EFI_STATUS efirc;

	/* Identify device */
	snpdev = efi_undi_snpdev ( cdb->IFnum );
	if ( ! snpdev ) {
		DBGC ( cdb, "UNDI invalid interface number %d\n", cdb->IFnum );
		cdb->StatCode = PXE_STATCODE_INVALID_CDB;
		cdb->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		return;
	}

	/* Fail if net device is currently claimed for use by iPXE */
	if ( efi_snp_claimed ) {
		cdb->StatCode = PXE_STATCODE_BUSY;
		cdb->StatFlags = PXE_STATFLAGS_COMMAND_FAILED;
		return;
	}

	/* Handle opcode */
	cdb->StatCode = PXE_STATCODE_SUCCESS;
	cdb->StatFlags = PXE_STATFLAGS_COMMAND_COMPLETE;
	switch ( cdb->OpCode ) {

	case PXE_OPCODE_GET_STATE:
		efirc = efi_undi_get_state ( snpdev, cdb );
		break;

	case PXE_OPCODE_START:
		efirc = efi_undi_start ( snpdev );
		break;

	case PXE_OPCODE_STOP:
		efirc = efi_undi_stop ( snpdev );
		break;

	case PXE_OPCODE_GET_INIT_INFO:
		efirc = efi_undi_get_init_info ( snpdev, cdb,
						 &db->get_init_info );
		break;

	case PXE_OPCODE_INITIALIZE:
		efirc = efi_undi_initialize ( snpdev, cdb );
		break;

	case PXE_OPCODE_RESET:
		efirc = efi_undi_reset ( snpdev );
		break;

	case PXE_OPCODE_SHUTDOWN:
		efirc = efi_undi_shutdown ( snpdev );
		break;

	case PXE_OPCODE_RECEIVE_FILTERS:
		efirc = efi_undi_receive_filters ( snpdev, cdb );
		break;

	case PXE_OPCODE_STATION_ADDRESS:
		efirc = efi_undi_station_address ( snpdev, cdb,
						   &cpb->station_address,
						   &db->station_address );
		break;

	case PXE_OPCODE_GET_STATUS:
		efirc = efi_undi_get_status ( snpdev, cdb, &db->get_status );
		break;

	case PXE_OPCODE_FILL_HEADER:
		efirc = efi_undi_fill_header ( snpdev, cdb, cpb );
		break;

	case PXE_OPCODE_TRANSMIT:
		efirc = efi_undi_transmit ( snpdev, &cpb->transmit );
		break;

	case PXE_OPCODE_RECEIVE:
		efirc = efi_undi_receive ( snpdev, &cpb->receive,
					   &db->receive );
		break;

	default:
		DBGC ( snpdev, "UNDI %p unsupported opcode %#04x\n",
		       snpdev, cdb->OpCode );
		efirc = EFI_UNSUPPORTED;
		break;
	}

	/* Convert EFI status code to UNDI status code */
	if ( efirc != 0 ) {
		cdb->StatFlags &= ~PXE_STATFLAGS_STATUS_MASK;
		cdb->StatFlags |= PXE_STATFLAGS_COMMAND_FAILED;
		cdb->StatCode = efi_undi_statcode ( efirc );
	}
}

/** UNDI interface
 *
 * Must be aligned on a 16-byte boundary, for no particularly good
 * reason.
 */
static PXE_SW_UNDI efi_snp_undi __attribute__ (( aligned ( 16 ) )) = {
	.Signature	= PXE_ROMID_SIGNATURE,
	.Len		= sizeof ( efi_snp_undi ),
	.Rev		= PXE_ROMID_REV,
	.MajorVer	= PXE_ROMID_MAJORVER,
	.MinorVer	= PXE_ROMID_MINORVER,
	.Implementation	= ( PXE_ROMID_IMP_SW_VIRT_ADDR |
			    PXE_ROMID_IMP_STATION_ADDR_SETTABLE |
			    PXE_ROMID_IMP_PROMISCUOUS_MULTICAST_RX_SUPPORTED |
			    PXE_ROMID_IMP_PROMISCUOUS_RX_SUPPORTED |
			    PXE_ROMID_IMP_BROADCAST_RX_SUPPORTED |
			    PXE_ROMID_IMP_TX_COMPLETE_INT_SUPPORTED |
			    PXE_ROMID_IMP_PACKET_RX_INT_SUPPORTED ),
	/* SnpDxe checks that BusCnt is non-zero.  It makes no further
	 * use of BusCnt, and never looks as BusType[].  As with much
	 * of the EDK2 code, this check seems to serve no purpose
	 * whatsoever but must nonetheless be humoured.
	 */
	.BusCnt		= 1,
	.BusType[0]	= PXE_BUSTYPE ( 'i', 'P', 'X', 'E' ),
};

/** Network Identification Interface (NII) */
static EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL efi_snp_device_nii = {
	.Revision	= EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_REVISION,
	.StringId	= "UNDI",
	.Type		= EfiNetworkInterfaceUndi,
	.MajorVer	= 3,
	.MinorVer	= 1,
	.Ipv6Supported	= TRUE, /* This is a raw packet interface, FFS! */
};

/******************************************************************************
 *
 * Component name protocol
 *
 ******************************************************************************
 */

/**
 * Look up driver name
 *
 * @v name2		Component name protocol
 * @v language		Language to use
 * @v driver_name	Driver name to fill in
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_get_driver_name ( EFI_COMPONENT_NAME2_PROTOCOL *name2,
			  CHAR8 *language __unused, CHAR16 **driver_name ) {
	struct efi_snp_device *snpdev =
		container_of ( name2, struct efi_snp_device, name2 );

	*driver_name = snpdev->driver_name;
	return 0;
}

/**
 * Look up controller name
 *
 * @v name2     		Component name protocol
 * @v device		Device
 * @v child		Child device, or NULL
 * @v language		Language to use
 * @v driver_name	Device name to fill in
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_get_controller_name ( EFI_COMPONENT_NAME2_PROTOCOL *name2,
			      EFI_HANDLE device __unused,
			      EFI_HANDLE child __unused,
			      CHAR8 *language __unused,
			      CHAR16 **controller_name ) {
	struct efi_snp_device *snpdev =
		container_of ( name2, struct efi_snp_device, name2 );

	*controller_name = snpdev->controller_name;
	return 0;
}

/******************************************************************************
 *
 * Load file protocol
 *
 ******************************************************************************
 */

/**
 * Load file
 *
 * @v loadfile		Load file protocol
 * @v path		File path
 * @v booting		Loading as part of a boot attempt
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_snp_load_file ( EFI_LOAD_FILE_PROTOCOL *load_file,
		    EFI_DEVICE_PATH_PROTOCOL *path __unused,
		    BOOLEAN booting, UINTN *len __unused,
		    VOID *data __unused ) {
	struct efi_snp_device *snpdev =
		container_of ( load_file, struct efi_snp_device, load_file );
	struct net_device *netdev = snpdev->netdev;
	int rc;

	/* Fail unless this is a boot attempt */
	if ( ! booting ) {
		DBGC ( snpdev, "SNPDEV %p cannot load non-boot file\n",
		       snpdev );
		return EFI_UNSUPPORTED;
	}

	/* Claim network devices for use by iPXE */
	efi_snp_claim();

	/* Start watchdog holdoff timer */
	efi_watchdog_start();

	/* Boot from network device */
	if ( ( rc = ipxe ( netdev ) ) != 0 )
		goto err_ipxe;

	/* Reset console */
	console_reset();

 err_ipxe:
	efi_watchdog_stop();
	efi_snp_release();
	return EFIRC ( rc );
}

/** Load file protocol */
static EFI_LOAD_FILE_PROTOCOL efi_snp_load_file_protocol = {
	.LoadFile	= efi_snp_load_file,
};

/******************************************************************************
 *
 * iPXE network driver
 *
 ******************************************************************************
 */

/**
 * Locate SNP device corresponding to network device
 *
 * @v netdev		Network device
 * @ret snp		SNP device, or NULL if not found
 */
static struct efi_snp_device * efi_snp_demux ( struct net_device *netdev ) {
	struct efi_snp_device *snpdev;

	list_for_each_entry ( snpdev, &efi_snp_devices, list ) {
		if ( snpdev->netdev == netdev )
			return snpdev;
	}
	return NULL;
}

/**
 * Create SNP device
 *
 * @v netdev		Network device
 * @ret rc		Return status code
 */
static int efi_snp_probe ( struct net_device *netdev ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct efi_device *efidev;
	struct efi_snp_device *snpdev;
	EFI_DEVICE_PATH_PROTOCOL *path_end;
	MAC_ADDR_DEVICE_PATH *macpath;
	VLAN_DEVICE_PATH *vlanpath;
	size_t path_prefix_len = 0;
	unsigned int ifcnt;
	unsigned int tag;
	void *interface;
	EFI_STATUS efirc;
	int rc;

	/* Find parent EFI device */
	efidev = efidev_parent ( netdev->dev );
	if ( ! efidev ) {
		DBG ( "SNP skipping non-EFI device %s\n", netdev->name );
		rc = 0;
		goto err_no_efidev;
	}

	/* Allocate the SNP device */
	snpdev = zalloc ( sizeof ( *snpdev ) );
	if ( ! snpdev ) {
		rc = -ENOMEM;
		goto err_alloc_snp;
	}
	snpdev->netdev = netdev_get ( netdev );
	snpdev->efidev = efidev;
	INIT_LIST_HEAD ( &snpdev->rx );

	/* Sanity check */
	if ( netdev->ll_protocol->ll_addr_len > sizeof ( EFI_MAC_ADDRESS ) ) {
		DBGC ( snpdev, "SNPDEV %p cannot support link-layer address "
		       "length %d for %s\n", snpdev,
		       netdev->ll_protocol->ll_addr_len, netdev->name );
		rc = -ENOTSUP;
		goto err_ll_addr_len;
	}

	/* Populate the SNP structure */
	memcpy ( &snpdev->snp, &efi_snp_device_snp, sizeof ( snpdev->snp ) );
	snpdev->snp.Mode = &snpdev->mode;
	if ( ( efirc = bs->CreateEvent ( EVT_NOTIFY_WAIT, TPL_NOTIFY,
					 efi_snp_wait_for_packet, snpdev,
					 &snpdev->snp.WaitForPacket ) ) != 0 ){
		rc = -EEFI ( efirc );
		DBGC ( snpdev, "SNPDEV %p could not create event: %s\n",
		       snpdev, strerror ( rc ) );
		goto err_create_event;
	}

	/* Populate the SNP mode structure */
	snpdev->mode.State = EfiSimpleNetworkStopped;
	efi_snp_set_mode ( snpdev );

	/* Populate the NII structure */
	memcpy ( &snpdev->nii, &efi_snp_device_nii, sizeof ( snpdev->nii ) );
	snpdev->nii.Id = ( ( intptr_t ) &efi_snp_undi );
	snpdev->nii.IfNum = efi_undi_ifnum ( snpdev );
	efi_snp_undi.EntryPoint = ( ( intptr_t ) efi_undi_issue );
	ifcnt = ( ( efi_snp_undi.IFcntExt << 8 ) | efi_snp_undi.IFcnt );
	if ( ifcnt < snpdev->nii.IfNum )
		ifcnt = snpdev->nii.IfNum;
	efi_snp_undi.IFcnt = ( ifcnt & 0xff );
	efi_snp_undi.IFcntExt = ( ifcnt >> 8 );
	efi_snp_undi.Fudge -= efi_undi_checksum ( &efi_snp_undi,
						  sizeof ( efi_snp_undi ) );

	/* Populate the component name structure */
	efi_snprintf ( snpdev->driver_name,
		       ( sizeof ( snpdev->driver_name ) /
			 sizeof ( snpdev->driver_name[0] ) ),
		       "%s %s", product_short_name, netdev->dev->driver_name );
	efi_snprintf ( snpdev->controller_name,
		       ( sizeof ( snpdev->controller_name ) /
			 sizeof ( snpdev->controller_name[0] ) ),
		       "%s %s (%s, %s)", product_short_name,
		       netdev->dev->driver_name, netdev->dev->name,
		       netdev_addr ( netdev ) );
	snpdev->name2.GetDriverName = efi_snp_get_driver_name;
	snpdev->name2.GetControllerName = efi_snp_get_controller_name;
	snpdev->name2.SupportedLanguages = "en";

	/* Populate the load file protocol structure */
	memcpy ( &snpdev->load_file, &efi_snp_load_file_protocol,
		 sizeof ( snpdev->load_file ) );

	/* Populate the device name */
	efi_snprintf ( snpdev->name, ( sizeof ( snpdev->name ) /
				       sizeof ( snpdev->name[0] ) ),
		       "%s", netdev->name );

	/* Allocate the new device path */
	path_prefix_len = efi_devpath_len ( efidev->path );
	snpdev->path = zalloc ( path_prefix_len + sizeof ( *macpath ) +
				sizeof ( *vlanpath ) + sizeof ( *path_end ) );
	if ( ! snpdev->path ) {
		rc = -ENOMEM;
		goto err_alloc_device_path;
	}

	/* Populate the device path */
	memcpy ( snpdev->path, efidev->path, path_prefix_len );
	macpath = ( ( ( void * ) snpdev->path ) + path_prefix_len );
	memset ( macpath, 0, sizeof ( *macpath ) );
	macpath->Header.Type = MESSAGING_DEVICE_PATH;
	macpath->Header.SubType = MSG_MAC_ADDR_DP;
	macpath->Header.Length[0] = sizeof ( *macpath );
	memcpy ( &macpath->MacAddress, netdev->ll_addr,
		 netdev->ll_protocol->ll_addr_len );
	macpath->IfType = ntohs ( netdev->ll_protocol->ll_proto );
	if ( ( tag = vlan_tag ( netdev ) ) ) {
		vlanpath = ( ( ( void * ) macpath ) + sizeof ( *macpath ) );
		memset ( vlanpath, 0, sizeof ( *vlanpath ) );
		vlanpath->Header.Type = MESSAGING_DEVICE_PATH;
		vlanpath->Header.SubType = MSG_VLAN_DP;
		vlanpath->Header.Length[0] = sizeof ( *vlanpath );
		vlanpath->VlanId = tag;
		path_end = ( ( ( void * ) vlanpath ) + sizeof ( *vlanpath ) );
	} else {
		path_end = ( ( ( void * ) macpath ) + sizeof ( *macpath ) );
	}
	memset ( path_end, 0, sizeof ( *path_end ) );
	path_end->Type = END_DEVICE_PATH_TYPE;
	path_end->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
	path_end->Length[0] = sizeof ( *path_end );

	/* Install the SNP */
	if ( ( efirc = bs->InstallMultipleProtocolInterfaces (
			&snpdev->handle,
			&efi_simple_network_protocol_guid, &snpdev->snp,
			&efi_device_path_protocol_guid, snpdev->path,
			&efi_nii_protocol_guid, &snpdev->nii,
			&efi_nii31_protocol_guid, &snpdev->nii,
			&efi_component_name2_protocol_guid, &snpdev->name2,
			&efi_load_file_protocol_guid, &snpdev->load_file,
			NULL ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( snpdev, "SNPDEV %p could not install protocols: %s\n",
		       snpdev, strerror ( rc ) );
		goto err_install_protocol_interface;
	}

	/* SnpDxe will repeatedly start up and shut down our NII/UNDI
	 * interface (in order to obtain the MAC address) before
	 * discovering that it cannot install another SNP on the same
	 * handle.  This causes the underlying network device to be
	 * unexpectedly closed.
	 *
	 * Prevent this by opening our own NII (and NII31) protocol
	 * instances to prevent SnpDxe from attempting to bind to
	 * them.
	 */
	if ( ( efirc = bs->OpenProtocol ( snpdev->handle,
					  &efi_nii_protocol_guid, &interface,
					  efi_image_handle, snpdev->handle,
					  ( EFI_OPEN_PROTOCOL_BY_DRIVER |
					    EFI_OPEN_PROTOCOL_EXCLUSIVE )))!=0){
		rc = -EEFI ( efirc );
		DBGC ( snpdev, "SNPDEV %p could not open NII protocol: %s\n",
		       snpdev, strerror ( rc ) );
		goto err_open_nii;
	}
	if ( ( efirc = bs->OpenProtocol ( snpdev->handle,
					  &efi_nii31_protocol_guid, &interface,
					  efi_image_handle, snpdev->handle,
					  ( EFI_OPEN_PROTOCOL_BY_DRIVER |
					    EFI_OPEN_PROTOCOL_EXCLUSIVE )))!=0){
		rc = -EEFI ( efirc );
		DBGC ( snpdev, "SNPDEV %p could not open NII31 protocol: %s\n",
		       snpdev, strerror ( rc ) );
		goto err_open_nii31;
	}

	/* Add as child of EFI parent device */
	if ( ( rc = efi_child_add ( efidev->device, snpdev->handle ) ) != 0 ) {
		DBGC ( snpdev, "SNPDEV %p could not become child of %s: %s\n",
		       snpdev, efi_handle_name ( efidev->device ),
		       strerror ( rc ) );
		goto err_efi_child_add;
	}

	/* Install HII */
	if ( ( rc = efi_snp_hii_install ( snpdev ) ) != 0 ) {
		DBGC ( snpdev, "SNPDEV %p could not install HII: %s\n",
		       snpdev, strerror ( rc ) );
		/* HII fails on several platforms.  It's
		 * non-essential, so treat this as a non-fatal
		 * error.
		 */
	}

	/* Add to list of SNP devices */
	list_add ( &snpdev->list, &efi_snp_devices );

	/* Close device path */
	bs->CloseProtocol ( efidev->device, &efi_device_path_protocol_guid,
			    efi_image_handle, efidev->device );

	DBGC ( snpdev, "SNPDEV %p installed for %s as device %s\n",
	       snpdev, netdev->name, efi_handle_name ( snpdev->handle ) );
	return 0;

	list_del ( &snpdev->list );
	if ( snpdev->package_list )
		efi_snp_hii_uninstall ( snpdev );
	efi_child_del ( efidev->device, snpdev->handle );
 err_efi_child_add:
	bs->CloseProtocol ( snpdev->handle, &efi_nii_protocol_guid,
			    efi_image_handle, snpdev->handle );
 err_open_nii:
	bs->CloseProtocol ( snpdev->handle, &efi_nii31_protocol_guid,
			    efi_image_handle, snpdev->handle );
 err_open_nii31:
	bs->UninstallMultipleProtocolInterfaces (
			snpdev->handle,
			&efi_simple_network_protocol_guid, &snpdev->snp,
			&efi_device_path_protocol_guid, snpdev->path,
			&efi_nii_protocol_guid, &snpdev->nii,
			&efi_nii31_protocol_guid, &snpdev->nii,
			&efi_component_name2_protocol_guid, &snpdev->name2,
			&efi_load_file_protocol_guid, &snpdev->load_file,
			NULL );
 err_install_protocol_interface:
	free ( snpdev->path );
 err_alloc_device_path:
	bs->CloseEvent ( snpdev->snp.WaitForPacket );
 err_create_event:
 err_ll_addr_len:
	netdev_put ( netdev );
	free ( snpdev );
 err_alloc_snp:
 err_no_efidev:
	return rc;
}

/**
 * Handle SNP device or link state change
 *
 * @v netdev		Network device
 */
static void efi_snp_notify ( struct net_device *netdev ) {
	struct efi_snp_device *snpdev;

	/* Locate SNP device */
	snpdev = efi_snp_demux ( netdev );
	if ( ! snpdev ) {
		DBG ( "SNP skipping non-SNP device %s\n", netdev->name );
		return;
	}

	/* Update link state */
	snpdev->mode.MediaPresent =
		( netdev_link_ok ( netdev ) ? TRUE : FALSE );
	DBGC ( snpdev, "SNPDEV %p link is %s\n", snpdev,
	       ( snpdev->mode.MediaPresent ? "up" : "down" ) );

	/* Update mode state */
	efi_snp_set_state ( snpdev );
}

/**
 * Destroy SNP device
 *
 * @v netdev		Network device
 */
static void efi_snp_remove ( struct net_device *netdev ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct efi_snp_device *snpdev;

	/* Locate SNP device */
	snpdev = efi_snp_demux ( netdev );
	if ( ! snpdev ) {
		DBG ( "SNP skipping non-SNP device %s\n", netdev->name );
		return;
	}

	/* Uninstall the SNP */
	list_del ( &snpdev->list );
	if ( snpdev->package_list )
		efi_snp_hii_uninstall ( snpdev );
	efi_child_del ( snpdev->efidev->device, snpdev->handle );
	bs->CloseProtocol ( snpdev->handle, &efi_nii_protocol_guid,
			    efi_image_handle, snpdev->handle );
	bs->CloseProtocol ( snpdev->handle, &efi_nii31_protocol_guid,
			    efi_image_handle, snpdev->handle );
	bs->UninstallMultipleProtocolInterfaces (
			snpdev->handle,
			&efi_simple_network_protocol_guid, &snpdev->snp,
			&efi_device_path_protocol_guid, snpdev->path,
			&efi_nii_protocol_guid, &snpdev->nii,
			&efi_nii31_protocol_guid, &snpdev->nii,
			&efi_component_name2_protocol_guid, &snpdev->name2,
			&efi_load_file_protocol_guid, &snpdev->load_file,
			NULL );
	free ( snpdev->path );
	bs->CloseEvent ( snpdev->snp.WaitForPacket );
	netdev_put ( snpdev->netdev );
	free ( snpdev );
}

/** SNP driver */
struct net_driver efi_snp_driver __net_driver = {
	.name = "SNP",
	.probe = efi_snp_probe,
	.notify = efi_snp_notify,
	.remove = efi_snp_remove,
};

/**
 * Find SNP device by EFI device handle
 *
 * @v handle		EFI device handle
 * @ret snpdev		SNP device, or NULL
 */
struct efi_snp_device * find_snpdev ( EFI_HANDLE handle ) {
	struct efi_snp_device *snpdev;

	list_for_each_entry ( snpdev, &efi_snp_devices, list ) {
		if ( snpdev->handle == handle )
			return snpdev;
	}
	return NULL;
}

/**
 * Get most recently opened SNP device
 *
 * @ret snpdev		Most recently opened SNP device, or NULL
 */
struct efi_snp_device * last_opened_snpdev ( void ) {
	struct net_device *netdev;

	netdev = last_opened_netdev();
	if ( ! netdev )
		return NULL;

	return efi_snp_demux ( netdev );
}

/**
 * Add to SNP claimed/released count
 *
 * @v delta		Claim count change
 */
void efi_snp_add_claim ( int delta ) {
	struct efi_snp_device *snpdev;

	/* Claim SNP devices */
	efi_snp_claimed += delta;
	assert ( efi_snp_claimed >= 0 );

	/* Update SNP mode state for each interface */
	list_for_each_entry ( snpdev, &efi_snp_devices, list )
		efi_snp_set_state ( snpdev );
}

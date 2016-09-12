/*
 * Copyright (C) 2016 Michael Brown <mbrown@fensystems.co.uk>.
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

#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ipxe/netdevice.h>
#include <ipxe/ethernet.h>
#include <ipxe/if_ether.h>
#include <ipxe/profile.h>
#include <ipxe/usb.h>
#include "axge.h"

/** @file
 *
 * Asix 10/100/1000 USB Ethernet driver
 *
 * Large chunks of functionality are undocumented in the available
 * datasheets.  The gaps are deduced from combinations of the Linux
 * driver, the FreeBSD driver, and experimentation with the hardware.
 */

/** Interrupt completion profiler */
static struct profiler axge_intr_profiler __profiler =
	{ .name = "axge.intr" };

/** Bulk IN completion profiler */
static struct profiler axge_in_profiler __profiler =
	{ .name = "axge.in" };

/** Bulk OUT profiler */
static struct profiler axge_out_profiler __profiler =
	{ .name = "axge.out" };

/** Default bulk IN configuration
 *
 * The Linux and FreeBSD drivers have set of magic constants which are
 * chosen based on both the Ethernet and USB link speeds.
 *
 * Experimentation shows that setting the "timer" value to zero seems
 * to prevent the device from ever coalescing multiple packets into a
 * single bulk IN transfer.  This allows us to get away with using a
 * 2kB receive I/O buffer and a zerocopy receive path.
 */
static struct axge_bulk_in_control axge_bicr = {
	.ctrl = 7,
	.timer = cpu_to_le16 ( 0 ),
	.size = 0,
	.ifg = 0,
};

/******************************************************************************
 *
 * Register access
 *
 ******************************************************************************
 */

/**
 * Read register
 *
 * @v asix		AXGE device
 * @v offset		Register offset
 * @v data		Data buffer
 * @v len		Length of data
 * @ret rc		Return status code
 */
static inline int axge_read_register ( struct axge_device *axge,
				       unsigned int offset, void *data,
				       size_t len ) {

	return usb_control ( axge->usb, AXGE_READ_MAC_REGISTER,
			     offset, len, data, len );
}

/**
 * Read one-byte register
 *
 * @v asix		AXGE device
 * @v offset		Register offset
 * @v value		Value to fill in
 * @ret rc		Return status code
 */
static inline int axge_read_byte ( struct axge_device *axge,
				   unsigned int offset, uint8_t *value ) {

	return axge_read_register ( axge, offset, value, sizeof ( *value ) );
}

/**
 * Read two-byte register
 *
 * @v asix		AXGE device
 * @v offset		Register offset
 * @v value		Value to fill in
 * @ret rc		Return status code
 */
static inline int axge_read_word ( struct axge_device *axge,
				   unsigned int offset, uint16_t *value ) {

	return axge_read_register ( axge, offset, value, sizeof ( *value ) );
}

/**
 * Read four-byte register
 *
 * @v asix		AXGE device
 * @v offset		Register offset
 * @v value		Value to fill in
 * @ret rc		Return status code
 */
static inline int axge_read_dword ( struct axge_device *axge,
				    unsigned int offset, uint32_t *value ) {

	return axge_read_register ( axge, offset, value, sizeof ( *value ) );
}

/**
 * Write register
 *
 * @v asix		AXGE device
 * @v offset		Register offset
 * @v data		Data buffer
 * @v len		Length of data
 * @ret rc		Return status code
 */
static inline int axge_write_register ( struct axge_device *axge,
					unsigned int offset, void *data,
					size_t len ) {

	return usb_control ( axge->usb, AXGE_WRITE_MAC_REGISTER,
			     offset, len, data, len );
}

/**
 * Write one-byte register
 *
 * @v asix		AXGE device
 * @v offset		Register offset
 * @v value		Value
 * @ret rc		Return status code
 */
static inline int axge_write_byte ( struct axge_device *axge,
				    unsigned int offset, uint8_t value ) {

	return axge_write_register ( axge, offset, &value, sizeof ( value ));
}

/**
 * Write two-byte register
 *
 * @v asix		AXGE device
 * @v offset		Register offset
 * @v value		Value
 * @ret rc		Return status code
 */
static inline int axge_write_word ( struct axge_device *axge,
				    unsigned int offset, uint16_t value ) {

	return axge_write_register ( axge, offset, &value, sizeof ( value ));
}

/**
 * Write one-byte register
 *
 * @v asix		AXGE device
 * @v offset		Register offset
 * @v value		Value
 * @ret rc		Return status code
 */
static inline int axge_write_dword ( struct axge_device *axge,
				     unsigned int offset, uint32_t value ) {

	return axge_write_register ( axge, offset, &value, sizeof ( value ));
}

/******************************************************************************
 *
 * Link status
 *
 ******************************************************************************
 */

/**
 * Get link status
 *
 * @v asix		AXGE device
 * @ret rc		Return status code
 */
static int axge_check_link ( struct axge_device *axge ) {
	struct net_device *netdev = axge->netdev;
	uint8_t plsr;
	int rc;

	/* Read physical link status register */
	if ( ( rc = axge_read_byte ( axge, AXGE_PLSR, &plsr ) ) != 0 ) {
		DBGC ( axge, "AXGE %p could not read PLSR: %s\n",
		       axge, strerror ( rc ) );
		return rc;
	}

	/* Update link status */
	if ( plsr & AXGE_PLSR_EPHY_ANY ) {
		DBGC ( axge, "AXGE %p link up (PLSR %02x)\n", axge, plsr );
		netdev_link_up ( netdev );
	} else {
		DBGC ( axge, "AXGE %p link down (PLSR %02x)\n", axge, plsr );
		netdev_link_down ( netdev );
	}

	return 0;
}

/******************************************************************************
 *
 * AXGE communications interface
 *
 ******************************************************************************
 */

/**
 * Complete interrupt transfer
 *
 * @v ep		USB endpoint
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void axge_intr_complete ( struct usb_endpoint *ep,
				 struct io_buffer *iobuf, int rc ) {
	struct axge_device *axge = container_of ( ep, struct axge_device,
						  usbnet.intr );
	struct net_device *netdev = axge->netdev;
	struct axge_interrupt *intr;
	size_t len = iob_len ( iobuf );
	unsigned int link_ok;

	/* Profile completions */
	profile_start ( &axge_intr_profiler );

	/* Ignore packets cancelled when the endpoint closes */
	if ( ! ep->open )
		goto ignore;

	/* Drop packets with errors */
	if ( rc != 0 ) {
		DBGC ( axge, "AXGE %p interrupt failed: %s\n",
		       axge, strerror ( rc ) );
		DBGC_HDA ( axge, 0, iobuf->data, iob_len ( iobuf ) );
		goto error;
	}

	/* Extract message header */
	if ( len < sizeof ( *intr ) ) {
		DBGC ( axge, "AXGE %p underlength interrupt:\n", axge );
		DBGC_HDA ( axge, 0, iobuf->data, iob_len ( iobuf ) );
		rc = -EINVAL;
		goto error;
	}
	intr = iobuf->data;

	/* Check magic signature */
	if ( intr->magic != cpu_to_le16 ( AXGE_INTR_MAGIC ) ) {
		DBGC ( axge, "AXGE %p malformed interrupt:\n", axge );
		DBGC_HDA ( axge, 0, iobuf->data, iob_len ( iobuf ) );
		rc = -EINVAL;
		goto error;
	}

	/* Extract link status */
	link_ok = ( intr->link & cpu_to_le16 ( AXGE_INTR_LINK_PPLS ) );
	if ( link_ok && ! netdev_link_ok ( netdev ) ) {
		DBGC ( axge, "AXGE %p link up\n", axge );
		netdev_link_up ( netdev );
	} else if ( netdev_link_ok ( netdev ) && ! link_ok ) {
		DBGC ( axge, "AXGE %p link down\n", axge );
		netdev_link_down ( netdev );
	}

	/* Free I/O buffer */
	free_iob ( iobuf );
	profile_stop ( &axge_intr_profiler );

	return;

 error:
	netdev_rx_err ( netdev, iob_disown ( iobuf ), rc );
 ignore:
	free_iob ( iobuf );
	return;
}

/** Interrupt endpoint operations */
static struct usb_endpoint_driver_operations axge_intr_operations = {
	.complete = axge_intr_complete,
};

/******************************************************************************
 *
 * AXGE data interface
 *
 ******************************************************************************
 */

/**
 * Complete bulk IN transfer
 *
 * @v ep		USB endpoint
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void axge_in_complete ( struct usb_endpoint *ep,
			       struct io_buffer *iobuf, int rc ) {
	struct axge_device *axge = container_of ( ep, struct axge_device,
						  usbnet.in );
	struct net_device *netdev = axge->netdev;
	struct axge_rx_footer *ftr;
	struct axge_rx_descriptor *desc;
	struct io_buffer *pkt;
	unsigned int count;
	unsigned int offset;
	size_t len;
	size_t padded_len;

	/* Profile receive completions */
	profile_start ( &axge_in_profiler );

	/* Ignore packets cancelled when the endpoint closes */
	if ( ! ep->open )
		goto ignore;

	/* Record USB errors against the network device */
	if ( rc != 0 ) {
		DBGC ( axge, "AXGE %p bulk IN failed: %s\n",
		       axge, strerror ( rc ) );
		goto error;
	}

	/* Sanity check */
	if ( iob_len ( iobuf ) < sizeof ( *ftr ) ) {
		DBGC ( axge, "AXGE %p underlength bulk IN:\n", axge );
		DBGC_HDA ( axge, 0, iobuf->data, iob_len ( iobuf ) );
		rc = -EINVAL;
		goto error;
	}

	/* Parse ftr, strip ftr and descriptors */
	iob_unput ( iobuf, sizeof ( *ftr ) );
	ftr = ( iobuf->data + iob_len ( iobuf ) );
	count = le16_to_cpu ( ftr->count );
	if ( count == 0 ) {
		DBGC ( axge, "AXGE %p zero-packet bulk IN:\n", axge );
		DBGC_HDA ( axge, 0, iobuf->data, iob_len ( iobuf ) );
		goto ignore;
	}
	offset = le16_to_cpu ( ftr->offset );
	if ( ( iob_len ( iobuf ) < offset ) ||
	     ( ( iob_len ( iobuf ) - offset ) < ( count * sizeof ( *desc ) ) )){
		DBGC ( axge, "AXGE %p malformed bulk IN footer:\n", axge );
		DBGC_HDA ( axge, 0, iobuf->data, iob_len ( iobuf ) );
		rc = -EINVAL;
		goto error;
	}
	desc = ( iobuf->data + offset );
	iob_unput ( iobuf, ( iob_len ( iobuf ) - offset ) );

	/* Process packets */
	for ( ; count-- ; desc++ ) {

		/* Parse descriptor */
		len = ( le16_to_cpu ( desc->len_flags ) & AXGE_RX_LEN_MASK );
		padded_len = ( ( len + AXGE_RX_LEN_PAD_ALIGN - 1 ) &
			       ~( AXGE_RX_LEN_PAD_ALIGN - 1 ) );
		if ( iob_len ( iobuf ) < padded_len ) {
			DBGC ( axge, "AXGE %p malformed bulk IN descriptor:\n",
			       axge );
			DBGC_HDA ( axge, 0, iobuf->data, iob_len ( iobuf ) );
			rc = -EINVAL;
			goto error;
		}

		/* Check for previous dropped packets */
		if ( desc->len_flags & cpu_to_le16 ( AXGE_RX_CRC_ERROR ) )
			netdev_rx_err ( netdev, NULL, -EIO );
		if ( desc->len_flags & cpu_to_le16 ( AXGE_RX_DROP_ERROR ) )
			netdev_rx_err ( netdev, NULL, -ENOBUFS );

		/* Allocate new I/O buffer, if applicable */
		if ( count ) {

			/* More packets remain: allocate a new buffer */
			pkt = alloc_iob ( AXGE_IN_RESERVE + len );
			if ( ! pkt ) {
				/* Record error and continue */
				netdev_rx_err ( netdev, NULL, -ENOMEM );
				iob_pull ( iobuf, padded_len );
				continue;
			}
			iob_reserve ( pkt, AXGE_IN_RESERVE );
			memcpy ( iob_put ( pkt, len ), iobuf->data, len );
			iob_pull ( iobuf, padded_len );

		} else {

			/* This is the last (or only) packet: use this buffer */
			iob_unput ( iobuf, ( padded_len - len ) );
			pkt = iob_disown ( iobuf );
		}

		/* Hand off to network stack */
		netdev_rx ( netdev, iob_disown ( pkt ) );
	}

	assert ( iobuf == NULL );
	profile_stop ( &axge_in_profiler );
	return;

 error:
	netdev_rx_err ( netdev, iob_disown ( iobuf ), rc );
 ignore:
	free_iob ( iobuf );
}

/** Bulk IN endpoint operations */
static struct usb_endpoint_driver_operations axge_in_operations = {
	.complete = axge_in_complete,
};

/**
 * Transmit packet
 *
 * @v asix		AXGE device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int axge_out_transmit ( struct axge_device *axge,
			       struct io_buffer *iobuf ) {
	struct axge_tx_header *hdr;
	size_t len = iob_len ( iobuf );
	int rc;

	/* Profile transmissions */
	profile_start ( &axge_out_profiler );

	/* Prepend header */
	if ( ( rc = iob_ensure_headroom ( iobuf, sizeof ( *hdr ) ) ) != 0 )
		return rc;
	hdr = iob_push ( iobuf, sizeof ( *hdr ) );
	hdr->len = cpu_to_le32 ( len );
	hdr->wtf = 0;

	/* Enqueue I/O buffer */
	if ( ( rc = usb_stream ( &axge->usbnet.out, iobuf, 0 ) ) != 0 )
		return rc;

	profile_stop ( &axge_out_profiler );
	return 0;
}

/**
 * Complete bulk OUT transfer
 *
 * @v ep		USB endpoint
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void axge_out_complete ( struct usb_endpoint *ep,
				struct io_buffer *iobuf, int rc ) {
	struct axge_device *axge = container_of ( ep, struct axge_device,
						  usbnet.out );
	struct net_device *netdev = axge->netdev;

	/* Report TX completion */
	netdev_tx_complete_err ( netdev, iobuf, rc );
}

/** Bulk OUT endpoint operations */
static struct usb_endpoint_driver_operations axge_out_operations = {
	.complete = axge_out_complete,
};

/******************************************************************************
 *
 * Network device interface
 *
 ******************************************************************************
 */

/**
 * Open network device
 *
 * @v netdev		Network device
 * @ret rc		Return status code
 */
static int axge_open ( struct net_device *netdev ) {
	struct axge_device *axge = netdev->priv;
	uint16_t rcr;
	int rc;

	/* Open USB network device */
	if ( ( rc = usbnet_open ( &axge->usbnet ) ) != 0 ) {
		DBGC ( axge, "AXGE %p could not open: %s\n",
		       axge, strerror ( rc ) );
		goto err_open;
	}

	/* Set MAC address */
	if ( ( rc = axge_write_register ( axge, AXGE_NIDR,
					  netdev->ll_addr, ETH_ALEN ) ) !=0){
		DBGC ( axge, "AXGE %p could not set MAC address: %s\n",
		       axge, strerror ( rc ) );
		goto err_write_mac;
	}

	/* Enable receiver */
	rcr = cpu_to_le16 ( AXGE_RCR_PRO | AXGE_RCR_AMALL |
			    AXGE_RCR_AB | AXGE_RCR_SO );
	if ( ( rc = axge_write_word ( axge, AXGE_RCR, rcr ) ) != 0 ) {
		DBGC ( axge, "AXGE %p could not write RCR: %s\n",
		       axge, strerror ( rc ) );
		goto err_write_rcr;
	}

	/* Update link status */
	axge_check_link ( axge );

	return 0;

	axge_write_word ( axge, AXGE_RCR, 0 );
 err_write_rcr:
 err_write_mac:
	usbnet_close ( &axge->usbnet );
 err_open:
	return rc;
}

/**
 * Close network device
 *
 * @v netdev		Network device
 */
static void axge_close ( struct net_device *netdev ) {
	struct axge_device *axge = netdev->priv;

	/* Disable receiver */
	axge_write_word ( axge, AXGE_RCR, 0 );

	/* Close USB network device */
	usbnet_close ( &axge->usbnet );
}

/**
 * Transmit packet
 *
 * @v netdev		Network device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int axge_transmit ( struct net_device *netdev,
			   struct io_buffer *iobuf ) {
	struct axge_device *axge = netdev->priv;
	int rc;

	/* Transmit packet */
	if ( ( rc = axge_out_transmit ( axge, iobuf ) ) != 0 )
		return rc;

	return 0;
}

/**
 * Poll for completed and received packets
 *
 * @v netdev		Network device
 */
static void axge_poll ( struct net_device *netdev ) {
	struct axge_device *axge = netdev->priv;
	int rc;

	/* Poll USB bus */
	usb_poll ( axge->bus );

	/* Refill endpoints */
	if ( ( rc = usbnet_refill ( &axge->usbnet ) ) != 0 )
		netdev_rx_err ( netdev, NULL, rc );
}

/** AXGE network device operations */
static struct net_device_operations axge_operations = {
	.open		= axge_open,
	.close		= axge_close,
	.transmit	= axge_transmit,
	.poll		= axge_poll,
};

/******************************************************************************
 *
 * USB interface
 *
 ******************************************************************************
 */

/**
 * Probe device
 *
 * @v func		USB function
 * @v config		Configuration descriptor
 * @ret rc		Return status code
 */
static int axge_probe ( struct usb_function *func,
			struct usb_configuration_descriptor *config ) {
	struct usb_device *usb = func->usb;
	struct net_device *netdev;
	struct axge_device *axge;
	uint16_t epprcr;
	uint16_t msr;
	uint8_t csr;
	int rc;

	/* Allocate and initialise structure */
	netdev = alloc_etherdev ( sizeof ( *axge ) );
	if ( ! netdev ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	netdev_init ( netdev, &axge_operations );
	netdev->dev = &func->dev;
	axge = netdev->priv;
	memset ( axge, 0, sizeof ( *axge ) );
	axge->usb = usb;
	axge->bus = usb->port->hub->bus;
	axge->netdev = netdev;
	usbnet_init ( &axge->usbnet, func, &axge_intr_operations,
		      &axge_in_operations, &axge_out_operations );
	usb_refill_init ( &axge->usbnet.intr, 0, 0, AXGE_INTR_MAX_FILL );
	usb_refill_init ( &axge->usbnet.in, AXGE_IN_RESERVE,
			  AXGE_IN_MTU, AXGE_IN_MAX_FILL );
	DBGC ( axge, "AXGE %p on %s\n", axge, func->name );

	/* Describe USB network device */
	if ( ( rc = usbnet_describe ( &axge->usbnet, config ) ) != 0 ) {
		DBGC ( axge, "AXGE %p could not describe: %s\n",
		       axge, strerror ( rc ) );
		goto err_describe;
	}

	/* Fetch MAC address */
	if ( ( rc = axge_read_register ( axge, AXGE_NIDR, netdev->hw_addr,
					 ETH_ALEN ) ) != 0 ) {
		DBGC ( axge, "AXGE %p could not fetch MAC address: %s\n",
		       axge, strerror ( rc ) );
		goto err_read_mac;
	}

	/* Power up PHY */
	if ( ( rc = axge_write_word ( axge, AXGE_EPPRCR, 0 ) ) != 0 ) {
		DBGC ( axge, "AXGE %p could not write EPPRCR: %s\n",
		       axge, strerror ( rc ) );
		goto err_write_epprcr_off;
	}
	epprcr = cpu_to_le16 ( AXGE_EPPRCR_IPRL );
	if ( ( rc = axge_write_word ( axge, AXGE_EPPRCR, epprcr ) ) != 0){
		DBGC ( axge, "AXGE %p could not write EPPRCR: %s\n",
		       axge, strerror ( rc ) );
		goto err_write_epprcr_on;
	}
	mdelay ( AXGE_EPPRCR_DELAY_MS );

	/* Select clocks */
	csr = ( AXGE_CSR_BCS | AXGE_CSR_ACS );
	if ( ( rc = axge_write_byte ( axge, AXGE_CSR, csr ) ) != 0){
		DBGC ( axge, "AXGE %p could not write CSR: %s\n",
		       axge, strerror ( rc ) );
		goto err_write_csr;
	}
	mdelay ( AXGE_CSR_DELAY_MS );

	/* Configure bulk IN pipeline */
	if ( ( rc = axge_write_register ( axge, AXGE_BICR, &axge_bicr,
					  sizeof ( axge_bicr ) ) ) != 0 ){
		DBGC ( axge, "AXGE %p could not write BICR: %s\n",
		       axge, strerror ( rc ) );
		goto err_write_bicr;
	}

	/* Set medium status */
	msr = cpu_to_le16 ( AXGE_MSR_GM | AXGE_MSR_FD | AXGE_MSR_RFC |
			    AXGE_MSR_TFC | AXGE_MSR_RE );
	if ( ( rc = axge_write_word ( axge, AXGE_MSR, msr ) ) != 0 ) {
		DBGC ( axge, "AXGE %p could not write MSR: %s\n",
		       axge, strerror ( rc ) );
		goto err_write_msr;
	}

	/* Register network device */
	if ( ( rc = register_netdev ( netdev ) ) != 0 )
		goto err_register;

	/* Update link status */
	axge_check_link ( axge );

	usb_func_set_drvdata ( func, axge );
	return 0;

	unregister_netdev ( netdev );
 err_register:
 err_write_msr:
 err_write_bicr:
 err_write_csr:
 err_write_epprcr_on:
 err_write_epprcr_off:
 err_read_mac:
 err_describe:
	netdev_nullify ( netdev );
	netdev_put ( netdev );
 err_alloc:
	return rc;
}

/**
 * Remove device
 *
 * @v func		USB function
 */
static void axge_remove ( struct usb_function *func ) {
	struct axge_device *axge = usb_func_get_drvdata ( func );
	struct net_device *netdev = axge->netdev;

	unregister_netdev ( netdev );
	netdev_nullify ( netdev );
	netdev_put ( netdev );
}

/** AXGE device IDs */
static struct usb_device_id axge_ids[] = {
	{
		.name = "ax88179",
		.vendor = 0x0b95,
		.product = 0x1790,
	},
	{
		.name = "ax88178a",
		.vendor = 0x0b95,
		.product = 0x178a,
	},
	{
		.name = "dub1312",
		.vendor = 0x2001,
		.product = 0x4a00,
	},
	{
		.name = "axge-sitecom",
		.vendor = 0x0df6,
		.product = 0x0072,
	},
	{
		.name = "axge-samsung",
		.vendor = 0x04e8,
		.product = 0xa100,
	},
	{
		.name = "onelinkdock",
		.vendor = 0x17ef,
		.product = 0x304b,
	},
};

/** AXGE driver */
struct usb_driver axge_driver __usb_driver = {
	.ids = axge_ids,
	.id_count = ( sizeof ( axge_ids ) / sizeof ( axge_ids[0] ) ),
	.class = USB_CLASS_ID ( USB_ANY_ID, USB_ANY_ID, USB_ANY_ID ),
	.score = USB_SCORE_NORMAL,
	.probe = axge_probe,
	.remove = axge_remove,
};

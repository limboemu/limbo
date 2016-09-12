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

#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <byteswap.h>
#include <ipxe/profile.h>
#include <ipxe/usb.h>
#include <ipxe/usbnet.h>
#include <ipxe/rndis.h>
#include "acm.h"

/** @file
 *
 * USB RNDIS driver
 *
 */

/** Interrupt completion profiler */
static struct profiler acm_intr_profiler __profiler =
	{ .name = "acm.intr" };

/** Bulk IN completion profiler */
static struct profiler acm_in_profiler __profiler =
	{ .name = "acm.in" };

/** Bulk OUT profiler */
static struct profiler acm_out_profiler __profiler =
	{ .name = "acm.out" };

/******************************************************************************
 *
 * USB RNDIS communications interface
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
static void acm_intr_complete ( struct usb_endpoint *ep,
				struct io_buffer *iobuf, int rc ) {
	struct acm_device *acm = container_of ( ep, struct acm_device,
						usbnet.intr );
	struct rndis_device *rndis = acm->rndis;
	struct usb_setup_packet *message;

	/* Profile completions */
	profile_start ( &acm_intr_profiler );

	/* Ignore packets cancelled when the endpoint closes */
	if ( ! ep->open )
		goto ignore;

	/* Drop packets with errors */
	if ( rc != 0 ) {
		DBGC ( acm, "ACM %p interrupt failed: %s\n",
		       acm, strerror ( rc ) );
		DBGC_HDA ( acm, 0, iobuf->data, iob_len ( iobuf ) );
		goto error;
	}

	/* Extract message header */
	if ( iob_len ( iobuf ) < sizeof ( *message ) ) {
		DBGC ( acm, "ACM %p underlength interrupt:\n", acm );
		DBGC_HDA ( acm, 0, iobuf->data, iob_len ( iobuf ) );
		rc = -EINVAL;
		goto error;
	}
	message = iobuf->data;

	/* Parse message header */
	switch ( message->request ) {

	case cpu_to_le16 ( CDC_RESPONSE_AVAILABLE ) :
	case cpu_to_le16 ( 0x0001 ) : /* qemu seems to use this value */
		acm->responded = 1;
		break;

	default:
		DBGC ( acm, "ACM %p unrecognised interrupt:\n", acm );
		DBGC_HDA ( acm, 0, iobuf->data, iob_len ( iobuf ) );
		rc = -ENOTSUP;
		goto error;
	}

	/* Free I/O buffer */
	free_iob ( iobuf );
	profile_stop ( &acm_intr_profiler );

	return;

 error:
	rndis_rx_err ( rndis, iob_disown ( iobuf ), rc );
 ignore:
	free_iob ( iobuf );
	return;
}

/** Interrupt endpoint operations */
static struct usb_endpoint_driver_operations acm_intr_operations = {
	.complete = acm_intr_complete,
};

/******************************************************************************
 *
 * USB RNDIS data interface
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
static void acm_in_complete ( struct usb_endpoint *ep, struct io_buffer *iobuf,
			      int rc ) {
	struct acm_device *acm = container_of ( ep, struct acm_device,
						usbnet.in );
	struct rndis_device *rndis = acm->rndis;

	/* Profile receive completions */
	profile_start ( &acm_in_profiler );

	/* Ignore packets cancelled when the endpoint closes */
	if ( ! ep->open )
		goto ignore;

	/* Record USB errors against the RNDIS device */
	if ( rc != 0 ) {
		DBGC ( acm, "ACM %p bulk IN failed: %s\n",
		       acm, strerror ( rc ) );
		goto error;
	}

	/* Hand off to RNDIS */
	rndis_rx ( rndis, iob_disown ( iobuf ) );

	profile_stop ( &acm_in_profiler );
	return;

 error:
	rndis_rx_err ( rndis, iob_disown ( iobuf ), rc );
 ignore:
	free_iob ( iobuf );
}

/** Bulk IN endpoint operations */
static struct usb_endpoint_driver_operations acm_in_operations = {
	.complete = acm_in_complete,
};

/**
 * Transmit packet
 *
 * @v acm		USB RNDIS device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int acm_out_transmit ( struct acm_device *acm,
			      struct io_buffer *iobuf ) {
	int rc;

	/* Profile transmissions */
	profile_start ( &acm_out_profiler );

	/* Enqueue I/O buffer */
	if ( ( rc = usb_stream ( &acm->usbnet.out, iobuf, 0 ) ) != 0 )
		return rc;

	profile_stop ( &acm_out_profiler );
	return 0;
}

/**
 * Complete bulk OUT transfer
 *
 * @v ep		USB endpoint
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void acm_out_complete ( struct usb_endpoint *ep, struct io_buffer *iobuf,
			       int rc ) {
	struct acm_device *acm = container_of ( ep, struct acm_device,
						usbnet.out );
	struct rndis_device *rndis = acm->rndis;

	/* Report TX completion */
	rndis_tx_complete_err ( rndis, iobuf, rc );
}

/** Bulk OUT endpoint operations */
static struct usb_endpoint_driver_operations acm_out_operations = {
	.complete = acm_out_complete,
};

/******************************************************************************
 *
 * USB RNDIS control interface
 *
 ******************************************************************************
 */

/**
 * Send control packet
 *
 * @v acm		USB RNDIS device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int acm_control_transmit ( struct acm_device *acm,
				  struct io_buffer *iobuf ) {
	struct rndis_device *rndis = acm->rndis;
	struct usb_device *usb = acm->usb;
	int rc;

	/* Send packet as an encapsulated command */
	if ( ( rc = cdc_send_encapsulated_command ( usb, acm->usbnet.comms,
						    iobuf->data,
						    iob_len ( iobuf ) ) ) != 0){
		DBGC ( acm, "ACM %p could not send encapsulated command: %s\n",
		       acm, strerror ( rc ) );
		return rc;
	}

	/* Complete packet immediately */
	rndis_tx_complete ( rndis, iobuf );

	return 0;
}

/**
 * Receive control packet
 *
 * @v acm		USB RNDIS device
 * @ret rc		Return status code
 */
static int acm_control_receive ( struct acm_device *acm ) {
	struct rndis_device *rndis = acm->rndis;
	struct usb_device *usb = acm->usb;
	struct io_buffer *iobuf;
	struct rndis_header *header;
	size_t mtu = ACM_RESPONSE_MTU;
	size_t len;
	int rc;

	/* Allocate I/O buffer */
	iobuf = alloc_iob ( mtu );
	if ( ! iobuf ) {
		rc = -ENOMEM;
		goto err_alloc;
	}

	/* Get encapsulated response */
	if ( ( rc = cdc_get_encapsulated_response ( usb, acm->usbnet.comms,
						    iobuf->data, mtu ) ) != 0 ){
		DBGC ( acm, "ACM %p could not get encapsulated response: %s\n",
		       acm, strerror ( rc ) );
		goto err_get_response;
	}

	/* Fix up buffer length */
	header = iobuf->data;
	len = le32_to_cpu ( header->len );
	if ( len > mtu ) {
		DBGC ( acm, "ACM %p overlength encapsulated response\n", acm );
		DBGC_HDA ( acm, 0, iobuf->data, mtu );
		rc = -EPROTO;
		goto err_len;
	}
	iob_put ( iobuf, len );

	/* Hand off to RNDIS */
	rndis_rx ( rndis, iob_disown ( iobuf ) );

	return 0;

 err_len:
 err_get_response:
	free_iob ( iobuf );
 err_alloc:
	return rc;
}

/******************************************************************************
 *
 * RNDIS interface
 *
 ******************************************************************************
 */

/**
 * Open RNDIS device
 *
 * @v rndis		RNDIS device
 * @ret rc		Return status code
 */
static int acm_open ( struct rndis_device *rndis ) {
	struct acm_device *acm = rndis->priv;
	int rc;

	/* Open USB network device */
	if ( ( rc = usbnet_open ( &acm->usbnet ) ) != 0 )
		goto err_open;

	return 0;

	usbnet_close ( &acm->usbnet );
 err_open:
	return rc;
}

/**
 * Close RNDIS device
 *
 * @v rndis		RNDIS device
 */
static void acm_close ( struct rndis_device *rndis ) {
	struct acm_device *acm = rndis->priv;

	/* Close USB network device */
	usbnet_close ( &acm->usbnet );
}

/**
 * Transmit packet
 *
 * @v rndis		RNDIS device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int acm_transmit ( struct rndis_device *rndis,
			  struct io_buffer *iobuf ) {
	struct acm_device *acm = rndis->priv;
	struct rndis_header *header = iobuf->data;

	/* Sanity check */
	assert ( iob_len ( iobuf ) >= sizeof ( *header ) );
	assert ( iob_len ( iobuf ) == le32_to_cpu ( header->len ) );

	/* Transmit packet via appropriate mechanism */
	if ( header->type == cpu_to_le32 ( RNDIS_PACKET_MSG ) ) {
		return acm_out_transmit ( acm, iobuf );
	} else {
		return acm_control_transmit ( acm, iobuf );
	}
}

/**
 * Poll for completed and received packets
 *
 * @v rndis		RNDIS device
 */
static void acm_poll ( struct rndis_device *rndis ) {
	struct acm_device *acm = rndis->priv;
	int rc;

	/* Poll USB bus */
	usb_poll ( acm->bus );

	/* Refill rings */
	if ( ( rc = usbnet_refill ( &acm->usbnet ) ) != 0 )
		rndis_rx_err ( rndis, NULL, rc );

	/* Retrieve encapsulated response, if applicable */
	if ( acm->responded ) {

		/* Clear flag */
		acm->responded = 0;

		/* Get encapsulated response */
		if ( ( rc = acm_control_receive ( acm ) ) != 0 )
			rndis_rx_err ( rndis, NULL, rc );
	}
}

/** USB RNDIS operations */
static struct rndis_operations acm_operations = {
	.open		= acm_open,
	.close		= acm_close,
	.transmit	= acm_transmit,
	.poll		= acm_poll,
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
static int acm_probe ( struct usb_function *func,
		       struct usb_configuration_descriptor *config ) {
	struct usb_device *usb = func->usb;
	struct rndis_device *rndis;
	struct acm_device *acm;
	int rc;

	/* Allocate and initialise structure */
	rndis = alloc_rndis ( sizeof ( *acm ) );
	if ( ! rndis ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	rndis_init ( rndis, &acm_operations );
	rndis->netdev->dev = &func->dev;
	acm = rndis->priv;
	acm->usb = usb;
	acm->bus = usb->port->hub->bus;
	acm->rndis = rndis;
	usbnet_init ( &acm->usbnet, func, &acm_intr_operations,
		      &acm_in_operations, &acm_out_operations );
	usb_refill_init ( &acm->usbnet.intr, 0, 0, ACM_INTR_MAX_FILL );
	usb_refill_init ( &acm->usbnet.in, 0, ACM_IN_MTU, ACM_IN_MAX_FILL );

	/* Describe USB network device */
	if ( ( rc = usbnet_describe ( &acm->usbnet, config ) ) != 0 ) {
		DBGC ( acm, "ACM %p could not describe: %s\n",
		       acm, strerror ( rc ) );
		goto err_describe;
	}

	/* Register RNDIS device */
	if ( ( rc = register_rndis ( rndis ) ) != 0 )
		goto err_register;

	usb_func_set_drvdata ( func, acm );
	return 0;

	unregister_rndis ( rndis );
 err_register:
 err_describe:
	free_rndis ( rndis );
 err_alloc:
	return rc;
}

/**
 * Remove device
 *
 * @v func		USB function
 */
static void acm_remove ( struct usb_function *func ) {
	struct acm_device *acm = usb_func_get_drvdata ( func );
	struct rndis_device *rndis = acm->rndis;

	/* Unregister RNDIS device */
	unregister_rndis ( rndis );

	/* Free RNDIS device */
	free_rndis ( rndis );
}

/** USB CDC-ACM device IDs */
static struct usb_device_id cdc_acm_ids[] = {
	{
		.name = "cdc-acm",
		.vendor = USB_ANY_ID,
		.product = USB_ANY_ID,
	},
};

/** USB CDC-ACM driver */
struct usb_driver cdc_acm_driver __usb_driver = {
	.ids = cdc_acm_ids,
	.id_count = ( sizeof ( cdc_acm_ids ) / sizeof ( cdc_acm_ids[0] ) ),
	.class = USB_CLASS_ID ( USB_CLASS_CDC, USB_SUBCLASS_CDC_ACM,
				USB_PROTOCOL_ACM_RNDIS ),
	.score = USB_SCORE_DEPRECATED,
	.probe = acm_probe,
	.remove = acm_remove,
};

/** USB RF-RNDIS device IDs */
static struct usb_device_id rf_rndis_ids[] = {
	{
		.name = "rf-rndis",
		.vendor = USB_ANY_ID,
		.product = USB_ANY_ID,
	},
};

/** USB RF-RNDIS driver */
struct usb_driver rf_rndis_driver __usb_driver = {
	.ids = rf_rndis_ids,
	.id_count = ( sizeof ( rf_rndis_ids ) / sizeof ( rf_rndis_ids[0] ) ),
	.class = USB_CLASS_ID ( USB_CLASS_WIRELESS, USB_SUBCLASS_WIRELESS_RADIO,
				USB_PROTOCOL_RADIO_RNDIS ),
	.score = USB_SCORE_DEPRECATED,
	.probe = acm_probe,
	.remove = acm_remove,
};

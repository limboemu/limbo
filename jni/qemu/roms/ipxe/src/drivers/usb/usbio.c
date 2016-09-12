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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ipxe/efi/efi.h>
#include <ipxe/efi/efi_driver.h>
#include <ipxe/efi/efi_utils.h>
#include <ipxe/efi/Protocol/UsbIo.h>
#include <ipxe/usb.h>
#include "usbio.h"

/** @file
 *
 * EFI_USB_IO_PROTOCOL pseudo Host Controller Interface driver
 *
 *
 * The EFI_USB_IO_PROTOCOL is an almost unbelievably poorly designed
 * abstraction of a USB device.  It would be just about forgivable for
 * an API to support only synchronous operation for bulk OUT
 * endpoints.  It is imbecilic to support only synchronous operation
 * for bulk IN endpoints.  This apparently intern-designed API
 * throttles a typical NIC down to 1.5% of its maximum throughput.
 * That isn't a typo.  It really is that slow.
 *
 * We can't even work around this stupidity by talking to the host
 * controller abstraction directly, because an identical limitation
 * exists in the EFI_USB2_HC_PROTOCOL.
 *
 * Unless you derive therapeutic value from watching download progress
 * indicators lethargically creep through every single integer from 0
 * to 100, you should use iPXE's native USB host controller drivers
 * instead.  (Or just upgrade from UEFI to "legacy" BIOS, which will
 * produce a similar speed increase.)
 *
 *
 * For added excitement, the EFI_USB_IO_PROTOCOL makes the
 * (demonstrably incorrect) assumption that a USB driver needs to
 * attach to exactly one interface within a USB device, and provides a
 * helper method to retrieve "the" interface descriptor.  Since pretty
 * much every USB network device requires binding to a pair of
 * control+data interfaces, this aspect of EFI_USB_IO_PROTOCOL is of
 * no use to us.
 *
 * We have our own existing code for reading USB descriptors, so we
 * don't actually care that the UsbGetInterfaceDescriptor() method
 * provided by EFI_USB_IO_PROTOCOL is useless for network devices.  We
 * can read the descriptors ourselves (via UsbControlTransfer()) and
 * get all of the information we need this way.  We can even work
 * around the fact that EFI_USB_IO_PROTOCOL provides separate handles
 * for each of the two interfaces comprising our network device.
 *
 * However, if we discover that we need to select an alternative
 * device configuration (e.g. for devices exposing both RNDIS and
 * ECM), then all hell breaks loose.  EFI_USB_IO_PROTOCOL starts to
 * panic because its cached interface and endpoint descriptors will no
 * longer be valid.  As mentioned above, the cached descriptors are
 * useless for network devices anyway so we _really_ don't care about
 * this, but EFI_USB_IO_PROTOCOL certainly cares.  It prints out a
 * manic warning message containing no fewer than six exclamation
 * marks and then literally commits seppuku in the middle of the
 * UsbControlTransfer() method by attempting to uninstall itself.
 * Quite how the caller is supposed to react when asked to stop using
 * the EFI_USB_IO_PROTOCOL instance while in the middle of an
 * uninterruptible call to said instance is left as an exercise for
 * the interested reader.
 *
 * There is no sensible way to work around this, so we just
 * preemptively fail if asked to change the device configuration, on
 * the basis that reporting a sarcastic error message is often
 * preferable to jumping through a NULL pointer and crashing the
 * system.
 */

/* Disambiguate the various error causes */
#define ENOTSUP_MORONIC_SPECIFICATION					\
	__einfo_error ( EINFO_ENOTSUP_MORONIC_SPECIFICATION )
#define EINFO_ENOTSUP_MORONIC_SPECIFICATION				\
	__einfo_uniqify ( EINFO_ENOTSUP, 0x01,				\
			  "EFI_USB_IO_PROTOCOL was designed by morons" )

/******************************************************************************
 *
 * Device model
 *
 ******************************************************************************
 */

/**
 * Determine endpoint interface number
 *
 * @v usbio		USB I/O device
 * @v ep		USB Endpoint
 * @ret interface	Interface number, or negative error
 */
static int usbio_interface ( struct usbio_device *usbio,
			     struct usb_endpoint *ep ) {
	EFI_HANDLE handle = usbio->handle;
	struct usb_device *usb = ep->usb;
	struct usb_configuration_descriptor *config;
	struct usb_interface_descriptor *interface;
	struct usb_endpoint_descriptor *endpoint;
	struct usb_function *func;
	unsigned int i;

	/* The control endpoint is not part of a described interface */
	if ( ep->address == USB_EP0_ADDRESS )
		return 0;

	/* Iterate over all interface descriptors looking for a match */
	config = usbio->config;
	for_each_config_descriptor ( interface, config ) {

		/* Skip non-interface descriptors */
		if ( interface->header.type != USB_INTERFACE_DESCRIPTOR )
			continue;

		/* Iterate over all endpoint descriptors looking for a match */
		for_each_interface_descriptor ( endpoint, config, interface ) {

			/* Skip non-endpoint descriptors */
			if ( endpoint->header.type != USB_ENDPOINT_DESCRIPTOR )
				continue;

			/* Check endpoint address */
			if ( endpoint->endpoint != ep->address )
				continue;

			/* Check interface belongs to this function */
			list_for_each_entry ( func, &usb->functions, list ) {

				/* Skip non-matching functions */
				if ( func->interface[0] != usbio->first )
					continue;

				/* Iterate over all interfaces for a match */
				for ( i = 0 ; i < func->desc.count ; i++ ) {
					if ( interface->interface ==
					     func->interface[i] )
						return interface->interface;
				}
			}
		}
	}

	DBGC ( usbio, "USBIO %s cannot find interface for %s",
	       efi_handle_name ( handle ), usb_endpoint_name ( ep ) );
	return -ENOENT;
}

/**
 * Open USB I/O interface
 *
 * @v usbio		USB I/O device
 * @v interface		Interface number
 * @ret rc		Return status code
 */
static int usbio_open ( struct usbio_device *usbio, unsigned int interface ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_HANDLE handle = usbio->handle;
	struct usbio_interface *intf = &usbio->interface[interface];
	EFI_DEVICE_PATH_PROTOCOL *path;
	EFI_DEVICE_PATH_PROTOCOL *end;
	USB_DEVICE_PATH *usbpath;
	union {
		void *interface;
		EFI_USB_IO_PROTOCOL *io;
	} u;
	EFI_STATUS efirc;
	int rc;

	/* Sanity check */
	assert ( interface < usbio->config->interfaces );

	/* If interface is already open, just increment the usage count */
	if ( intf->count ) {
		intf->count++;
		return 0;
	}

	/* Construct device path for this interface */
	path = usbio->path;
	usbpath = usbio->usbpath;
	usbpath->InterfaceNumber = interface;
	end = efi_devpath_end ( path );

	/* Locate handle for this endpoint's interface */
	if ( ( efirc = bs->LocateDevicePath ( &efi_usb_io_protocol_guid, &path,
					      &intf->handle ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( usbio, "USBIO %s could not locate ",
		       efi_handle_name ( handle ) );
		DBGC ( usbio, "%s: %s\n",
		       efi_devpath_text ( usbio->path ), strerror ( rc ) );
		return rc;
	}

	/* Check that expected path was located */
	if ( path != end ) {
		DBGC ( usbio, "USBIO %s located incomplete ",
		       efi_handle_name ( handle ) );
		DBGC ( usbio, "%s\n", efi_handle_name ( intf->handle ) );
		return -EXDEV;
	}

	/* Open USB I/O protocol on this handle */
	if ( ( efirc = bs->OpenProtocol ( intf->handle,
					  &efi_usb_io_protocol_guid,
					  &u.interface, efi_image_handle,
					  intf->handle,
					  ( EFI_OPEN_PROTOCOL_BY_DRIVER |
					    EFI_OPEN_PROTOCOL_EXCLUSIVE )))!=0){
		rc = -EEFI ( efirc );
		DBGC ( usbio, "USBIO %s cannot open ",
		       efi_handle_name ( handle ) );
		DBGC ( usbio, "%s: %s\n",
		       efi_handle_name ( intf->handle ), strerror ( rc ) );
		DBGC_EFI_OPENERS ( usbio, intf->handle,
				   &efi_usb_io_protocol_guid );
		return rc;
	}
	intf->io = u.io;

	/* Increment usage count */
	intf->count++;

	return 0;
}

/**
 * Close USB I/O interface
 *
 * @v usbio		USB I/O device
 * @v interface		Interface number
 */
static void usbio_close ( struct usbio_device *usbio, unsigned int interface ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct usbio_interface *intf = &usbio->interface[interface];

	/* Sanity checks */
	assert ( interface < usbio->config->interfaces );
	assert ( intf->count > 0 );

	/* Decrement usage count */
	intf->count--;

	/* Do nothing if interface is still in use */
	if ( intf->count )
		return;

	/* Close USB I/O protocol */
	bs->CloseProtocol ( intf->handle, &efi_usb_io_protocol_guid,
			    efi_image_handle, intf->handle );
}

/******************************************************************************
 *
 * Control endpoints
 *
 ******************************************************************************
 */

/**
 * Open control endpoint
 *
 * @v endpoint		Endpoint
 * @ret rc		Return status code
 */
static int usbio_control_open ( struct usbio_endpoint *endpoint __unused ) {

	/* Nothing to do */
	return 0;
}

/**
 * Close control endpoint
 *
 * @v endpoint		Endpoint
 */
static void usbio_control_close ( struct usbio_endpoint *endpoint __unused ) {

	/* Nothing to do */
}

/**
 * Poll control endpoint
 *
 * @v endpoint		Endpoint
 */
static void usbio_control_poll ( struct usbio_endpoint *endpoint ) {
	struct usbio_device *usbio = endpoint->usbio;
	struct usb_endpoint *ep = endpoint->ep;
	EFI_HANDLE handle = usbio->handle;
	EFI_USB_IO_PROTOCOL *io;
	union {
		struct usb_setup_packet setup;
		EFI_USB_DEVICE_REQUEST efi;
	} *msg;
	EFI_USB_DATA_DIRECTION direction;
	struct io_buffer *iobuf;
	unsigned int index;
	unsigned int flags;
	unsigned int recipient;
	unsigned int interface;
	uint16_t request;
	void *data;
	size_t len;
	UINT32 status;
	EFI_STATUS efirc;
	int rc;

	/* Do nothing if ring is empty */
	if ( endpoint->cons == endpoint->prod )
		return;

	/* Consume next transfer */
	index = ( endpoint->cons++ % USBIO_RING_COUNT );
	iobuf = endpoint->iobuf[index];
	flags = endpoint->flags[index];

	/* Sanity check */
	if ( ! ( flags & USBIO_MESSAGE ) ) {
		DBGC ( usbio, "USBIO %s %s non-message transfer\n",
		       efi_handle_name ( handle ), usb_endpoint_name ( ep ) );
		rc = -ENOTSUP;
		goto err_not_message;
	}

	/* Construct transfer */
	assert ( iob_len ( iobuf ) >= sizeof ( *msg ) );
	msg = iobuf->data;
	iob_pull ( iobuf, sizeof ( *msg ) );
	request = le16_to_cpu ( msg->setup.request );
	len = iob_len ( iobuf );
	if ( len ) {
		data = iobuf->data;
		direction = ( ( request & USB_DIR_IN ) ?
			      EfiUsbDataIn : EfiUsbDataOut );
	} else {
		data = NULL;
		direction = EfiUsbNoData;
	}

	/* Determine interface for this transfer */
	recipient = ( request & USB_RECIP_MASK );
	if ( recipient == USB_RECIP_INTERFACE ) {
		/* Recipient is an interface: use interface number directly */
		interface = le16_to_cpu ( msg->setup.index );
	} else {
		/* Route all other requests through the first interface */
		interface = 0;
	}

	/* Open interface */
	if ( ( rc = usbio_open ( usbio, interface ) ) != 0 )
		goto err_open;
	io = usbio->interface[interface].io;

	/* Due to the design of EFI_USB_IO_PROTOCOL, attempting to set
	 * the configuration to a non-default value is basically a
	 * self-destruct button.
	 */
	if ( ( request == USB_SET_CONFIGURATION ) &&
	     ( le16_to_cpu ( msg->setup.value ) != usbio->config->config ) ) {
		rc = -ENOTSUP_MORONIC_SPECIFICATION;
		DBGC ( usbio, "USBIO %s cannot change configuration: %s\n",
		       efi_handle_name ( handle ), strerror ( rc ) );
		goto err_moronic_specification;
	}

	/* Submit transfer */
	if ( ( efirc = io->UsbControlTransfer ( io, &msg->efi, direction, 0,
						data, len, &status ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( usbio, "USBIO %s %s could not submit control transfer ",
		       efi_handle_name ( handle ), usb_endpoint_name ( ep ) );
		DBGC ( usbio, "via %s: %s (status %04x)\n",
		       efi_handle_name ( usbio->interface[interface].handle ),
		       strerror ( rc ), status );
		goto err_transfer;
	}

	/* Close interface */
	usbio_close ( usbio, interface );

	/* Complete transfer */
	usb_complete ( ep, iobuf );

	return;

 err_transfer:
 err_moronic_specification:
	usbio_close ( usbio, interface );
 err_open:
 err_not_message:
	usb_complete_err ( ep, iobuf, rc );
}

/** Control endpoint operations */
static struct usbio_operations usbio_control_operations = {
	.open	= usbio_control_open,
	.close	= usbio_control_close,
	.poll	= usbio_control_poll,
};

/******************************************************************************
 *
 * Bulk IN endpoints
 *
 ******************************************************************************
 */

/**
 * Open bulk IN endpoint
 *
 * @v endpoint		Endpoint
 * @ret rc		Return status code
 */
static int usbio_bulk_in_open ( struct usbio_endpoint *endpoint __unused ) {

	/* Nothing to do */
	return 0;
}

/**
 * Close bulk IN endpoint
 *
 * @v endpoint		Endpoint
 */
static void usbio_bulk_in_close ( struct usbio_endpoint *endpoint __unused ) {

	/* Nothing to do */
}

/**
 * Poll bulk IN endpoint
 *
 * @v endpoint		Endpoint
 */
static void usbio_bulk_in_poll ( struct usbio_endpoint *endpoint ) {
	struct usbio_device *usbio = endpoint->usbio;
	struct usb_endpoint *ep = endpoint->ep;
	EFI_USB_IO_PROTOCOL *io = endpoint->io;
	EFI_HANDLE handle = usbio->handle;
	struct io_buffer *iobuf;
	unsigned int index;
	UINTN len;
	UINT32 status;
	EFI_STATUS efirc;
	int rc;

	/* Do nothing if ring is empty */
	if ( endpoint->cons == endpoint->prod )
		return;

	/* Attempt (but do not yet consume) next transfer */
	index = ( endpoint->cons % USBIO_RING_COUNT );
	iobuf = endpoint->iobuf[index];

	/* Construct transfer */
	len = iob_len ( iobuf );

	/* Upon being turned on, the EFI_USB_IO_PROTOCOL did nothing
	 * for several minutes before firing a small ARP packet a few
	 * millimetres into the ether.
	 */
	efirc = io->UsbBulkTransfer ( io, ep->address, iobuf->data,
				      &len, 1, &status );
	if ( efirc == EFI_TIMEOUT )
		return;

	/* Consume transfer */
	endpoint->cons++;

	/* Check for failure */
	if ( efirc != 0 ) {
		rc = -EEFI ( efirc );
		DBGC2 ( usbio, "USBIO %s %s could not submit bulk IN transfer: "
			"%s (status %04x)\n", efi_handle_name ( handle ),
			usb_endpoint_name ( ep ), strerror ( rc ), status );
		usb_complete_err ( ep, iobuf, rc );
		return;
	}

	/* Update length */
	iob_put ( iobuf, ( len - iob_len ( iobuf ) ) );

	/* Complete transfer */
	usb_complete ( ep, iobuf );
}

/** Bulk endpoint operations */
static struct usbio_operations usbio_bulk_in_operations = {
	.open	= usbio_bulk_in_open,
	.close	= usbio_bulk_in_close,
	.poll	= usbio_bulk_in_poll,
};

/******************************************************************************
 *
 * Bulk OUT endpoints
 *
 ******************************************************************************
 */

/**
 * Open bulk OUT endpoint
 *
 * @v endpoint		Endpoint
 * @ret rc		Return status code
 */
static int usbio_bulk_out_open ( struct usbio_endpoint *endpoint __unused ) {

	/* Nothing to do */
	return 0;
}

/**
 * Close bulk OUT endpoint
 *
 * @v endpoint		Endpoint
 */
static void usbio_bulk_out_close ( struct usbio_endpoint *endpoint __unused ) {

	/* Nothing to do */
}

/**
 * Poll bulk OUT endpoint
 *
 * @v endpoint		Endpoint
 */
static void usbio_bulk_out_poll ( struct usbio_endpoint *endpoint ) {
	struct usbio_device *usbio = endpoint->usbio;
	struct usb_endpoint *ep = endpoint->ep;
	EFI_USB_IO_PROTOCOL *io = endpoint->io;
	EFI_HANDLE handle = usbio->handle;
	struct io_buffer *iobuf;
	unsigned int index;
	unsigned int flags;
	UINTN len;
	UINT32 status;
	EFI_STATUS efirc;
	int rc;

	/* Do nothing if ring is empty */
	if ( endpoint->cons == endpoint->prod )
		return;

	/* Consume next transfer */
	index = ( endpoint->cons++ % USBIO_RING_COUNT );
	iobuf = endpoint->iobuf[index];
	flags = endpoint->flags[index];

	/* Construct transfer */
	len = iob_len ( iobuf );

	/* Submit transfer */
	if ( ( efirc = io->UsbBulkTransfer ( io, ep->address, iobuf->data,
					     &len, 0, &status ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( usbio, "USBIO %s %s could not submit bulk OUT transfer: "
		       "%s (status %04x)\n", efi_handle_name ( handle ),
		       usb_endpoint_name ( ep ), strerror ( rc ), status );
		goto err;
	}

	/* Update length */
	iob_put ( iobuf, ( len - iob_len ( iobuf ) ) );

	/* Submit zero-length transfer if required */
	len = 0;
	if ( ( flags & USBIO_ZLEN ) &&
	     ( efirc = io->UsbBulkTransfer ( io, ep->address, NULL, &len, 0,
					     &status ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( usbio, "USBIO %s %s could not submit zero-length "
		       "transfer: %s (status %04x)\n",
		       efi_handle_name ( handle ), usb_endpoint_name ( ep ),
		       strerror ( rc ), status );
		goto err;
	}

	/* Complete transfer */
	usb_complete ( ep, iobuf );

	return;

 err:
	usb_complete_err ( ep, iobuf, rc );
}

/** Bulk endpoint operations */
static struct usbio_operations usbio_bulk_out_operations = {
	.open	= usbio_bulk_out_open,
	.close	= usbio_bulk_out_close,
	.poll	= usbio_bulk_out_poll,
};

/******************************************************************************
 *
 * Interrupt endpoints
 *
 ******************************************************************************
 *
 * The EFI_USB_IO_PROTOCOL provides two ways to interact with
 * interrupt endpoints, neither of which naturally model the hardware
 * interaction.  The UsbSyncInterruptTransfer() method allows imposes
 * a 1ms overhead for every interrupt transfer (which could result in
 * up to a 50% decrease in overall throughput for the device).  The
 * UsbAsyncInterruptTransfer() method provides no way for us to
 * prevent transfers when no I/O buffers are available.
 *
 * We work around this design by utilising a small, fixed ring buffer
 * into which the interrupt callback delivers the data.  This aims to
 * provide buffer space even if no I/O buffers have yet been enqueued.
 * The scheme is not guaranteed since the fixed ring buffer may also
 * become full.  However:
 *
 *   - devices which send a constant stream of interrupts (and which
 *     therefore might exhaust the fixed ring buffer) tend to be
 *     responding to every interrupt request, and can tolerate lost
 *     packets, and
 *
 *   - devices which cannot tolerate lost interrupt packets tend to send
 *     only a few small messages.
 *
 * The scheme should therefore work in practice.
 */

/**
 * Interrupt endpoint callback
 *
 * @v data		Received data
 * @v len		Length of received data
 * @v context		Callback context
 * @v status		Transfer status
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI usbio_interrupt_callback ( VOID *data, UINTN len,
						    VOID *context,
						    UINT32 status ) {
	struct usbio_interrupt_ring *intr = context;
	struct usbio_endpoint *endpoint = intr->endpoint;
	struct usbio_device *usbio = endpoint->usbio;
	struct usb_endpoint *ep = endpoint->ep;
	EFI_HANDLE handle = usbio->handle;
	unsigned int fill;
	unsigned int index;

	/* Sanity check */
	assert ( len <= ep->mtu );

	/* Do nothing if ring is empty */
	fill = ( intr->prod - intr->cons );
	if ( fill >= USBIO_INTR_COUNT ) {
		DBGC ( usbio, "USBIO %s %s dropped interrupt completion\n",
		       efi_handle_name ( handle ), usb_endpoint_name ( ep ) );
		return 0;
	}

	/* Do nothing if transfer was unsuccessful */
	if ( status != 0 ) {
		DBGC ( usbio, "USBIO %s %s interrupt completion status %04x\n",
		       efi_handle_name ( handle ), usb_endpoint_name ( ep ),
		       status );
		return 0; /* Unclear what failure actually means here */
	}

	/* Copy data to buffer and increment producer counter */
	index = ( intr->prod % USBIO_INTR_COUNT );
	memcpy ( intr->data[index], data, len );
	intr->len[index] = len;
	intr->prod++;

	return 0;
}

/**
 * Open interrupt endpoint
 *
 * @v endpoint		Endpoint
 * @ret rc		Return status code
 */
static int usbio_interrupt_open ( struct usbio_endpoint *endpoint ) {
	struct usbio_device *usbio = endpoint->usbio;
	struct usbio_interrupt_ring *intr;
	struct usb_endpoint *ep = endpoint->ep;
	EFI_USB_IO_PROTOCOL *io = endpoint->io;
	EFI_HANDLE handle = usbio->handle;
	unsigned int interval;
	unsigned int i;
	void *data;
	EFI_STATUS efirc;
	int rc;

	/* Allocate interrupt ring buffer */
	intr = zalloc ( sizeof ( *intr ) + ( USBIO_INTR_COUNT * ep->mtu ) );
	if ( ! intr ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	endpoint->intr = intr;
	intr->endpoint = endpoint;
	data = ( ( ( void * ) intr ) + sizeof ( *intr ) );
	for ( i = 0 ; i < USBIO_INTR_COUNT ; i++ ) {
		intr->data[i] = data;
		data += ep->mtu;
	}

	/* Determine polling interval */
	interval = ( ep->interval >> 3 /* microframes -> milliseconds */ );
	if ( ! interval )
		interval = 1; /* May not be zero */

	/* Add to periodic schedule */
	if ( ( efirc = io->UsbAsyncInterruptTransfer ( io, ep->address, TRUE,
						       interval, ep->mtu,
						       usbio_interrupt_callback,
						       intr ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( usbio, "USBIO %s %s could not schedule interrupt "
		       "transfer: %s\n", efi_handle_name ( handle ),
		       usb_endpoint_name ( ep ), strerror ( rc ) );
		goto err_schedule;
	}

	return 0;

	io->UsbAsyncInterruptTransfer ( io, ep->address, FALSE, 0, 0,
					NULL, NULL );
 err_schedule:
	free ( intr );
 err_alloc:
	return rc;
}

/**
 * Close interrupt endpoint
 *
 * @v endpoint		Endpoint
 */
static void usbio_interrupt_close ( struct usbio_endpoint *endpoint ) {
	struct usb_endpoint *ep = endpoint->ep;
	EFI_USB_IO_PROTOCOL *io = endpoint->io;

	/* Remove from periodic schedule */
	io->UsbAsyncInterruptTransfer ( io, ep->address, FALSE, 0, 0,
					NULL, NULL );

	/* Free interrupt ring buffer */
	free ( endpoint->intr );
}

/**
 * Poll interrupt endpoint
 *
 * @v endpoint		Endpoint
 */
static void usbio_interrupt_poll ( struct usbio_endpoint *endpoint ) {
	struct usbio_interrupt_ring *intr = endpoint->intr;
	struct usb_endpoint *ep = endpoint->ep;
	struct io_buffer *iobuf;
	unsigned int index;
	unsigned int intr_index;
	size_t len;

	/* Do nothing if ring is empty */
	if ( endpoint->cons == endpoint->prod )
		return;

	/* Do nothing if interrupt ring is empty */
	if ( intr->cons == intr->prod )
		return;

	/* Consume next transfer */
	index = ( endpoint->cons++ % USBIO_RING_COUNT );
	iobuf = endpoint->iobuf[index];

	/* Populate I/O buffer */
	intr_index = ( intr->cons++ % USBIO_INTR_COUNT );
	len = intr->len[intr_index];
	assert ( len <= iob_len ( iobuf ) );
	iob_put ( iobuf, ( len - iob_len ( iobuf ) ) );
	memcpy ( iobuf->data, intr->data[intr_index], len );

	/* Complete transfer */
	usb_complete ( ep, iobuf );
}

/** Interrupt endpoint operations */
static struct usbio_operations usbio_interrupt_operations = {
	.open	= usbio_interrupt_open,
	.close	= usbio_interrupt_close,
	.poll	= usbio_interrupt_poll,
};

/******************************************************************************
 *
 * Endpoint operations
 *
 ******************************************************************************
 */

/**
 * Open endpoint
 *
 * @v ep		USB endpoint
 * @ret rc		Return status code
 */
static int usbio_endpoint_open ( struct usb_endpoint *ep ) {
	struct usb_bus *bus = ep->usb->port->hub->bus;
	struct usbio_device *usbio = usb_bus_get_hostdata ( bus );
	struct usbio_endpoint *endpoint;
	EFI_HANDLE handle = usbio->handle;
	unsigned int attr = ( ep->attributes & USB_ENDPOINT_ATTR_TYPE_MASK );
	int interface;
	int rc;

	/* Allocate and initialise structure */
	endpoint = zalloc ( sizeof ( *endpoint ) );
	if ( ! endpoint ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	usb_endpoint_set_hostdata ( ep, endpoint );
	endpoint->usbio = usbio;
	endpoint->ep = ep;

	/* Identify endpoint operations */
	if ( attr == USB_ENDPOINT_ATTR_CONTROL ) {
		endpoint->op = &usbio_control_operations;
	} else if ( attr == USB_ENDPOINT_ATTR_BULK ) {
		endpoint->op = ( ( ep->address & USB_DIR_IN ) ?
				 &usbio_bulk_in_operations :
				 &usbio_bulk_out_operations );
	} else if ( attr == USB_ENDPOINT_ATTR_INTERRUPT ) {
		endpoint->op = &usbio_interrupt_operations;
	} else {
		rc = -ENOTSUP;
		goto err_operations;
	}

	/* Identify interface for this endpoint */
	interface = usbio_interface ( usbio, ep );
	if ( interface < 0 ) {
		rc = interface;
		goto err_interface;
	}
	endpoint->interface = interface;

	/* Open interface */
	if ( ( rc = usbio_open ( usbio, interface ) ) != 0 )
		goto err_open_interface;
	endpoint->handle = usbio->interface[interface].handle;
	endpoint->io = usbio->interface[interface].io;
	DBGC ( usbio, "USBIO %s %s using ",
	       efi_handle_name ( handle ), usb_endpoint_name ( ep ) );
	DBGC ( usbio, "%s\n", efi_handle_name ( endpoint->handle ) );

	/* Open endpoint */
	if ( ( rc = endpoint->op->open ( endpoint ) ) != 0 )
		goto err_open_endpoint;

	/* Add to list of endpoints */
	list_add_tail ( &endpoint->list, &usbio->endpoints );

	return 0;

	list_del ( &endpoint->list );
	endpoint->op->close ( endpoint );
 err_open_endpoint:
	usbio_close ( usbio, interface );
 err_open_interface:
 err_interface:
 err_operations:
	free ( endpoint );
 err_alloc:
	return rc;
}

/**
 * Close endpoint
 *
 * @v ep		USB endpoint
 */
static void usbio_endpoint_close ( struct usb_endpoint *ep ) {
	struct usbio_endpoint *endpoint = usb_endpoint_get_hostdata ( ep );
	struct usbio_device *usbio = endpoint->usbio;
	struct io_buffer *iobuf;
	unsigned int index;

	/* Remove from list of endpoints */
	list_del ( &endpoint->list );

	/* Close endpoint */
	endpoint->op->close ( endpoint );

	/* Close interface */
	usbio_close ( usbio, endpoint->interface );

	/* Cancel any incomplete transfers */
	while ( endpoint->cons != endpoint->prod ) {
		index = ( endpoint->cons++ % USBIO_RING_COUNT );
		iobuf = endpoint->iobuf[index];
		usb_complete_err ( ep, iobuf, -ECANCELED );
	}

	/* Free endpoint */
	free ( endpoint );
}

/**
 * Reset endpoint
 *
 * @v ep		USB endpoint
 * @ret rc		Return status code
 */
static int usbio_endpoint_reset ( struct usb_endpoint *ep __unused ) {

	/* Nothing to do */
	return 0;
}

/**
 * Update MTU
 *
 * @v ep		USB endpoint
 * @ret rc		Return status code
 */
static int usbio_endpoint_mtu ( struct usb_endpoint *ep __unused ) {

	/* Nothing to do */
	return 0;
}

/**
 * Enqueue transfer
 *
 * @v ep		USB endpoint
 * @v iobuf		I/O buffer
 * @v flags		Transfer flags
 * @ret rc		Return status code
 */
static int usbio_endpoint_enqueue ( struct usb_endpoint *ep,
				    struct io_buffer *iobuf,
				    unsigned int flags ) {
	struct usbio_endpoint *endpoint = usb_endpoint_get_hostdata ( ep );
	unsigned int fill;
	unsigned int index;

	/* Fail if transfer ring is full */
	fill = ( endpoint->prod - endpoint->cons );
	if ( fill >= USBIO_RING_COUNT )
		return -ENOBUFS;

	/* Add to ring */
	index = ( endpoint->prod++ % USBIO_RING_COUNT );
	endpoint->iobuf[index] = iobuf;
	endpoint->flags[index] = flags;

	return 0;
}

/**
 * Enqueue message transfer
 *
 * @v ep		USB endpoint
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int usbio_endpoint_message ( struct usb_endpoint *ep,
				    struct io_buffer *iobuf ) {

	/* Enqueue transfer */
	return usbio_endpoint_enqueue ( ep, iobuf, USBIO_MESSAGE );
}

/**
 * Enqueue stream transfer
 *
 * @v ep		USB endpoint
 * @v iobuf		I/O buffer
 * @v zlp		Append a zero-length packet
 * @ret rc		Return status code
 */
static int usbio_endpoint_stream ( struct usb_endpoint *ep,
				   struct io_buffer *iobuf, int zlp ) {

	/* Enqueue transfer */
	return usbio_endpoint_enqueue ( ep, iobuf, ( zlp ? USBIO_ZLEN : 0 ) );
}

/**
 * Poll for completions
 *
 * @v endpoint		Endpoint
 */
static void usbio_endpoint_poll ( struct usbio_endpoint *endpoint ) {

	/* Poll endpoint */
	endpoint->op->poll ( endpoint );
}

/******************************************************************************
 *
 * Device operations
 *
 ******************************************************************************
 */

/**
 * Open device
 *
 * @v usb		USB device
 * @ret rc		Return status code
 */
static int usbio_device_open ( struct usb_device *usb ) {
	struct usbio_device *usbio =
		usb_bus_get_hostdata ( usb->port->hub->bus );

	usb_set_hostdata ( usb, usbio );
	return 0;
}

/**
 * Close device
 *
 * @v usb		USB device
 */
static void usbio_device_close ( struct usb_device *usb __unused ) {

	/* Nothing to do */
}

/**
 * Assign device address
 *
 * @v usb		USB device
 * @ret rc		Return status code
 */
static int usbio_device_address ( struct usb_device *usb __unused ) {

	/* Nothing to do */
	return 0;
}

/******************************************************************************
 *
 * Hub operations
 *
 ******************************************************************************
 */

/**
 * Open hub
 *
 * @v hub		USB hub
 * @ret rc		Return status code
 */
static int usbio_hub_open ( struct usb_hub *hub ) {

	/* Disallow non-root hubs */
	if ( hub->usb )
		return -ENOTSUP;

	/* Nothing to do */
	return 0;
}

/**
 * Close hub
 *
 * @v hub		USB hub
 */
static void usbio_hub_close ( struct usb_hub *hub __unused ) {

	/* Nothing to do */
}

/******************************************************************************
 *
 * Root hub operations
 *
 ******************************************************************************
 */

/**
 * Open root hub
 *
 * @v hub		USB hub
 * @ret rc		Return status code
 */
static int usbio_root_open ( struct usb_hub *hub __unused ) {

	/* Nothing to do */
	return 0;
}

/**
 * Close root hub
 *
 * @v hub		USB hub
 */
static void usbio_root_close ( struct usb_hub *hub __unused ) {

	/* Nothing to do */
}

/**
 * Enable port
 *
 * @v hub		USB hub
 * @v port		USB port
 * @ret rc		Return status code
 */
static int usbio_root_enable ( struct usb_hub *hub __unused,
			       struct usb_port *port __unused ) {

	/* Nothing to do */
	return 0;
}

/**
 * Disable port
 *
 * @v hub		USB hub
 * @v port		USB port
 * @ret rc		Return status code
 */
static int usbio_root_disable ( struct usb_hub *hub __unused,
				struct usb_port *port __unused ) {

	/* Nothing to do */
	return 0;
}

/**
 * Update root hub port speed
 *
 * @v hub		USB hub
 * @v port		USB port
 * @ret rc		Return status code
 */
static int usbio_root_speed ( struct usb_hub *hub __unused,
			      struct usb_port *port ) {

	/* Not actually exposed via EFI_USB_IO_PROTOCOL */
	port->speed = USB_SPEED_HIGH;
	return 0;
}

/**
 * Clear transaction translator buffer
 *
 * @v hub		USB hub
 * @v port		USB port
 * @v ep		USB endpoint
 * @ret rc		Return status code
 */
static int usbio_root_clear_tt ( struct usb_hub *hub __unused,
				 struct usb_port *port __unused,
				 struct usb_endpoint *ep __unused ) {

	/* Should never be called; this is a root hub */
	return -ENOTSUP;
}

/******************************************************************************
 *
 * Bus operations
 *
 ******************************************************************************
 */

/**
 * Open USB bus
 *
 * @v bus		USB bus
 * @ret rc		Return status code
 */
static int usbio_bus_open ( struct usb_bus *bus __unused ) {

	/* Nothing to do */
	return 0;
}

/**
 * Close USB bus
 *
 * @v bus		USB bus
 */
static void usbio_bus_close ( struct usb_bus *bus __unused ) {

	/* Nothing to do */
}

/**
 * Poll USB bus
 *
 * @v bus		USB bus
 */
static void usbio_bus_poll ( struct usb_bus *bus ) {
	struct usbio_device *usbio = usb_bus_get_hostdata ( bus );
	struct usbio_endpoint *endpoint;

	/* Poll all endpoints.  We trust that completion handlers are
	 * minimal and will not do anything that could plausibly
	 * affect the endpoint list itself.
	 */
	list_for_each_entry ( endpoint, &usbio->endpoints, list )
		usbio_endpoint_poll ( endpoint );
}

/******************************************************************************
 *
 * EFI driver interface
 *
 ******************************************************************************
 */

/** USB I/O host controller driver operations */
static struct usb_host_operations usbio_operations = {
	.endpoint = {
		.open = usbio_endpoint_open,
		.close = usbio_endpoint_close,
		.reset = usbio_endpoint_reset,
		.mtu = usbio_endpoint_mtu,
		.message = usbio_endpoint_message,
		.stream = usbio_endpoint_stream,
	},
	.device = {
		.open = usbio_device_open,
		.close = usbio_device_close,
		.address = usbio_device_address,
	},
	.bus = {
		.open = usbio_bus_open,
		.close = usbio_bus_close,
		.poll = usbio_bus_poll,
	},
	.hub = {
		.open = usbio_hub_open,
		.close = usbio_hub_close,
	},
	.root = {
		.open = usbio_root_open,
		.close = usbio_root_close,
		.enable = usbio_root_enable,
		.disable = usbio_root_disable,
		.speed = usbio_root_speed,
		.clear_tt = usbio_root_clear_tt,
	},
};

/**
 * Check to see if driver supports a device
 *
 * @v handle		EFI device handle
 * @ret rc		Return status code
 */
static int usbio_supported ( EFI_HANDLE handle ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_USB_DEVICE_DESCRIPTOR device;
	EFI_USB_INTERFACE_DESCRIPTOR interface;
	struct usb_function_descriptor desc;
	struct usb_driver *driver;
	struct usb_device_id *id;
	union {
		void *interface;
		EFI_USB_IO_PROTOCOL *io;
	} usb;
	EFI_STATUS efirc;
	int rc;

	/* Get protocol */
	if ( ( efirc = bs->OpenProtocol ( handle, &efi_usb_io_protocol_guid,
					  &usb.interface, efi_image_handle,
					  handle,
					  EFI_OPEN_PROTOCOL_GET_PROTOCOL ))!=0){
		rc = -EEFI ( efirc );
		DBGCP ( handle, "USB %s is not a USB device\n",
			efi_handle_name ( handle ) );
		goto err_open_protocol;
	}

	/* Get device descriptor */
	if ( ( efirc = usb.io->UsbGetDeviceDescriptor ( usb.io,
							&device ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( handle, "USB %s could not get device descriptor: "
		       "%s\n", efi_handle_name ( handle ), strerror ( rc ) );
		goto err_get_device_descriptor;
	}
	memset ( &desc, 0, sizeof ( desc ) );
	desc.vendor = device.IdVendor;
	desc.product = device.IdProduct;

	/* Get interface descriptor */
	if ( ( efirc = usb.io->UsbGetInterfaceDescriptor ( usb.io,
							   &interface ) ) !=0){
		rc = -EEFI ( efirc );
		DBGC ( handle, "USB %s could not get interface descriptor: "
		       "%s\n", efi_handle_name ( handle ), strerror ( rc ) );
		goto err_get_interface_descriptor;
	}
	desc.class.class.class = interface.InterfaceClass;
	desc.class.class.subclass = interface.InterfaceSubClass;
	desc.class.class.protocol = interface.InterfaceProtocol;

	/* Look for a driver for this interface */
	driver = usb_find_driver ( &desc, &id );
	if ( ! driver ) {
		rc = -ENOTSUP;
		goto err_unsupported;
	}

	/* Success */
	rc = 0;

 err_unsupported:
 err_get_interface_descriptor:
 err_get_device_descriptor:
	bs->CloseProtocol ( handle, &efi_usb_io_protocol_guid,
			    efi_image_handle, handle );
 err_open_protocol:
	return rc;
}

/**
 * Fetch configuration descriptor
 *
 * @v usbio		USB I/O device
 * @ret rc		Return status code
 */
static int usbio_config ( struct usbio_device *usbio ) {
	EFI_HANDLE handle = usbio->handle;
	EFI_USB_IO_PROTOCOL *io = usbio->io;
	EFI_USB_DEVICE_DESCRIPTOR device;
	EFI_USB_CONFIG_DESCRIPTOR partial;
	union {
		struct usb_setup_packet setup;
		EFI_USB_DEVICE_REQUEST efi;
	} msg;
	UINT32 status;
	size_t len;
	unsigned int count;
	unsigned int value;
	unsigned int i;
	EFI_STATUS efirc;
	int rc;

	/* Get device descriptor */
	if ( ( efirc = io->UsbGetDeviceDescriptor ( io, &device ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( usbio, "USB %s could not get device descriptor: "
		       "%s\n", efi_handle_name ( handle ), strerror ( rc ) );
		goto err_get_device_descriptor;
	}
	count = device.NumConfigurations;

	/* Get current partial configuration descriptor */
	if ( ( efirc = io->UsbGetConfigDescriptor ( io, &partial ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( usbio, "USB %s could not get partial configuration "
		       "descriptor: %s\n", efi_handle_name ( handle ),
		       strerror ( rc ) );
		goto err_get_configuration_descriptor;
	}
	len = le16_to_cpu ( partial.TotalLength );

	/* Allocate configuration descriptor */
	usbio->config = malloc ( len );
	if ( ! usbio->config ) {
		rc = -ENOMEM;
		goto err_alloc;
	}

	/* There is, naturally, no way to retrieve the entire device
	 * configuration descriptor via EFI_USB_IO_PROTOCOL.  Worse,
	 * there is no way to even retrieve the index of the current
	 * configuration descriptor.  We have to iterate over all
	 * possible configuration descriptors looking for the
	 * descriptor that matches the current configuration value.
	 */
	for ( i = 0 ; i < count ; i++ ) {

		/* Construct request */
		msg.setup.request = cpu_to_le16 ( USB_GET_DESCRIPTOR );
		value = ( ( USB_CONFIGURATION_DESCRIPTOR << 8 ) | i );
		msg.setup.value = cpu_to_le16 ( value );
		msg.setup.index = 0;
		msg.setup.len = cpu_to_le16 ( len );

		/* Get full configuration descriptor */
		if ( ( efirc = io->UsbControlTransfer ( io, &msg.efi,
							EfiUsbDataIn, 0,
							usbio->config, len,
							&status ) ) != 0 ) {
			rc = -EEFI ( efirc );
			DBGC ( usbio, "USB %s could not get configuration %d "
			       "descriptor: %s\n", efi_handle_name ( handle ),
			       i, strerror ( rc ) );
			goto err_control_transfer;
		}

		/* Ignore unless this is the current configuration */
		if ( usbio->config->config != partial.ConfigurationValue )
			continue;

		/* Check length */
		if ( le16_to_cpu ( usbio->config->len ) != len ) {
			DBGC ( usbio, "USB %s configuration descriptor length "
			       "mismatch\n", efi_handle_name ( handle ) );
			rc = -EINVAL;
			goto err_len;
		}

		return 0;
	}

	/* No match found */
	DBGC ( usbio, "USB %s could not find current configuration "
	       "descriptor\n", efi_handle_name ( handle ) );
	rc = -ENOENT;

 err_len:
 err_control_transfer:
	free ( usbio->config );
 err_alloc:
 err_get_configuration_descriptor:
 err_get_device_descriptor:
	return rc;
}

/**
 * Construct device path for opening other interfaces
 *
 * @v usbio		USB I/O device
 * @ret rc		Return status code
 */
static int usbio_path ( struct usbio_device *usbio ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_HANDLE handle = usbio->handle;
	EFI_DEVICE_PATH_PROTOCOL *path;
	EFI_DEVICE_PATH_PROTOCOL *end;
	USB_DEVICE_PATH *usbpath;
	union {
		void *interface;
		EFI_DEVICE_PATH_PROTOCOL *path;
	} u;
	size_t len;
	EFI_STATUS efirc;
	int rc;

	/* Open device path protocol */
	if ( ( efirc = bs->OpenProtocol ( handle,
					  &efi_device_path_protocol_guid,
					  &u.interface, efi_image_handle,
					  handle,
					  EFI_OPEN_PROTOCOL_GET_PROTOCOL ))!=0){
		rc = -EEFI ( efirc );
		DBGC ( usbio, "USBIO %s cannot open device path protocol: "
		       "%s\n", efi_handle_name ( handle ), strerror ( rc ) );
		goto err_open_protocol;
	}
	path = u.interface;

	/* Locate end of device path and sanity check */
	len = efi_devpath_len ( path );
	if ( len < sizeof ( *usbpath ) ) {
		DBGC ( usbio, "USBIO %s underlength device path\n",
		       efi_handle_name ( handle ) );
		rc = -EINVAL;
		goto err_underlength;
	}
	usbpath = ( ( ( void * ) path ) + len - sizeof ( *usbpath ) );
	if ( ! ( ( usbpath->Header.Type == MESSAGING_DEVICE_PATH ) &&
		 ( usbpath->Header.SubType == MSG_USB_DP ) ) ) {
		DBGC ( usbio, "USBIO %s not a USB device path: ",
		       efi_handle_name ( handle ) );
		DBGC ( usbio, "%s\n", efi_devpath_text ( path ) );
		rc = -EINVAL;
		goto err_non_usb;
	}

	/* Allocate copy of device path */
	usbio->path = malloc ( len + sizeof ( *end ) );
	if ( ! usbio->path ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	memcpy ( usbio->path, path, ( len + sizeof ( *end ) ) );
	usbio->usbpath = ( ( ( void * ) usbio->path ) + len -
			   sizeof ( *usbpath ) );

	/* Close protocol */
	bs->CloseProtocol ( handle, &efi_device_path_protocol_guid,
			    efi_image_handle, handle );

	return 0;

	free ( usbio->path );
 err_alloc:
 err_non_usb:
 err_underlength:
	bs->CloseProtocol ( handle, &efi_device_path_protocol_guid,
			    efi_image_handle, handle );
 err_open_protocol:
	return rc;
}

/**
 * Construct interface list
 *
 * @v usbio		USB I/O device
 * @ret rc		Return status code
 */
static int usbio_interfaces ( struct usbio_device *usbio ) {
	EFI_HANDLE handle = usbio->handle;
	EFI_USB_IO_PROTOCOL *io = usbio->io;
	EFI_USB_INTERFACE_DESCRIPTOR interface;
	unsigned int first;
	unsigned int count;
	EFI_STATUS efirc;
	int rc;

	/* Get interface descriptor */
	if ( ( efirc = io->UsbGetInterfaceDescriptor ( io, &interface ) ) != 0){
		rc = -EEFI ( efirc );
		DBGC ( usbio, "USB %s could not get interface descriptor: "
		       "%s\n", efi_handle_name ( handle ), strerror ( rc ) );
		goto err_get_interface_descriptor;
	}

	/* Record first interface number */
	first = interface.InterfaceNumber;
	count = usbio->config->interfaces;
	assert ( first < count );
	usbio->first = first;

	/* Allocate interface list */
	usbio->interface = zalloc ( count * sizeof ( usbio->interface[0] ) );
	if ( ! usbio->interface ) {
		rc = -ENOMEM;
		goto err_alloc;
	}

	/* Use already-opened protocol for control transfers and for
	 * the first interface.
	 */
	usbio->interface[0].handle = handle;
	usbio->interface[0].io = io;
	usbio->interface[0].count = 1;
	usbio->interface[first].handle = handle;
	usbio->interface[first].io = io;
	usbio->interface[first].count = 1;

	return 0;

	free ( usbio->interface );
 err_alloc:
 err_get_interface_descriptor:
	return rc;
}

/**
 * Attach driver to device
 *
 * @v efidev		EFI device
 * @ret rc		Return status code
 */
static int usbio_start ( struct efi_device *efidev ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_HANDLE handle = efidev->device;
	struct usbio_device *usbio;
	struct usb_port *port;
	union {
		void *interface;
		EFI_USB_IO_PROTOCOL *io;
	} u;
	EFI_STATUS efirc;
	int rc;

	/* Allocate and initialise structure */
	usbio = zalloc ( sizeof ( *usbio ) );
	if ( ! usbio ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	efidev_set_drvdata ( efidev, usbio );
	usbio->handle = handle;
	INIT_LIST_HEAD ( &usbio->endpoints );

	/* Open USB I/O protocol */
	if ( ( efirc = bs->OpenProtocol ( handle, &efi_usb_io_protocol_guid,
					  &u.interface, efi_image_handle,
					  handle,
					  ( EFI_OPEN_PROTOCOL_BY_DRIVER |
					    EFI_OPEN_PROTOCOL_EXCLUSIVE )))!=0){
		rc = -EEFI ( efirc );
		DBGC ( usbio, "USBIO %s cannot open USB I/O protocol: %s\n",
		       efi_handle_name ( handle ), strerror ( rc ) );
		DBGC_EFI_OPENERS ( usbio, handle, &efi_usb_io_protocol_guid );
		goto err_open_usbio;
	}
	usbio->io = u.io;

	/* Describe generic device */
	efi_device_info ( handle, "USB", &usbio->dev );
	usbio->dev.parent = &efidev->dev;
	list_add ( &usbio->dev.siblings, &efidev->dev.children );
	INIT_LIST_HEAD ( &usbio->dev.children );

	/* Fetch configuration descriptor */
	if ( ( rc = usbio_config ( usbio ) ) != 0 )
		goto err_config;

	/* Construct device path */
	if ( ( rc = usbio_path ( usbio ) ) != 0 )
		goto err_path;

	/* Construct interface list */
	if ( ( rc = usbio_interfaces ( usbio ) ) != 0 )
		goto err_interfaces;

	/* Allocate USB bus */
	usbio->bus = alloc_usb_bus ( &usbio->dev, 1 /* single "port" */,
				     USBIO_MTU, &usbio_operations );
	if ( ! usbio->bus ) {
		rc = -ENOMEM;
		goto err_alloc_bus;
	}
	usb_bus_set_hostdata ( usbio->bus, usbio );
	usb_hub_set_drvdata ( usbio->bus->hub, usbio );

	/* Set port protocol */
	port = usb_port ( usbio->bus->hub, 1 );
	port->protocol = USB_PROTO_2_0;

	/* Register USB bus */
	if ( ( rc = register_usb_bus ( usbio->bus ) ) != 0 )
		goto err_register;

	return 0;

	unregister_usb_bus ( usbio->bus );
 err_register:
	free_usb_bus ( usbio->bus );
 err_alloc_bus:
	free ( usbio->interface );
 err_interfaces:
	free ( usbio->path );
 err_path:
	free ( usbio->config );
 err_config:
	list_del ( &usbio->dev.siblings );
	bs->CloseProtocol ( handle, &efi_usb_io_protocol_guid,
			    efi_image_handle, handle );
 err_open_usbio:
	free ( usbio );
 err_alloc:
	return rc;
}

/**
 * Detach driver from device
 *
 * @v efidev		EFI device
 */
static void usbio_stop ( struct efi_device *efidev ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	EFI_HANDLE handle = efidev->device;
	struct usbio_device *usbio = efidev_get_drvdata ( efidev );

	unregister_usb_bus ( usbio->bus );
	free_usb_bus ( usbio->bus );
	free ( usbio->interface );
	free ( usbio->path );
	free ( usbio->config );
	list_del ( &usbio->dev.siblings );
	bs->CloseProtocol ( handle, &efi_usb_io_protocol_guid,
			    efi_image_handle, handle );
	free ( usbio );
}

/** EFI USB I/O driver */
struct efi_driver usbio_driver __efi_driver ( EFI_DRIVER_NORMAL ) = {
	.name = "USBIO",
	.supported = usbio_supported,
	.start = usbio_start,
	.stop = usbio_stop,
};

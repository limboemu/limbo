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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <ipxe/efi/efi.h>
#include <ipxe/efi/efi_utils.h>
#include <ipxe/efi/efi_driver.h>
#include <ipxe/efi/efi_usb.h>
#include <ipxe/usb.h>

/** @file
 *
 * EFI USB I/O PROTOCOL
 *
 */

/**
 * Transcribe data direction (for debugging)
 *
 * @v direction		Data direction
 * @ret text		Transcribed data direction
 */
static const char * efi_usb_direction_name ( EFI_USB_DATA_DIRECTION direction ){

	switch ( direction ) {
	case EfiUsbDataIn:	return "in";
	case EfiUsbDataOut:	return "out";
	case EfiUsbNoData:	return "none";
	default:		return "<UNKNOWN>";
	}
}

/******************************************************************************
 *
 * Endpoints
 *
 ******************************************************************************
 */

/**
 * Poll USB bus
 *
 * @v usbdev		EFI USB device
 */
static void efi_usb_poll ( struct efi_usb_device *usbdev ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct usb_bus *bus = usbdev->usb->port->hub->bus;
	EFI_TPL tpl;

	/* UEFI manages to ingeniously combine the worst aspects of
	 * both polling and interrupt-driven designs.  There is no way
	 * to support proper interrupt-driven operation, since there
	 * is no way to hook in an interrupt service routine.  A
	 * mockery of interrupts is provided by UEFI timers, which
	 * trigger at a preset rate and can fire at any time.
	 *
	 * We therefore have all of the downsides of a polling design
	 * (inefficiency and inability to sleep until something
	 * interesting happens) combined with all of the downsides of
	 * an interrupt-driven design (the complexity of code that
	 * could be preempted at any time).
	 *
	 * The UEFI specification expects us to litter the entire
	 * codebase with calls to RaiseTPL() as needed for sections of
	 * code that are not reentrant.  Since this doesn't actually
	 * gain us any substantive benefits (since even with such
	 * calls we would still be suffering from the limitations of a
	 * polling design), we instead choose to wrap only calls to
	 * usb_poll().  This should be sufficient for most practical
	 * purposes.
	 *
	 * A "proper" solution would involve rearchitecting the whole
	 * codebase to support interrupt-driven operation.
	 */
	tpl = bs->RaiseTPL ( TPL_NOTIFY );

	/* Poll bus */
	usb_poll ( bus );

	/* Restore task priority level */
	bs->RestoreTPL ( tpl );
}

/**
 * Poll USB bus (from endpoint event timer)
 *
 * @v event		EFI event
 * @v context		EFI USB endpoint
 */
static VOID EFIAPI efi_usb_timer ( EFI_EVENT event __unused,
				   VOID *context ) {
	struct efi_usb_endpoint *usbep = context;
	struct usb_bus *bus = usbep->usbintf->usbdev->usb->port->hub->bus;

	/* Poll bus */
	usb_poll ( bus );

	/* Refill endpoint */
	usb_refill ( &usbep->ep );
}

/**
 * Get endpoint MTU
 *
 * @v usbintf		EFI USB interface
 * @v endpoint		Endpoint address
 * @ret mtu		Endpoint MTU, or negative error
 */
static int efi_usb_mtu ( struct efi_usb_interface *usbintf,
			 unsigned int endpoint ) {
	struct efi_usb_device *usbdev = usbintf->usbdev;
	struct usb_interface_descriptor *interface;
	struct usb_endpoint_descriptor *desc;

	/* Locate cached interface descriptor */
	interface = usb_interface_descriptor ( usbdev->config,
					       usbintf->interface,
					       usbintf->alternate );
	if ( ! interface ) {
		DBGC ( usbdev, "USBDEV %s alt %d has no interface descriptor\n",
		       usbintf->name, usbintf->alternate );
		return -ENOENT;
	}

	/* Locate and copy cached endpoint descriptor */
	for_each_interface_descriptor ( desc, usbdev->config, interface ) {
		if ( ( desc->header.type == USB_ENDPOINT_DESCRIPTOR ) &&
		     ( desc->endpoint == endpoint ) )
			return USB_ENDPOINT_MTU ( le16_to_cpu ( desc->sizes ) );
	}

	DBGC ( usbdev, "USBDEV %s alt %d ep %02x has no descriptor\n",
	       usbintf->name, usbintf->alternate, endpoint );
	return -ENOENT;
}

/**
 * Open endpoint
 *
 * @v usbintf		EFI USB interface
 * @v endpoint		Endpoint address
 * @v attributes	Endpoint attributes
 * @v interval		Interval (in milliseconds)
 * @v driver		Driver operations
 * @ret rc		Return status code
 */
static int efi_usb_open ( struct efi_usb_interface *usbintf,
			  unsigned int endpoint, unsigned int attributes,
			  unsigned int interval,
			  struct usb_endpoint_driver_operations *driver ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct efi_usb_device *usbdev = usbintf->usbdev;
	struct efi_usb_endpoint *usbep;
	unsigned int index = USB_ENDPOINT_IDX ( endpoint );
	int mtu;
	EFI_STATUS efirc;
	int rc;

	/* Get endpoint MTU */
	mtu = efi_usb_mtu ( usbintf, endpoint );
	if ( mtu < 0 ) {
		rc = mtu;
		goto err_mtu;
	}

	/* Allocate and initialise structure */
	usbep = zalloc ( sizeof ( *usbep ) );
	if ( ! usbep ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	usbep->usbintf = usbintf;
	usb_endpoint_init ( &usbep->ep, usbdev->usb, driver );
	usb_endpoint_describe ( &usbep->ep, endpoint, attributes, mtu, 0,
				( interval << 3 /* microframes */ ) );

	/* Open endpoint */
	if ( ( rc = usb_endpoint_open ( &usbep->ep ) ) != 0 ) {
		DBGC ( usbdev, "USBDEV %s %s could not open: %s\n",
		       usbintf->name, usb_endpoint_name ( &usbep->ep ),
		       strerror ( rc ) );
		goto err_open;
	}

	/* Record opened endpoint */
	usbintf->endpoint[index] = usbep;
	DBGC ( usbdev, "USBDEV %s %s opened\n",
	       usbintf->name, usb_endpoint_name ( &usbep->ep ) );

	/* Create event */
	if ( ( efirc = bs->CreateEvent ( ( EVT_TIMER | EVT_NOTIFY_SIGNAL ),
					 TPL_NOTIFY, efi_usb_timer, usbep,
					 &usbep->event ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( usbdev, "USBDEV %s %s could not create event: %s\n",
		       usbintf->name, usb_endpoint_name ( &usbep->ep ),
		       strerror ( rc ) );
		goto err_event;
	}

	return 0;

	bs->CloseEvent ( usbep->event );
 err_event:
	usbintf->endpoint[index] = usbep;
	usb_endpoint_close ( &usbep->ep );
 err_open:
	free ( usbep );
 err_alloc:
 err_mtu:
	return rc;
}

/**
 * Close endpoint
 *
 * @v usbep		EFI USB endpoint
 */
static void efi_usb_close ( struct efi_usb_endpoint *usbep ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct efi_usb_interface *usbintf = usbep->usbintf;
	struct efi_usb_device *usbdev = usbintf->usbdev;
	unsigned int index = USB_ENDPOINT_IDX ( usbep->ep.address );

	/* Sanity check */
	assert ( usbintf->endpoint[index] == usbep );

	/* Cancel timer (if applicable) and close event */
	bs->SetTimer ( usbep->event, TimerCancel, 0 );
	bs->CloseEvent ( usbep->event );

	/* Close endpoint */
	usb_endpoint_close ( &usbep->ep );
	DBGC ( usbdev, "USBDEV %s %s closed\n",
	       usbintf->name, usb_endpoint_name ( &usbep->ep ) );

	/* Free endpoint */
	free ( usbep );

	/* Record closed endpoint */
	usbintf->endpoint[index] = NULL;
}

/**
 * Close all endpoints
 *
 * @v usbintf		EFI USB interface
 */
static void efi_usb_close_all ( struct efi_usb_interface *usbintf ) {
	struct efi_usb_endpoint *usbep;
	unsigned int i;

	for ( i = 0 ; i < ( sizeof ( usbintf->endpoint ) /
			    sizeof ( usbintf->endpoint[0] ) ) ; i++ ) {
		usbep = usbintf->endpoint[i];
		if ( usbep )
			efi_usb_close ( usbep );
	}
}

/**
 * Complete synchronous transfer
 *
 * @v ep		USB endpoint
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void efi_usb_sync_complete ( struct usb_endpoint *ep,
				    struct io_buffer *iobuf __unused, int rc ) {
	struct efi_usb_endpoint *usbep =
		container_of ( ep, struct efi_usb_endpoint, ep );

	/* Record completion status */
	usbep->rc = rc;
}

/** Synchronous endpoint operations */
static struct usb_endpoint_driver_operations efi_usb_sync_driver = {
	.complete = efi_usb_sync_complete,
};

/**
 * Perform synchronous transfer
 *
 * @v usbintf		USB endpoint
 * @v endpoint		Endpoint address
 * @v attributes	Endpoint attributes
 * @v timeout		Timeout (in milliseconds)
 * @v data		Data buffer
 * @v len		Length of data buffer
 * @ret rc		Return status code
 */
static int efi_usb_sync_transfer ( struct efi_usb_interface *usbintf,
				   unsigned int endpoint,
				   unsigned int attributes,
				   unsigned int timeout,
				   void *data, size_t *len ) {
	struct efi_usb_device *usbdev = usbintf->usbdev;
	struct efi_usb_endpoint *usbep;
	struct io_buffer *iobuf;
	unsigned int index = USB_ENDPOINT_IDX ( endpoint );
	unsigned int i;
	int rc;

	/* Open endpoint, if applicable */
	if ( ( ! usbintf->endpoint[index] ) &&
	     ( ( rc = efi_usb_open ( usbintf, endpoint, attributes, 0,
				     &efi_usb_sync_driver ) ) != 0 ) ) {
		goto err_open;
	}
	usbep = usbintf->endpoint[index];

	/* Allocate and construct I/O buffer */
	iobuf = alloc_iob ( *len );
	if ( ! iobuf ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	iob_put ( iobuf, *len );
	if ( ! ( endpoint & USB_ENDPOINT_IN ) )
		memcpy ( iobuf->data, data, *len );

	/* Initialise completion status */
	usbep->rc = -EINPROGRESS;

	/* Enqueue transfer */
	if ( ( rc = usb_stream ( &usbep->ep, iobuf, 0 ) ) != 0 ) {
		DBGC ( usbdev, "USBDEV %s %s could not enqueue: %s\n",
		       usbintf->name, usb_endpoint_name ( &usbep->ep ),
		       strerror ( rc ) );
		goto err_stream;
	}

	/* Wait for completion */
	rc = -ETIMEDOUT;
	for ( i = 0 ; ( ( timeout == 0 ) || ( i < timeout ) ) ; i++ ) {

		/* Poll bus */
		efi_usb_poll ( usbdev );

		/* Check for completion */
		if ( usbep->rc != -EINPROGRESS ) {
			rc = usbep->rc;
			break;
		}

		/* Delay */
		mdelay ( 1 );
	}

	/* Check for errors */
	if ( rc != 0 ) {
		DBGC ( usbdev, "USBDEV %s %s failed: %s\n", usbintf->name,
		       usb_endpoint_name ( &usbep->ep ), strerror ( rc ) );
		goto err_completion;
	}

	/* Copy completion to data buffer, if applicable */
	assert ( iob_len ( iobuf ) <= *len );
	if ( endpoint & USB_ENDPOINT_IN )
		memcpy ( data, iobuf->data, iob_len ( iobuf ) );
	*len = iob_len ( iobuf );

	/* Free I/O buffer */
	free_iob ( iobuf );

	/* Leave endpoint open */
	return 0;

 err_completion:
 err_stream:
	free_iob ( iobuf );
 err_alloc:
	efi_usb_close ( usbep );
 err_open:
	return EFIRC ( rc );
}

/**
 * Complete asynchronous transfer
 *
 * @v ep		USB endpoint
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void efi_usb_async_complete ( struct usb_endpoint *ep,
				     struct io_buffer *iobuf, int rc ) {
	struct efi_usb_endpoint *usbep =
		container_of ( ep, struct efi_usb_endpoint, ep );
	UINT32 status;

	/* Ignore packets cancelled when the endpoint closes */
	if ( ! ep->open )
		goto drop;

	/* Construct status */
	status = ( ( rc == 0 ) ? 0 : EFI_USB_ERR_STALL );

	/* Report completion */
	usbep->callback ( iobuf->data, iob_len ( iobuf ), usbep->context,
			  status );

 drop:
	/* Recycle I/O buffer */
	usb_recycle ( &usbep->ep, iobuf );
}

/** Asynchronous endpoint operations */
static struct usb_endpoint_driver_operations efi_usb_async_driver = {
	.complete = efi_usb_async_complete,
};

/**
 * Start asynchronous transfer
 *
 * @v usbintf		EFI USB interface
 * @v endpoint		Endpoint address
 * @v interval		Interval (in milliseconds)
 * @v len		Transfer length
 * @v callback		Callback function
 * @v context		Context for callback function
 * @ret rc		Return status code
 */
static int efi_usb_async_start ( struct efi_usb_interface *usbintf,
				 unsigned int endpoint, unsigned int interval,
				 size_t len,
				 EFI_ASYNC_USB_TRANSFER_CALLBACK callback,
				 void *context ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct efi_usb_device *usbdev = usbintf->usbdev;
	struct efi_usb_endpoint *usbep;
	unsigned int index = USB_ENDPOINT_IDX ( endpoint );
	EFI_STATUS efirc;
	int rc;

	/* Open endpoint */
	if ( ( rc = efi_usb_open ( usbintf, endpoint,
				   USB_ENDPOINT_ATTR_INTERRUPT, interval,
				   &efi_usb_async_driver ) ) != 0 )
		goto err_open;
	usbep = usbintf->endpoint[index];

	/* Record callback parameters */
	usbep->callback = callback;
	usbep->context = context;

	/* Prefill endpoint */
	usb_refill_init ( &usbep->ep, 0, len, EFI_USB_ASYNC_FILL );
	if ( ( rc = usb_prefill ( &usbep->ep ) ) != 0 ) {
		DBGC ( usbdev, "USBDEV %s %s could not prefill: %s\n",
		       usbintf->name, usb_endpoint_name ( &usbep->ep ),
		       strerror ( rc ) );
		goto err_prefill;
	}

	/* Start timer */
	if ( ( efirc = bs->SetTimer ( usbep->event, TimerPeriodic,
				      ( interval * 10000 ) ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( usbdev, "USBDEV %s %s could not set timer: %s\n",
		       usbintf->name, usb_endpoint_name ( &usbep->ep ),
		       strerror ( rc ) );
		goto err_timer;
	}

	return 0;

	bs->SetTimer ( usbep->event, TimerCancel, 0 );
 err_timer:
 err_prefill:
	efi_usb_close ( usbep );
 err_open:
	return rc;
}

/**
 * Stop asynchronous transfer
 *
 * @v usbintf		EFI USB interface
 * @v endpoint		Endpoint address
 */
static void efi_usb_async_stop ( struct efi_usb_interface *usbintf,
				 unsigned int endpoint ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct efi_usb_endpoint *usbep;
	unsigned int index = USB_ENDPOINT_IDX ( endpoint );

	/* Do nothing if endpoint is already closed */
	usbep = usbintf->endpoint[index];
	if ( ! usbep )
		return;

	/* Stop timer */
	bs->SetTimer ( usbep->event, TimerCancel, 0 );

	/* Close endpoint */
	efi_usb_close ( usbep );
}

/******************************************************************************
 *
 * USB I/O protocol
 *
 ******************************************************************************
 */

/**
 * Perform control transfer
 *
 * @v usbio		USB I/O protocol
 * @v packet		Setup packet
 * @v direction		Data direction
 * @v timeout		Timeout (in milliseconds)
 * @v data		Data buffer
 * @v len		Length of data
 * @ret status		Transfer status
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_control_transfer ( EFI_USB_IO_PROTOCOL *usbio,
			   EFI_USB_DEVICE_REQUEST *packet,
			   EFI_USB_DATA_DIRECTION direction,
			   UINT32 timeout, VOID *data, UINTN len,
			   UINT32 *status ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;
	unsigned int request = ( packet->RequestType |
				 USB_REQUEST_TYPE ( packet->Request ) );
	unsigned int value = le16_to_cpu ( packet->Value );
	unsigned int index = le16_to_cpu ( packet->Index );
	int rc;

	DBGC2 ( usbdev, "USBDEV %s control %04x:%04x:%04x:%04x %s %dms "
		"%p+%zx\n", usbintf->name, request, value, index,
		le16_to_cpu ( packet->Length ),
		efi_usb_direction_name ( direction ), timeout, data,
		( ( size_t ) len ) );

	/* Clear status */
	*status = 0;

	/* Block attempts to change the device configuration, since
	 * this is logically impossible to do given the constraints of
	 * the EFI_USB_IO_PROTOCOL design.
	 */
	if ( ( request == USB_SET_CONFIGURATION ) &&
	     ( value != usbdev->config->config ) ) {
		DBGC ( usbdev, "USBDEV %s cannot set configuration %d: not "
		       "logically possible\n", usbintf->name, index );
		rc = -ENOTSUP;
		goto err_change_config;
	}

	/* If we are selecting a new alternate setting then close all
	 * open endpoints.
	 */
	if ( ( request == USB_SET_INTERFACE ) &&
	     ( value != usbintf->alternate ) )
		efi_usb_close_all ( usbintf );

	/* Issue control transfer */
	if ( ( rc = usb_control ( usbdev->usb, request, value, index,
				  data, len ) ) != 0 ) {
		DBGC ( usbdev, "USBDEV %s control %04x:%04x:%04x:%04x %p+%zx "
		       "failed: %s\n", usbintf->name, request, value, index,
		       le16_to_cpu ( packet->Length ), data, ( ( size_t ) len ),
		       strerror ( rc ) );
		/* Assume that any error represents a stall */
		*status = EFI_USB_ERR_STALL;
		goto err_control;
	}

	/* Update alternate setting, if applicable */
	if ( request == USB_SET_INTERFACE ) {
		usbintf->alternate = value;
		DBGC ( usbdev, "USBDEV %s alt %d selected\n",
		       usbintf->name, usbintf->alternate );
	}

 err_control:
 err_change_config:
	return EFIRC ( rc );
}

/**
 * Perform bulk transfer
 *
 * @v usbio		USB I/O protocol
 * @v endpoint		Endpoint address
 * @v data		Data buffer
 * @v len		Length of data
 * @v timeout		Timeout (in milliseconds)
 * @ret status		Transfer status
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_bulk_transfer ( EFI_USB_IO_PROTOCOL *usbio, UINT8 endpoint, VOID *data,
			UINTN *len, UINTN timeout, UINT32 *status ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;
	size_t actual = *len;
	int rc;

	DBGC2 ( usbdev, "USBDEV %s bulk %s %p+%zx %dms\n", usbintf->name,
		( ( endpoint & USB_ENDPOINT_IN ) ? "IN" : "OUT" ), data,
		( ( size_t ) *len ), ( ( unsigned int ) timeout ) );

	/* Clear status */
	*status = 0;

	/* Perform synchronous transfer */
	if ( ( rc = efi_usb_sync_transfer ( usbintf, endpoint,
					    USB_ENDPOINT_ATTR_BULK, timeout,
					    data, &actual ) ) != 0 ) {
		/* Assume that any error represents a timeout */
		*status = EFI_USB_ERR_TIMEOUT;
		return rc;
	}

	return 0;
}

/**
 * Perform synchronous interrupt transfer
 *
 * @v usbio		USB I/O protocol
 * @v endpoint		Endpoint address
 * @v data		Data buffer
 * @v len		Length of data
 * @v timeout		Timeout (in milliseconds)
 * @ret status		Transfer status
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_sync_interrupt_transfer ( EFI_USB_IO_PROTOCOL *usbio, UINT8 endpoint,
				  VOID *data, UINTN *len, UINTN timeout,
				  UINT32 *status ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;
	size_t actual = *len;
	int rc;

	DBGC2 ( usbdev, "USBDEV %s sync intr %s %p+%zx %dms\n", usbintf->name,
		( ( endpoint & USB_ENDPOINT_IN ) ? "IN" : "OUT" ), data,
		( ( size_t ) *len ), ( ( unsigned int ) timeout ) );

	/* Clear status */
	*status = 0;

	/* Perform synchronous transfer */
	if ( ( rc = efi_usb_sync_transfer ( usbintf, endpoint,
					    USB_ENDPOINT_ATTR_INTERRUPT,
					    timeout, data, &actual ) ) != 0 ) {
		/* Assume that any error represents a timeout */
		*status = EFI_USB_ERR_TIMEOUT;
		return rc;
	}

	return 0;
}

/**
 * Perform asynchronous interrupt transfer
 *
 * @v usbio		USB I/O protocol
 * @v endpoint		Endpoint address
 * @v start		Start (rather than stop) transfer
 * @v interval		Polling interval (in milliseconds)
 * @v len		Data length
 * @v callback		Callback function
 * @v context		Context for callback function
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_async_interrupt_transfer ( EFI_USB_IO_PROTOCOL *usbio, UINT8 endpoint,
				   BOOLEAN start, UINTN interval, UINTN len,
				   EFI_ASYNC_USB_TRANSFER_CALLBACK callback,
				   VOID *context ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;
	int rc;

	DBGC2 ( usbdev, "USBDEV %s async intr %s len %#zx int %d %p/%p\n",
		usbintf->name,
		( ( endpoint & USB_ENDPOINT_IN ) ? "IN" : "OUT" ),
		( ( size_t ) len ), ( ( unsigned int ) interval ),
		callback, context );

	/* Start/stop transfer as applicable */
	if ( start ) {

		/* Start new transfer */
		if ( ( rc = efi_usb_async_start ( usbintf, endpoint, interval,
						  len, callback,
						  context ) ) != 0 )
			goto err_start;

	} else {

		/* Stop transfer */
		efi_usb_async_stop ( usbintf, endpoint );

	}

	return 0;

 err_start:
	return EFIRC ( rc );
}

/**
 * Perform synchronous isochronous transfer
 *
 * @v usbio		USB I/O protocol
 * @v endpoint		Endpoint address
 * @v data		Data buffer
 * @v len		Length of data
 * @ret status		Transfer status
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_isochronous_transfer ( EFI_USB_IO_PROTOCOL *usbio, UINT8 endpoint,
			       VOID *data, UINTN len, UINT32 *status ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;

	DBGC2 ( usbdev, "USBDEV %s sync iso %s %p+%zx\n", usbintf->name,
		( ( endpoint & USB_ENDPOINT_IN ) ? "IN" : "OUT" ), data,
		( ( size_t ) len ) );

	/* Clear status */
	*status = 0;

	/* Not supported */
	return EFI_UNSUPPORTED;
}

/**
 * Perform asynchronous isochronous transfers
 *
 * @v usbio		USB I/O protocol
 * @v endpoint		Endpoint address
 * @v data		Data buffer
 * @v len		Length of data
 * @v callback		Callback function
 * @v context		Context for callback function
 * @ret status		Transfer status
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_async_isochronous_transfer ( EFI_USB_IO_PROTOCOL *usbio, UINT8 endpoint,
				     VOID *data, UINTN len,
				     EFI_ASYNC_USB_TRANSFER_CALLBACK callback,
				     VOID *context ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;

	DBGC2 ( usbdev, "USBDEV %s async iso %s %p+%zx %p/%p\n", usbintf->name,
		( ( endpoint & USB_ENDPOINT_IN ) ? "IN" : "OUT" ), data,
		( ( size_t ) len ), callback, context );

	/* Not supported */
	return EFI_UNSUPPORTED;
}

/**
 * Get device descriptor
 *
 * @v usbio		USB I/O protocol
 * @ret efidesc		EFI device descriptor
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_get_device_descriptor ( EFI_USB_IO_PROTOCOL *usbio,
				EFI_USB_DEVICE_DESCRIPTOR *efidesc ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;

	DBGC2 ( usbdev, "USBDEV %s get device descriptor\n", usbintf->name );

	/* Copy cached device descriptor */
	memcpy ( efidesc, &usbdev->usb->device, sizeof ( *efidesc ) );

	return 0;
}

/**
 * Get configuration descriptor
 *
 * @v usbio		USB I/O protocol
 * @ret efidesc		EFI interface descriptor
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_get_config_descriptor ( EFI_USB_IO_PROTOCOL *usbio,
				EFI_USB_CONFIG_DESCRIPTOR *efidesc ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;

	DBGC2 ( usbdev, "USBDEV %s get configuration descriptor\n",
		usbintf->name );

	/* Copy cached configuration descriptor */
	memcpy ( efidesc, usbdev->config, sizeof ( *efidesc ) );

	return 0;
}

/**
 * Get interface descriptor
 *
 * @v usbio		USB I/O protocol
 * @ret efidesc		EFI interface descriptor
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_get_interface_descriptor ( EFI_USB_IO_PROTOCOL *usbio,
				   EFI_USB_INTERFACE_DESCRIPTOR *efidesc ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;
	struct usb_interface_descriptor *desc;

	DBGC2 ( usbdev, "USBDEV %s get interface descriptor\n", usbintf->name );

	/* Locate cached interface descriptor */
	desc = usb_interface_descriptor ( usbdev->config, usbintf->interface,
					  usbintf->alternate );
	if ( ! desc ) {
		DBGC ( usbdev, "USBDEV %s alt %d has no interface descriptor\n",
		       usbintf->name, usbintf->alternate );
		return -ENOENT;
	}

	/* Copy cached interface descriptor */
	memcpy ( efidesc, desc, sizeof ( *efidesc ) );

	return 0;
}

/**
 * Get endpoint descriptor
 *
 * @v usbio		USB I/O protocol
 * @v address		Endpoint index
 * @ret efidesc		EFI interface descriptor
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_get_endpoint_descriptor ( EFI_USB_IO_PROTOCOL *usbio, UINT8 index,
				  EFI_USB_ENDPOINT_DESCRIPTOR *efidesc ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;
	struct usb_interface_descriptor *interface;
	struct usb_endpoint_descriptor *desc;

	DBGC2 ( usbdev, "USBDEV %s get endpoint %d descriptor\n",
		usbintf->name, index );

	/* Locate cached interface descriptor */
	interface = usb_interface_descriptor ( usbdev->config,
					       usbintf->interface,
					       usbintf->alternate );
	if ( ! interface ) {
		DBGC ( usbdev, "USBDEV %s alt %d has no interface descriptor\n",
		       usbintf->name, usbintf->alternate );
		return -ENOENT;
	}

	/* Locate and copy cached endpoint descriptor */
	for_each_interface_descriptor ( desc, usbdev->config, interface ) {
		if ( ( desc->header.type == USB_ENDPOINT_DESCRIPTOR ) &&
		     ( index-- == 0 ) ) {
			memcpy ( efidesc, desc, sizeof ( *efidesc ) );
			return 0;
		}
	}
	return -ENOENT;
}

/**
 * Get string descriptor
 *
 * @v usbio		USB I/O protocol
 * @v language		Language ID
 * @v index		String index
 * @ret string		String
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_get_string_descriptor ( EFI_USB_IO_PROTOCOL *usbio, UINT16 language,
				UINT8 index, CHAR16 **string ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;
	struct usb_descriptor_header header;
	VOID *buffer;
	size_t len;
	EFI_STATUS efirc;
	int rc;

	DBGC2 ( usbdev, "USBDEV %s get string %d:%d descriptor\n",
		usbintf->name, language, index );

	/* Read descriptor header */
	if ( ( rc = usb_get_descriptor ( usbdev->usb, 0, USB_STRING_DESCRIPTOR,
					 index, language, &header,
					 sizeof ( header ) ) ) != 0 ) {
		DBGC ( usbdev, "USBDEV %s could not get string %d:%d "
		       "descriptor header: %s\n", usbintf->name, language,
		       index, strerror ( rc ) );
		goto err_get_header;
	}
	len = header.len;

	/* Allocate buffer */
	if ( ( efirc = bs->AllocatePool ( EfiBootServicesData, len,
					  &buffer ) ) != 0 ) {
		rc = -EEFI ( efirc );
		goto err_alloc;
	}

	/* Read whole descriptor */
	if ( ( rc = usb_get_descriptor ( usbdev->usb, 0, USB_STRING_DESCRIPTOR,
					 index, language, buffer,
					 len ) ) != 0 ) {
		DBGC ( usbdev, "USBDEV %s could not get string %d:%d "
		       "descriptor: %s\n", usbintf->name, language,
		       index, strerror ( rc ) );
		goto err_get_descriptor;
	}

	/* Shuffle down and terminate string */
	memmove ( buffer, ( buffer + sizeof ( header ) ),
		  ( len - sizeof ( header ) ) );
	memset ( ( buffer + len - sizeof ( header ) ), 0, sizeof ( **string ) );

	/* Return allocated string */
	*string = buffer;
	return 0;

 err_get_descriptor:
	bs->FreePool ( buffer );
 err_alloc:
 err_get_header:
	return EFIRC ( rc );
}

/**
 * Get supported languages
 *
 * @v usbio		USB I/O protocol
 * @ret languages	Language ID table
 * @ret len		Length of language ID table
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_get_supported_languages ( EFI_USB_IO_PROTOCOL *usbio,
				  UINT16 **languages, UINT16 *len ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;

	DBGC2 ( usbdev, "USBDEV %s get supported languages\n", usbintf->name );

	/* Return cached supported languages */
	*languages = ( ( ( void * ) usbdev->languages ) +
		       sizeof ( *(usbdev->languages) ) );
	*len = usbdev->languages->len;

	return 0;
}

/**
 * Reset port
 *
 * @v usbio		USB I/O protocol
 * @ret efirc		EFI status code
 */
static EFI_STATUS EFIAPI
efi_usb_port_reset ( EFI_USB_IO_PROTOCOL *usbio ) {
	struct efi_usb_interface *usbintf =
		container_of ( usbio, struct efi_usb_interface, usbio );
	struct efi_usb_device *usbdev = usbintf->usbdev;

	DBGC2 ( usbdev, "USBDEV %s reset port\n", usbintf->name );

	/* This is logically impossible to do, since resetting the
	 * port may destroy state belonging to other
	 * EFI_USB_IO_PROTOCOL instances belonging to the same USB
	 * device.  (This is yet another artifact of the incredibly
	 * poor design of the EFI_USB_IO_PROTOCOL.)
	 */
	return EFI_INVALID_PARAMETER;
}

/** USB I/O protocol */
static EFI_USB_IO_PROTOCOL efi_usb_io_protocol = {
	.UsbControlTransfer		= efi_usb_control_transfer,
	.UsbBulkTransfer		= efi_usb_bulk_transfer,
	.UsbAsyncInterruptTransfer	= efi_usb_async_interrupt_transfer,
	.UsbSyncInterruptTransfer	= efi_usb_sync_interrupt_transfer,
	.UsbIsochronousTransfer		= efi_usb_isochronous_transfer,
	.UsbAsyncIsochronousTransfer	= efi_usb_async_isochronous_transfer,
	.UsbGetDeviceDescriptor		= efi_usb_get_device_descriptor,
	.UsbGetConfigDescriptor		= efi_usb_get_config_descriptor,
	.UsbGetInterfaceDescriptor	= efi_usb_get_interface_descriptor,
	.UsbGetEndpointDescriptor	= efi_usb_get_endpoint_descriptor,
	.UsbGetStringDescriptor		= efi_usb_get_string_descriptor,
	.UsbGetSupportedLanguages	= efi_usb_get_supported_languages,
	.UsbPortReset			= efi_usb_port_reset,
};

/******************************************************************************
 *
 * USB driver
 *
 ******************************************************************************
 */

/**
 * Install interface
 *
 * @v usbdev		EFI USB device
 * @v interface		Interface number
 * @ret rc		Return status code
 */
static int efi_usb_install ( struct efi_usb_device *usbdev,
			     unsigned int interface ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct efi_device *efidev = usbdev->efidev;
	struct efi_usb_interface *usbintf;
	struct usb_device *usb;
	EFI_DEVICE_PATH_PROTOCOL *path_end;
	USB_DEVICE_PATH *usbpath;
	unsigned int path_count;
	size_t path_prefix_len;
	size_t path_len;
	EFI_STATUS efirc;
	int rc;

	/* Calculate device path length */
	path_count = ( usb_depth ( usbdev->usb ) + 1 );
	path_prefix_len = efi_devpath_len ( efidev->path );
	path_len = ( path_prefix_len + ( path_count * sizeof ( *usbpath ) ) +
		     sizeof ( *path_end ) );

	/* Allocate and initialise structure */
	usbintf = zalloc ( sizeof ( *usbintf ) + path_len );
	if ( ! usbintf ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	snprintf ( usbintf->name, sizeof ( usbintf->name ), "%s[%d]",
		   usbdev->name, interface );
	usbintf->usbdev = usbdev;
	usbintf->interface = interface;
	memcpy ( &usbintf->usbio, &efi_usb_io_protocol,
		 sizeof ( usbintf->usbio ) );
	usbintf->path = ( ( ( void * ) usbintf ) + sizeof ( *usbintf ) );

	/* Construct device path */
	memcpy ( usbintf->path, efidev->path, path_prefix_len );
	path_end = ( ( ( void * ) usbintf->path ) + path_len -
		     sizeof ( *path_end ) );
	path_end->Type = END_DEVICE_PATH_TYPE;
	path_end->SubType = END_ENTIRE_DEVICE_PATH_SUBTYPE;
	path_end->Length[0] = sizeof ( *path_end );
	usbpath = ( ( ( void * ) path_end ) - sizeof ( *usbpath ) );
	usbpath->InterfaceNumber = interface;
	for ( usb = usbdev->usb ; usb ; usbpath--, usb = usb->port->hub->usb ) {
		usbpath->Header.Type = MESSAGING_DEVICE_PATH;
		usbpath->Header.SubType = MSG_USB_DP;
		usbpath->Header.Length[0] = sizeof ( *usbpath );
		usbpath->ParentPortNumber = usb->port->address;
	}

	/* Add to list of interfaces */
	list_add_tail ( &usbintf->list, &usbdev->interfaces );

	/* Install protocols */
	if ( ( efirc = bs->InstallMultipleProtocolInterfaces (
			&usbintf->handle,
			&efi_usb_io_protocol_guid, &usbintf->usbio,
			&efi_device_path_protocol_guid, usbintf->path,
			NULL ) ) != 0 ) {
		rc = -EEFI ( efirc );
		DBGC ( usbdev, "USBDEV %s could not install protocols: %s\n",
		       usbintf->name, strerror ( rc ) );
		goto err_install_protocol;
	}

	DBGC ( usbdev, "USBDEV %s installed as %s\n",
	       usbintf->name, efi_handle_name ( usbintf->handle ) );
	return 0;

	efi_usb_close_all ( usbintf );
	bs->UninstallMultipleProtocolInterfaces (
			usbintf->handle,
			&efi_usb_io_protocol_guid, &usbintf->usbio,
			&efi_device_path_protocol_guid, usbintf->path,
			NULL );
 err_install_protocol:
	list_del ( &usbintf->list );
	free ( usbintf );
 err_alloc:
	return rc;
}

/**
 * Uninstall interface
 *
 * @v usbintf		EFI USB interface
 */
static void efi_usb_uninstall ( struct efi_usb_interface *usbintf ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;

	/* Close all endpoints */
	efi_usb_close_all ( usbintf );

	/* Uninstall protocols */
	bs->UninstallMultipleProtocolInterfaces (
			usbintf->handle,
			&efi_usb_io_protocol_guid, &usbintf->usbio,
			&efi_device_path_protocol_guid, usbintf->path,
			NULL );

	/* Remove from list of interfaces */
	list_del ( &usbintf->list );

	/* Free interface */
	free ( usbintf );
}

/**
 * Uninstall all interfaces
 *
 * @v usbdev		EFI USB device
 */
static void efi_usb_uninstall_all ( struct efi_usb_device *efiusb ) {
	struct efi_usb_interface *usbintf;

	/* Uninstall all interfaces */
	while ( ( usbintf = list_first_entry ( &efiusb->interfaces,
					       struct efi_usb_interface,
					       list ) ) ) {
		efi_usb_uninstall ( usbintf );
	}
}

/**
 * Probe device
 *
 * @v func		USB function
 * @v config		Configuration descriptor
 * @ret rc		Return status code
 */
static int efi_usb_probe ( struct usb_function *func,
			   struct usb_configuration_descriptor *config ) {
	EFI_BOOT_SERVICES *bs = efi_systab->BootServices;
	struct usb_device *usb = func->usb;
	struct efi_usb_device *usbdev;
	struct efi_usb_interface *usbintf;
	struct efi_device *efidev;
	struct usb_descriptor_header header;
	size_t config_len;
	unsigned int i;
	int rc;

	/* Find parent EFI device */
	efidev = efidev_parent ( &func->dev );
	if ( ! efidev ) {
		rc = -ENOTTY;
		goto err_no_efidev;
	}

	/* Get configuration length */
	config_len = le16_to_cpu ( config->len );

	/* Get supported languages descriptor header */
	if ( ( rc = usb_get_descriptor ( usb, 0, USB_STRING_DESCRIPTOR, 0, 0,
					 &header, sizeof ( header ) ) ) != 0 ) {
		/* Assume no strings are present */
		header.len = 0;
	}

	/* Allocate and initialise structure */
	usbdev = zalloc ( sizeof ( *usbdev ) + config_len + header.len );
	if ( ! usbdev ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	usb_func_set_drvdata ( func, usbdev );
	usbdev->name = func->name;
	usbdev->usb = usb;
	usbdev->efidev = efidev;
	usbdev->config = ( ( ( void * ) usbdev ) + sizeof ( *usbdev ) );
	memcpy ( usbdev->config, config, config_len );
	usbdev->languages = ( ( ( void * ) usbdev->config ) + config_len );
	INIT_LIST_HEAD ( &usbdev->interfaces );

	/* Get supported languages descriptor */
	if ( header.len &&
	     ( rc = usb_get_descriptor ( usb, 0, USB_STRING_DESCRIPTOR, 0, 0,
					 usbdev->languages,
					 header.len ) ) != 0 ) {
		DBGC ( usbdev, "USBDEV %s could not get supported languages: "
		       "%s\n", usbdev->name, strerror ( rc ) );
		goto err_get_languages;
	}

	/* Install interfaces */
	for ( i = 0 ; i < func->desc.count ; i++ ) {
		if ( ( rc = efi_usb_install ( usbdev,
					      func->interface[i] ) ) != 0 )
			goto err_install;
	}

	/* Connect any external drivers */
	list_for_each_entry ( usbintf, &usbdev->interfaces, list )
		bs->ConnectController ( usbintf->handle, NULL, NULL, TRUE );

	return 0;

 err_install:
	efi_usb_uninstall_all ( usbdev );
	assert ( list_empty ( &usbdev->interfaces ) );
 err_get_languages:
	free ( usbdev );
 err_alloc:
 err_no_efidev:
	return rc;
}

/**
 * Remove device
 *
 * @v func		USB function
 */
static void efi_usb_remove ( struct usb_function *func ) {
	struct efi_usb_device *usbdev = usb_func_get_drvdata ( func );

	/* Uninstall all interfaces */
	efi_usb_uninstall_all ( usbdev );
	assert ( list_empty ( &usbdev->interfaces ) );

	/* Free device */
	free ( usbdev );
}

/** USB I/O protocol device IDs */
static struct usb_device_id efi_usb_ids[] = {
	{
		.name = "usbio",
		.vendor = USB_ANY_ID,
		.product = USB_ANY_ID,
	},
};

/** USB I/O protocol driver */
struct usb_driver usbio_driver __usb_driver = {
	.ids = efi_usb_ids,
	.id_count = ( sizeof ( efi_usb_ids ) / sizeof ( efi_usb_ids[0] ) ),
	.class = USB_CLASS_ID ( USB_ANY_ID, USB_ANY_ID, USB_ANY_ID ),
	.score = USB_SCORE_FALLBACK,
	.probe = efi_usb_probe,
	.remove = efi_usb_remove,
};

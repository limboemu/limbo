#ifndef _IPXE_EFI_USB_H
#define _IPXE_EFI_USB_H

/** @file
 *
 * USB I/O protocol
 *
 */

#include <ipxe/list.h>
#include <ipxe/efi/efi.h>
#include <ipxe/efi/efi_driver.h>
#include <ipxe/efi/Protocol/UsbIo.h>
#include <ipxe/usb.h>

/** An EFI USB device */
struct efi_usb_device {
	/** Name */
	const char *name;
	/** The underlying USB device */
	struct usb_device *usb;
	/** The underlying EFI device */
	struct efi_device *efidev;
	/** Configuration descriptor */
	struct usb_configuration_descriptor *config;
	/** Supported languages */
	struct usb_descriptor_header *languages;
	/** List of interfaces */
	struct list_head interfaces;
};

/** An EFI USB device interface */
struct efi_usb_interface {
	/** Name */
	char name[32];
	/** Containing USB device */
	struct efi_usb_device *usbdev;
	/** List of interfaces */
	struct list_head list;

	/** Interface number */
	unsigned int interface;
	/** Alternate setting */
	unsigned int alternate;
	/** EFI handle */
	EFI_HANDLE handle;
	/** USB I/O protocol */
	EFI_USB_IO_PROTOCOL usbio;
	/** Device path */
	EFI_DEVICE_PATH_PROTOCOL *path;

	/** Opened endpoints */
	struct efi_usb_endpoint *endpoint[32];
};

/** An EFI USB device endpoint */
struct efi_usb_endpoint {
	/** EFI USB device interface */
	struct efi_usb_interface *usbintf;
	/** USB endpoint */
	struct usb_endpoint ep;

	/** Most recent synchronous completion status */
	int rc;

	/** Asynchronous timer event */
	EFI_EVENT event;
	/** Asynchronous callback handler */
	EFI_ASYNC_USB_TRANSFER_CALLBACK callback;
	/** Asynchronous callback context */
	void *context;
};

/** Asynchronous transfer fill level
 *
 * This is a policy decision.
 */
#define EFI_USB_ASYNC_FILL 2

#endif /* _IPXE_EFI_USB_H */

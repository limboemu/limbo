#ifndef _USBIO_H
#define _USBIO_H

/** @file
 *
 * EFI_USB_IO_PROTOCOL pseudo Host Controller Interface driver
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/list.h>
#include <ipxe/device.h>
#include <ipxe/efi/efi.h>
#include <ipxe/efi/Protocol/UsbIo.h>
#include <ipxe/efi/Protocol/DevicePath.h>
#include <ipxe/usb.h>

/** USB I/O maximum transfer size
 *
 * The API provides no way to discover the maximum transfer size.
 * Assume the 16kB supported by EHCI.
 */
#define USBIO_MTU 16384

/** USB I/O interrupt ring buffer size
 *
 * This is a policy decision.
 */
#define USBIO_INTR_COUNT 4

/** A USB interrupt ring buffer */
struct usbio_interrupt_ring {
	/** USB I/O endpoint */
	struct usbio_endpoint *endpoint;
	/** Producer counter */
	unsigned int prod;
	/** Consumer counter */
	unsigned int cons;
	/** Data buffers */
	void *data[USBIO_INTR_COUNT];
	/** Lengths */
	size_t len[USBIO_INTR_COUNT];
};

/** USB I/O ring buffer size
 *
 * This is a policy decision.
 */
#define USBIO_RING_COUNT 64

/** A USB I/O endpoint */
struct usbio_endpoint {
	/** USB I/O device */
	struct usbio_device *usbio;
	/** USB endpoint */
	struct usb_endpoint *ep;
	/** List of endpoints */
	struct list_head list;
	/** USB I/O endpoint operations */
	struct usbio_operations *op;

	/** Containing interface number */
	unsigned int interface;
	/** EFI handle */
	EFI_HANDLE handle;
	/** USB I/O protocol */
	EFI_USB_IO_PROTOCOL *io;

	/** Producer counter */
	unsigned int prod;
	/** Consumer counter */
	unsigned int cons;
	/** I/O buffers */
	struct io_buffer *iobuf[USBIO_RING_COUNT];
	/** Flags */
	uint8_t flags[USBIO_RING_COUNT];

	/** Interrupt ring buffer (if applicable) */
	struct usbio_interrupt_ring *intr;
};

/** USB I/O transfer flags */
enum usbio_flags {
	/** This is a message transfer */
	USBIO_MESSAGE = 0x01,
	/** This transfer requires zero-length packet termination */
	USBIO_ZLEN = 0x02,
};

/** USB I/O endpoint operations */
struct usbio_operations {
	/** Open endpoint
	 *
	 * @v endpoint		Endpoint
	 * @ret rc		Return status code
	 */
	int ( * open ) ( struct usbio_endpoint *endpoint );
	/** Close endpoint
	 *
	 * @v endpoint		Endpoint
	 */
	void ( * close ) ( struct usbio_endpoint *endpoint );
	/** Poll endpoint
	 *
	 * @v endpoint		Endpoint
	 */
	void ( * poll ) ( struct usbio_endpoint *endpoint );
};

/** A USB I/O protocol interface */
struct usbio_interface {
	/** EFI device handle */
	EFI_HANDLE handle;
	/** USB I/O protocol */
	EFI_USB_IO_PROTOCOL *io;
	/** Usage count */
	unsigned int count;
};

/** A USB I/O protocol device
 *
 * We model each externally-provided USB I/O protocol device as a host
 * controller containing a root hub with a single port.
 */
struct usbio_device {
	/** EFI device handle */
	EFI_HANDLE handle;
	/** USB I/O protocol */
	EFI_USB_IO_PROTOCOL *io;
	/** Generic device */
	struct device dev;

	/** Configuration descriptor */
	struct usb_configuration_descriptor *config;

	/** Device path */
	EFI_DEVICE_PATH_PROTOCOL *path;
	/** Final component of USB device path */
	USB_DEVICE_PATH *usbpath;

	/** First interface number */
	uint8_t first;
	/** USB I/O protocol interfaces */
	struct usbio_interface *interface;

	/** USB bus */
	struct usb_bus *bus;
	/** List of endpoints */
	struct list_head endpoints;
};

#endif /* _USBIO_H */

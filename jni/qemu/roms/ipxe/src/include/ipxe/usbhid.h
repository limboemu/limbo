#ifndef _IPXE_USBHID_H
#define _IPXE_USBHID_H

/** @file
 *
 * USB human interface devices (HID)
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/usb.h>

/** Class code for human interface devices */
#define USB_CLASS_HID 3

/** Subclass code for boot devices */
#define USB_SUBCLASS_HID_BOOT 1

/** Set protocol */
#define USBHID_SET_PROTOCOL						\
	( USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE |		\
	  USB_REQUEST_TYPE ( 0x0b ) )

/** Boot protocol */
#define USBHID_PROTOCOL_BOOT 0

/** Report protocol */
#define USBHID_PROTOCOL_REPORT 1

/** Set idle time */
#define USBHID_SET_IDLE							\
	( USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE |		\
	  USB_REQUEST_TYPE ( 0x0a ) )

/** Set report */
#define USBHID_SET_REPORT						\
	( USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE |		\
	  USB_REQUEST_TYPE ( 0x09 ) )

/** Input report type */
#define USBHID_REPORT_INPUT 0x01

/** Output report type */
#define USBHID_REPORT_OUTPUT 0x02

/** Feature report type */
#define USBHID_REPORT_FEATURE 0x03

/** A USB human interface device */
struct usb_hid {
	/** USB function */
	struct usb_function *func;
	/** Interrupt IN endpoint */
	struct usb_endpoint in;
	/** Interrupt OUT endpoint (optional) */
	struct usb_endpoint out;
};

/**
 * Initialise USB human interface device
 *
 * @v hid		USB human interface device
 * @v func		USB function
 * @v in		Interrupt IN endpoint operations
 * @v out		Interrupt OUT endpoint operations (or NULL)
 */
static inline __attribute__ (( always_inline )) void
usbhid_init ( struct usb_hid *hid, struct usb_function *func,
	      struct usb_endpoint_driver_operations *in,
	      struct usb_endpoint_driver_operations *out ) {
	struct usb_device *usb = func->usb;

	hid->func = func;
	usb_endpoint_init ( &hid->in, usb, in );
	if ( out )
		usb_endpoint_init ( &hid->out, usb, out );
}

/**
 * Set protocol
 *
 * @v usb		USB device
 * @v interface		Interface number
 * @v protocol		HID protocol
 * @ret rc		Return status code
 */
static inline __attribute__ (( always_inline )) int
usbhid_set_protocol ( struct usb_device *usb, unsigned int interface,
		      unsigned int protocol ) {

	return usb_control ( usb, USBHID_SET_PROTOCOL, protocol, interface,
			     NULL, 0 );
}

/**
 * Set idle time
 *
 * @v usb		USB device
 * @v interface		Interface number
 * @v report		Report ID
 * @v duration		Duration (in 4ms units)
 * @ret rc		Return status code
 */
static inline __attribute__ (( always_inline )) int
usbhid_set_idle ( struct usb_device *usb, unsigned int interface,
		  unsigned int report, unsigned int duration ) {

	return usb_control ( usb, USBHID_SET_IDLE,
			     ( ( duration << 8 ) | report ),
			     interface, NULL, 0 );
}

/**
 * Set report
 *
 * @v usb		USB device
 * @v interface		Interface number
 * @v type		Report type
 * @v report		Report ID
 * @v data		Report data
 * @v len		Length of report data
 * @ret rc		Return status code
 */
static inline __attribute__ (( always_inline )) int
usbhid_set_report ( struct usb_device *usb, unsigned int interface,
		    unsigned int type, unsigned int report, void *data,
		    size_t len ) {

	return usb_control ( usb, USBHID_SET_REPORT, ( ( type << 8 ) | report ),
			     interface, data, len );
}

extern int usbhid_open ( struct usb_hid *hid );
extern void usbhid_close ( struct usb_hid *hid );
extern int usbhid_refill ( struct usb_hid *hid );
extern int usbhid_describe ( struct usb_hid *hid,
			     struct usb_configuration_descriptor *config );

#endif /* _IPXE_USBHID_H */

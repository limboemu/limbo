#ifndef _ACM_H
#define _ACM_H

/** @file
 *
 * USB RNDIS Ethernet driver
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/usb.h>
#include <ipxe/cdc.h>

/** CDC-ACM subclass */
#define USB_SUBCLASS_CDC_ACM 0x02

/** CDC-ACM RNDIS device protocol */
#define USB_PROTOCOL_ACM_RNDIS 0xff

/** Class code for wireless devices */
#define USB_CLASS_WIRELESS 0xe0

/** Radio frequency device subclass */
#define USB_SUBCLASS_WIRELESS_RADIO 0x01

/** Radio frequency RNDIS device protocol */
#define USB_PROTOCOL_RADIO_RNDIS 0x03

/** A USB RNDIS network device */
struct acm_device {
	/** USB device */
	struct usb_device *usb;
	/** USB bus */
	struct usb_bus *bus;
	/** RNDIS device */
	struct rndis_device *rndis;
	/** USB network device */
	struct usbnet_device usbnet;

	/** An encapsulated response is available */
	int responded;
};

/** Interrupt maximum fill level
 *
 * This is a policy decision.
 */
#define ACM_INTR_MAX_FILL 2

/** Bulk IN maximum fill level
 *
 * This is a policy decision.
 */
#define ACM_IN_MAX_FILL 8

/** Bulk IN buffer size
 *
 * This is a policy decision.
 */
#define ACM_IN_MTU 2048

/** Encapsulated response buffer size
 *
 * This is a policy decision.
 */
#define ACM_RESPONSE_MTU 128

#endif /* _ACM_H */

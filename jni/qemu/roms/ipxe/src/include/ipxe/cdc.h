#ifndef _IPXE_CDC_H
#define _IPXE_CDC_H

/** @file
 *
 * USB Communications Device Class (CDC)
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/usb.h>

/** Class code for communications devices */
#define USB_CLASS_CDC 2

/** Send encapsulated command */
#define CDC_SEND_ENCAPSULATED_COMMAND					\
	( USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE |		\
	  USB_REQUEST_TYPE ( 0x00 ) )

/** Get encapsulated response */
#define CDC_GET_ENCAPSULATED_RESPONSE					\
	( USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE |		\
	  USB_REQUEST_TYPE ( 0x01 ) )

/** Union functional descriptor */
struct cdc_union_descriptor {
	/** Descriptor header */
	struct usb_descriptor_header header;
	/** Descriptor subtype */
	uint8_t subtype;
	/** Interfaces (variable-length) */
	uint8_t interface[1];
} __attribute__ (( packed ));

/** Union functional descriptor subtype */
#define CDC_SUBTYPE_UNION 6

/** Ethernet descriptor subtype */
#define CDC_SUBTYPE_ETHERNET 15

/** Response available */
#define CDC_RESPONSE_AVAILABLE						\
	( USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE |		\
	  USB_REQUEST_TYPE ( 0x01 ) )

/** Network connection notification */
#define CDC_NETWORK_CONNECTION						\
	( USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE |		\
	  USB_REQUEST_TYPE ( 0x00 ) )

/** Connection speed change notification */
#define CDC_CONNECTION_SPEED_CHANGE					\
	( USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE |		\
	  USB_REQUEST_TYPE ( 0x2a ) )

/** Connection speed change notification */
struct cdc_connection_speed_change {
	/** Downlink bit rate, in bits per second */
	uint32_t down;
	/** Uplink bit rate, in bits per second */
	uint32_t up;
} __attribute__ (( packed ));

extern struct cdc_union_descriptor *
cdc_union_descriptor ( struct usb_configuration_descriptor *config,
		       struct usb_interface_descriptor *interface );

/**
 * Send encapsulated command
 *
 * @v usb		USB device
 * @v interface		Interface number
 * @v data		Command
 * @v len		Length of command
 * @ret rc		Return status code
 */
static inline __attribute__ (( always_inline )) int
cdc_send_encapsulated_command ( struct usb_device *usb, unsigned int interface,
				void *data, size_t len ) {

	return usb_control ( usb, CDC_SEND_ENCAPSULATED_COMMAND, 0, interface,
			     data, len );
}

/**
* Get encapsulated response
*
* @v usb		USB device
* @v interface		Interface number
* @v data		Response buffer
* @v len		Length of response buffer
* @ret rc		Return status code
*/
static inline __attribute__ (( always_inline )) int
cdc_get_encapsulated_response ( struct usb_device *usb, unsigned int interface,
				void *data, size_t len ) {

	return usb_control ( usb, CDC_GET_ENCAPSULATED_RESPONSE, 0, interface,
			     data, len );
}

#endif /* _IPXE_CDC_H */

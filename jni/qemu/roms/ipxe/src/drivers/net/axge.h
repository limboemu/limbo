#ifndef _AXGE_H
#define _AXGE_H

/** @file
 *
 * Asix 10/100/1000 USB Ethernet driver
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/usb.h>
#include <ipxe/usbnet.h>

/** Read MAC register */
#define AXGE_READ_MAC_REGISTER						\
	( USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE |		\
	  USB_REQUEST_TYPE ( 0x01 ) )

/** Write MAC register */
#define AXGE_WRITE_MAC_REGISTER						\
	( USB_DIR_OUT | USB_TYPE_VENDOR | USB_RECIP_DEVICE |		\
	  USB_REQUEST_TYPE ( 0x01 ) )

/** Physical Link Status Register */
#define AXGE_PLSR 0x02
#define AXGE_PLSR_EPHY_10		0x10	/**< Ethernet at 10Mbps */
#define AXGE_PLSR_EPHY_100		0x20	/**< Ethernet at 100Mbps */
#define AXGE_PLSR_EPHY_1000		0x40	/**< Ethernet at 1000Mbps */
#define AXGE_PLSR_EPHY_ANY						\
	( AXGE_PLSR_EPHY_10 |						\
	  AXGE_PLSR_EPHY_100 |						\
	  AXGE_PLSR_EPHY_1000 )

/** RX Control Register */
#define AXGE_RCR 0x0b
#define AXGE_RCR_PRO			0x0001	/**< Promiscuous mode */
#define AXGE_RCR_AMALL			0x0002	/**< Accept all multicasts */
#define AXGE_RCR_AB			0x0008	/**< Accept broadcasts */
#define AXGE_RCR_SO			0x0080	/**< Start operation */

/** Node ID Register */
#define AXGE_NIDR 0x10

/** Medium Status Register */
#define AXGE_MSR 0x22
#define AXGE_MSR_GM			0x0001	/**< Gigabit mode */
#define AXGE_MSR_FD			0x0002	/**< Full duplex */
#define AXGE_MSR_RFC			0x0010	/**< RX flow control enable */
#define AXGE_MSR_TFC			0x0020	/**< TX flow control enable */
#define AXGE_MSR_RE			0x0100	/**< Receive enable */

/** Ethernet PHY Power and Reset Control Register */
#define AXGE_EPPRCR 0x26
#define AXGE_EPPRCR_IPRL		0x0020	/**< Undocumented */

/** Delay after initialising EPPRCR */
#define AXGE_EPPRCR_DELAY_MS 200

/** Bulk IN Control Register (undocumented) */
#define AXGE_BICR 0x2e

/** Bulk IN Control (undocumented) */
struct axge_bulk_in_control {
	/** Control */
	uint8_t ctrl;
	/** Timer */
	uint16_t timer;
	/** Size */
	uint8_t size;
	/** Inter-frame gap */
	uint8_t ifg;
} __attribute__ (( packed ));

/** Clock Select Register (undocumented) */
#define AXGE_CSR 0x33
#define AXGE_CSR_BCS			0x01	/**< Undocumented */
#define AXGE_CSR_ACS			0x02	/**< Undocumented */

/** Delay after initialising CSR */
#define AXGE_CSR_DELAY_MS 100

/** Transmit packet header */
struct axge_tx_header {
	/** Packet length */
	uint32_t len;
	/** Answers on a postcard, please */
	uint32_t wtf;
} __attribute__ (( packed ));

/** Receive packet footer */
struct axge_rx_footer {
	/** Packet count */
	uint16_t count;
	/** Header offset */
	uint16_t offset;
} __attribute__ (( packed ));

/** Receive packet descriptor */
struct axge_rx_descriptor {
	/** Checksum information */
	uint16_t check;
	/** Length and error flags */
	uint16_t len_flags;
} __attribute__ (( packed ));

/** Receive packet length mask */
#define AXGE_RX_LEN_MASK 0x1fff

/** Receive packet length alignment */
#define AXGE_RX_LEN_PAD_ALIGN 8

/** Receive packet CRC error */
#define AXGE_RX_CRC_ERROR 0x2000

/** Receive packet dropped error */
#define AXGE_RX_DROP_ERROR 0x8000

/** Interrupt data */
struct axge_interrupt {
	/** Magic signature */
	uint16_t magic;
	/** Link state */
	uint16_t link;
	/** PHY register MR01 */
	uint16_t mr01;
	/** PHY register MR05 */
	uint16_t mr05;
} __attribute__ (( packed ));

/** Interrupt magic signature */
#define AXGE_INTR_MAGIC 0x00a1

/** Link is up */
#define AXGE_INTR_LINK_PPLS 0x0001

/** An AXGE network device */
struct axge_device {
	/** USB device */
	struct usb_device *usb;
	/** USB bus */
	struct usb_bus *bus;
	/** Network device */
	struct net_device *netdev;
	/** USB network device */
	struct usbnet_device usbnet;
};

/** Interrupt maximum fill level
 *
 * This is a policy decision.
 */
#define AXGE_INTR_MAX_FILL 2

/** Bulk IN maximum fill level
 *
 * This is a policy decision.
 */
#define AXGE_IN_MAX_FILL 8

/** Bulk IN buffer size
 *
 * This is a policy decision.
 */
#define AXGE_IN_MTU 2048

/** Amount of space to reserve at start of bulk IN buffers
 *
 * This is required to allow for protocols such as ARP which may reuse
 * a received I/O buffer for transmission.
 */
#define AXGE_IN_RESERVE sizeof ( struct axge_tx_header )

#endif /* _AXGE_H */

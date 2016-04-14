#ifndef	_IPXE_PCI_H
#define _IPXE_PCI_H

/*
 * Support for NE2000 PCI clones added David Monro June 1997
 * Generalised for other PCI NICs by Ken Yap July 1997
 * PCI support rewritten by Michael Brown 2006
 *
 * Most of this is taken from /usr/src/linux/include/linux/pci.h.
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2, or (at
 * your option) any later version.
 */

FILE_LICENCE ( GPL2_ONLY );

#include <stdint.h>
#include <ipxe/device.h>
#include <ipxe/tables.h>
#include <ipxe/pci_io.h>
#include "pci_ids.h"

/*
 * PCI constants
 *
 */

#define PCI_COMMAND_IO			0x1	/* Enable response in I/O space */
#define PCI_COMMAND_MEM			0x2	/* Enable response in mem space */
#define PCI_COMMAND_MASTER		0x4	/* Enable bus mastering */

#define PCI_CACHE_LINE_SIZE		0x0c	/* 8 bits */
#define PCI_LATENCY_TIMER		0x0d	/* 8 bits */

#define PCI_COMMAND_SPECIAL		0x8	/* Enable response to special cycles */
#define PCI_COMMAND_INVALIDATE		0x10	/* Use memory write and invalidate */
#define  PCI_COMMAND_VGA_PALETTE 0x20	/* Enable palette snooping */
#define  PCI_COMMAND_PARITY	0x40	/* Enable parity checking */
#define  PCI_COMMAND_WAIT 	0x80	/* Enable address/data stepping */
#define  PCI_COMMAND_SERR	0x100	/* Enable SERR */
#define  PCI_COMMAND_FAST_BACK	0x200	/* Enable back-to-back writes */
#define  PCI_COMMAND_INTX_DISABLE 0x400 /* INTx Emulation Disable */


#define PCI_VENDOR_ID           0x00    /* 16 bits */
#define PCI_DEVICE_ID           0x02    /* 16 bits */
#define PCI_COMMAND             0x04    /* 16 bits */

#define PCI_STATUS		0x06	/* 16 bits */
#define  PCI_STATUS_CAP_LIST	0x10	/* Support Capability List */
#define  PCI_STATUS_66MHZ	0x20	/* Support 66 Mhz PCI 2.1 bus */
#define  PCI_STATUS_UDF		0x40	/* Support User Definable Features [obsolete] */
#define  PCI_STATUS_FAST_BACK	0x80	/* Accept fast-back to back */
#define  PCI_STATUS_PARITY	0x100	/* Detected parity error */
#define  PCI_STATUS_DEVSEL_MASK	0x600	/* DEVSEL timing */
#define  PCI_STATUS_DEVSEL_FAST	0x000	
#define  PCI_STATUS_DEVSEL_MEDIUM 0x200
#define  PCI_STATUS_DEVSEL_SLOW 0x400
#define  PCI_STATUS_SIG_TARGET_ABORT 0x800 /* Set on target abort */
#define  PCI_STATUS_REC_TARGET_ABORT 0x1000 /* Master ack of " */
#define  PCI_STATUS_REC_MASTER_ABORT 0x2000 /* Set on master abort */
#define  PCI_STATUS_SIG_SYSTEM_ERROR 0x4000 /* Set when we drive SERR */
#define  PCI_STATUS_DETECTED_PARITY 0x8000 /* Set on parity error */

#define PCI_REVISION            0x08    /* 8 bits  */
#define PCI_REVISION_ID         0x08    /* 8 bits  */
#define PCI_CLASS_REVISION      0x08    /* 32 bits  */
#define PCI_CLASS_CODE          0x0b    /* 8 bits */
#define PCI_SUBCLASS_CODE       0x0a    /* 8 bits */
#define PCI_HEADER_TYPE         0x0e    /* 8 bits */
#define  PCI_HEADER_TYPE_NORMAL	0
#define  PCI_HEADER_TYPE_BRIDGE 1
#define  PCI_HEADER_TYPE_CARDBUS 2


/* Header type 0 (normal devices) */
#define PCI_CARDBUS_CIS		0x28
#define PCI_SUBSYSTEM_VENDOR_ID	0x2c
#define PCI_SUBSYSTEM_ID	0x2e  

#define PCI_BASE_ADDRESS_0      0x10    /* 32 bits */
#define PCI_BASE_ADDRESS_1      0x14    /* 32 bits */
#define PCI_BASE_ADDRESS_2      0x18    /* 32 bits */
#define PCI_BASE_ADDRESS_3      0x1c    /* 32 bits */
#define PCI_BASE_ADDRESS_4      0x20    /* 32 bits */
#define PCI_BASE_ADDRESS_5      0x24    /* 32 bits */

#define PCI_BASE_ADDRESS_SPACE		0x01    /* 0 = memory, 1 = I/O */
#define PCI_BASE_ADDRESS_SPACE_IO	0x01
#define PCI_BASE_ADDRESS_SPACE_MEMORY	0x00

#define PCI_BASE_ADDRESS_MEM_TYPE_MASK	0x06
#define PCI_BASE_ADDRESS_MEM_TYPE_32	0x00	/* 32 bit address */
#define PCI_BASE_ADDRESS_MEM_TYPE_1M	0x02	/* Below 1M [obsolete] */
#define PCI_BASE_ADDRESS_MEM_TYPE_64	0x04	/* 64 bit address */
#define	PCI_BASE_ADDRESS_MEM_MASK	(~0x0f)
#define	PCI_BASE_ADDRESS_IO_MASK	(~0x03)
#define	PCI_ROM_ADDRESS		0x30	/* 32 bits */
#define	PCI_ROM_ADDRESS_ENABLE	0x01	/* Write 1 to enable ROM,
					   bits 31..11 are address,
					   10..2 are reserved */

#define PCI_CAPABILITY_LIST	0x34	/* Offset of first capability list entry */

#define PCI_INTERRUPT_LINE	0x3c	/* IRQ number (0-15) */
#define PCI_INTERRUPT_PIN	0x3d	/* IRQ pin on PCI bus (A-D) */

/* Header type 1 (PCI-to-PCI bridges) */
#define PCI_PRIMARY_BUS		0x18	/* Primary bus number */
#define PCI_SECONDARY_BUS	0x19	/* Secondary bus number */
#define PCI_SUBORDINATE_BUS	0x1a	/* Highest bus number behind the bridge */
#define PCI_SEC_LATENCY_TIMER	0x1b	/* Latency timer for secondary interface */
#define PCI_IO_BASE		0x1c	/* I/O range behind the bridge */
#define PCI_IO_LIMIT		0x1d
#define  PCI_IO_RANGE_TYPE_MASK	0x0f	/* I/O bridging type */
#define  PCI_IO_RANGE_TYPE_16	0x00
#define  PCI_IO_RANGE_TYPE_32	0x01
#define  PCI_IO_RANGE_MASK	~0x0f
#define PCI_SEC_STATUS		0x1e	/* Secondary status register, only bit 14 used */
#define PCI_MEMORY_BASE		0x20	/* Memory range behind */
#define PCI_MEMORY_LIMIT	0x22
#define  PCI_MEMORY_RANGE_TYPE_MASK 0x0f
#define  PCI_MEMORY_RANGE_MASK	~0x0f
#define PCI_PREF_MEMORY_BASE	0x24	/* Prefetchable memory range behind */
#define PCI_PREF_MEMORY_LIMIT	0x26
#define  PCI_PREF_RANGE_TYPE_MASK 0x0f
#define  PCI_PREF_RANGE_TYPE_32	0x00
#define  PCI_PREF_RANGE_TYPE_64	0x01
#define  PCI_PREF_RANGE_MASK	~0x0f
#define PCI_PREF_BASE_UPPER32	0x28	/* Upper half of prefetchable memory range */
#define PCI_PREF_LIMIT_UPPER32	0x2c
#define PCI_IO_BASE_UPPER16	0x30	/* Upper half of I/O addresses */
#define PCI_IO_LIMIT_UPPER16	0x32
/* 0x34 same as for htype 0 */
/* 0x35-0x3b is reserved */
#define PCI_ROM_ADDRESS1	0x38	/* Same as PCI_ROM_ADDRESS, but for htype 1 */
/* 0x3c-0x3d are same as for htype 0 */
#define PCI_BRIDGE_CONTROL	0x3e
#define  PCI_BRIDGE_CTL_PARITY	0x01	/* Enable parity detection on secondary interface */
#define  PCI_BRIDGE_CTL_SERR	0x02	/* The same for SERR forwarding */
#define  PCI_BRIDGE_CTL_NO_ISA	0x04	/* Disable bridging of ISA ports */
#define  PCI_BRIDGE_CTL_VGA	0x08	/* Forward VGA addresses */
#define  PCI_BRIDGE_CTL_MASTER_ABORT 0x20  /* Report master aborts */
#define  PCI_BRIDGE_CTL_BUS_RESET 0x40	/* Secondary bus reset */
#define  PCI_BRIDGE_CTL_FAST_BACK 0x80	/* Fast Back2Back enabled on secondary interface */

#define PCI_CB_CAPABILITY_LIST	0x14

/* Capability lists */

#define PCI_CAP_LIST_ID		0	/* Capability ID */
#define  PCI_CAP_ID_PM		0x01	/* Power Management */
#define  PCI_CAP_ID_AGP		0x02	/* Accelerated Graphics Port */
#define  PCI_CAP_ID_VPD		0x03	/* Vital Product Data */
#define  PCI_CAP_ID_SLOTID	0x04	/* Slot Identification */
#define  PCI_CAP_ID_MSI		0x05	/* Message Signalled Interrupts */
#define  PCI_CAP_ID_CHSWP	0x06	/* CompactPCI HotSwap */
#define  PCI_CAP_ID_VNDR	0x09	/* Vendor specific */
#define  PCI_CAP_ID_EXP		0x10	/* PCI Express */
#define PCI_CAP_LIST_NEXT	1	/* Next capability in the list */
#define PCI_CAP_FLAGS		2	/* Capability defined flags (16 bits) */
#define PCI_CAP_SIZEOF		4

/* Power Management Registers */

#define PCI_PM_PMC              2       /* PM Capabilities Register */
#define  PCI_PM_CAP_VER_MASK	0x0007	/* Version */
#define  PCI_PM_CAP_PME_CLOCK	0x0008	/* PME clock required */
#define  PCI_PM_CAP_RESERVED    0x0010  /* Reserved field */
#define  PCI_PM_CAP_DSI		0x0020	/* Device specific initialization */
#define  PCI_PM_CAP_AUX_POWER	0x01C0	/* Auxiliary power support mask */
#define  PCI_PM_CAP_D1		0x0200	/* D1 power state support */
#define  PCI_PM_CAP_D2		0x0400	/* D2 power state support */
#define  PCI_PM_CAP_PME		0x0800	/* PME pin supported */
#define  PCI_PM_CAP_PME_MASK    0xF800  /* PME Mask of all supported states */
#define  PCI_PM_CAP_PME_D0      0x0800  /* PME# from D0 */
#define  PCI_PM_CAP_PME_D1      0x1000  /* PME# from D1 */
#define  PCI_PM_CAP_PME_D2      0x2000  /* PME# from D2 */
#define  PCI_PM_CAP_PME_D3      0x4000  /* PME# from D3 (hot) */
#define  PCI_PM_CAP_PME_D3cold  0x8000  /* PME# from D3 (cold) */
#define PCI_PM_CTRL		4	/* PM control and status register */
#define  PCI_PM_CTRL_STATE_MASK	0x0003	/* Current power state (D0 to D3) */
#define  PCI_PM_CTRL_PME_ENABLE	0x0100	/* PME pin enable */
#define  PCI_PM_CTRL_DATA_SEL_MASK	0x1e00	/* Data select (??) */
#define  PCI_PM_CTRL_DATA_SCALE_MASK	0x6000	/* Data scale (??) */
#define  PCI_PM_CTRL_PME_STATUS	0x8000	/* PME pin status */
#define PCI_PM_PPB_EXTENSIONS	6	/* PPB support extensions (??) */
#define  PCI_PM_PPB_B2_B3	0x40	/* Stop clock when in D3hot (??) */
#define  PCI_PM_BPCC_ENABLE	0x80	/* Bus power/clock control enable (??) */
#define PCI_PM_DATA_REGISTER	7	/* (??) */
#define PCI_PM_SIZEOF		8

/* AGP registers */

#define PCI_AGP_VERSION		2	/* BCD version number */
#define PCI_AGP_RFU		3	/* Rest of capability flags */
#define PCI_AGP_STATUS		4	/* Status register */
#define  PCI_AGP_STATUS_RQ_MASK	0xff000000	/* Maximum number of requests - 1 */
#define  PCI_AGP_STATUS_SBA	0x0200	/* Sideband addressing supported */
#define  PCI_AGP_STATUS_64BIT	0x0020	/* 64-bit addressing supported */
#define  PCI_AGP_STATUS_FW	0x0010	/* FW transfers supported */
#define  PCI_AGP_STATUS_RATE4	0x0004	/* 4x transfer rate supported */
#define  PCI_AGP_STATUS_RATE2	0x0002	/* 2x transfer rate supported */
#define  PCI_AGP_STATUS_RATE1	0x0001	/* 1x transfer rate supported */
#define PCI_AGP_COMMAND		8	/* Control register */
#define  PCI_AGP_COMMAND_RQ_MASK 0xff000000  /* Master: Maximum number of requests */
#define  PCI_AGP_COMMAND_SBA	0x0200	/* Sideband addressing enabled */
#define  PCI_AGP_COMMAND_AGP	0x0100	/* Allow processing of AGP transactions */
#define  PCI_AGP_COMMAND_64BIT	0x0020 	/* Allow processing of 64-bit addresses */
#define  PCI_AGP_COMMAND_FW	0x0010 	/* Force FW transfers */
#define  PCI_AGP_COMMAND_RATE4	0x0004	/* Use 4x rate */
#define  PCI_AGP_COMMAND_RATE2	0x0002	/* Use 2x rate */
#define  PCI_AGP_COMMAND_RATE1	0x0001	/* Use 1x rate */
#define PCI_AGP_SIZEOF		12

/* Slot Identification */

#define PCI_SID_ESR		2	/* Expansion Slot Register */
#define  PCI_SID_ESR_NSLOTS	0x1f	/* Number of expansion slots available */
#define  PCI_SID_ESR_FIC	0x20	/* First In Chassis Flag */
#define PCI_SID_CHASSIS_NR	3	/* Chassis Number */

/* Message Signalled Interrupts registers */

#define PCI_MSI_FLAGS		2	/* Various flags */
#define  PCI_MSI_FLAGS_64BIT	0x80	/* 64-bit addresses allowed */
#define  PCI_MSI_FLAGS_QSIZE	0x70	/* Message queue size configured */
#define  PCI_MSI_FLAGS_QMASK	0x0e	/* Maximum queue size available */
#define  PCI_MSI_FLAGS_ENABLE	0x01	/* MSI feature enabled */
#define PCI_MSI_RFU		3	/* Rest of capability flags */
#define PCI_MSI_ADDRESS_LO	4	/* Lower 32 bits */
#define PCI_MSI_ADDRESS_HI	8	/* Upper 32 bits (if PCI_MSI_FLAGS_64BIT set) */
#define PCI_MSI_DATA_32		8	/* 16 bits of data for 32-bit devices */
#define PCI_MSI_DATA_64		12	/* 16 bits of data for 64-bit devices */

/* Advanced Error Reporting */

#define PCI_ERR_UNCOR_STATUS	4	/* Uncorrectable Error Status */
#define  PCI_ERR_UNC_TRAIN	0x00000001	/* Training */
#define  PCI_ERR_UNC_DLP	0x00000010	/* Data Link Protocol */
#define  PCI_ERR_UNC_POISON_TLP	0x00001000	/* Poisoned TLP */
#define  PCI_ERR_UNC_FCP	0x00002000	/* Flow Control Protocol */
#define  PCI_ERR_UNC_COMP_TIME	0x00004000	/* Completion Timeout */
#define  PCI_ERR_UNC_COMP_ABORT	0x00008000	/* Completer Abort */
#define  PCI_ERR_UNC_UNX_COMP	0x00010000	/* Unexpected Completion */
#define  PCI_ERR_UNC_RX_OVER	0x00020000	/* Receiver Overflow */
#define  PCI_ERR_UNC_MALF_TLP	0x00040000	/* Malformed TLP */
#define  PCI_ERR_UNC_ECRC	0x00080000	/* ECRC Error Status */
#define  PCI_ERR_UNC_UNSUP	0x00100000	/* Unsupported Request */
#define PCI_ERR_UNCOR_MASK	8	/* Uncorrectable Error Mask */
	/* Same bits as above */
#define PCI_ERR_UNCOR_SEVER	12	/* Uncorrectable Error Severity */
	/* Same bits as above */
#define PCI_ERR_COR_STATUS	16	/* Correctable Error Status */
#define  PCI_ERR_COR_RCVR	0x00000001	/* Receiver Error Status */
#define  PCI_ERR_COR_BAD_TLP	0x00000040	/* Bad TLP Status */
#define  PCI_ERR_COR_BAD_DLLP	0x00000080	/* Bad DLLP Status */
#define  PCI_ERR_COR_REP_ROLL	0x00000100	/* REPLAY_NUM Rollover */
#define  PCI_ERR_COR_REP_TIMER	0x00001000	/* Replay Timer Timeout */
#define PCI_ERR_COR_MASK	20	/* Correctable Error Mask */
	/* Same bits as above */

/** A PCI device ID list entry */
struct pci_device_id {
	/** Name */
	const char *name;
	/** PCI vendor ID */
	uint16_t vendor;
	/** PCI device ID */
	uint16_t device;
	/** Arbitrary driver data */
	unsigned long driver_data;
};

/** Match-anything ID */
#define PCI_ANY_ID 0xffff

/** A PCI device */
struct pci_device {
	/** Generic device */
	struct device dev;
	/** Memory base
	 *
	 * This is the physical address of the first valid memory BAR.
	 */
	unsigned long membase;
	/**
	 * I/O address
	 *
	 * This is the physical address of the first valid I/O BAR.
	 */
	unsigned long ioaddr;
	/** Vendor ID */
	uint16_t vendor;
	/** Device ID */
	uint16_t device;
	/** Device class */
	uint32_t class;
	/** Interrupt number */
	uint8_t irq;
	/** Bus, device, and function (bus:dev.fn) number */
	uint16_t busdevfn;
	/** Driver for this device */
	struct pci_driver *driver;
	/** Driver-private data
	 *
	 * Use pci_set_drvdata() and pci_get_drvdata() to access this
	 * field.
	 */
	void *priv;
	/** Driver device ID */
	struct pci_device_id *id;
};

/** A PCI driver */
struct pci_driver {
	/** PCI ID table */
	struct pci_device_id *ids;
	/** Number of entries in PCI ID table */
	unsigned int id_count;
	/**
	 * Probe device
	 *
	 * @v pci	PCI device
	 * @ret rc	Return status code
	 */
	int ( * probe ) ( struct pci_device *pci );
	/**
	 * Remove device
	 *
	 * @v pci	PCI device
	 */
	void ( * remove ) ( struct pci_device *pci );
};

/** PCI driver table */
#define PCI_DRIVERS __table ( struct pci_driver, "pci_drivers" )

/** Declare a PCI driver */
#define __pci_driver __table_entry ( PCI_DRIVERS, 01 )

/** Declare a fallback PCI driver */
#define __pci_driver_fallback __table_entry ( PCI_DRIVERS, 02 )

#define PCI_BUS( busdevfn )		( ( (busdevfn) >> 8 ) & 0xff )
#define PCI_SLOT( busdevfn )		( ( (busdevfn) >> 3 ) & 0x1f )
#define PCI_FUNC( busdevfn )		( ( (busdevfn) >> 0 ) & 0x07 )
#define PCI_BUSDEVFN( bus, slot, func )	\
	( ( (bus) << 8 ) | ( (slot) << 3 ) | ( (func) << 0 ) )
#define PCI_FIRST_FUNC( busdevfn )	( (busdevfn) & ~0x07 )

#define PCI_BASE_CLASS( class )		( (class) >> 16 )
#define PCI_SUB_CLASS( class )		( ( (class) >> 8 ) & 0xff )
#define PCI_PROG_INTF( class )		( (class) & 0xff )

/*
 * PCI_ROM is used to build up entries in a struct pci_id array.  It
 * is also parsed by parserom.pl to generate Makefile rules and files
 * for rom-o-matic.
 *
 * PCI_ID can be used to generate entries without creating a
 * corresponding ROM in the build process.
 */
#define PCI_ID( _vendor, _device, _name, _description, _data ) {	\
	.vendor = _vendor,						\
	.device = _device,						\
	.name = _name,							\
	.driver_data = _data						\
}
#define PCI_ROM( _vendor, _device, _name, _description, _data ) \
	PCI_ID( _vendor, _device, _name, _description, _data )

/** PCI device debug message format */
#define PCI_FMT "PCI %02x:%02x.%x"

/** PCI device debug message arguments */
#define PCI_ARGS( pci )							\
	PCI_BUS ( (pci)->busdevfn ), PCI_SLOT ( (pci)->busdevfn ),	\
	PCI_FUNC ( (pci)->busdevfn )

extern void adjust_pci_device ( struct pci_device *pci );
extern unsigned long pci_bar_start ( struct pci_device *pci,
				     unsigned int reg );
extern int pci_read_config ( struct pci_device *pci );
extern int pci_find_next ( struct pci_device *pci, unsigned int busdevfn );
extern int pci_find_driver ( struct pci_device *pci );
extern int pci_probe ( struct pci_device *pci );
extern void pci_remove ( struct pci_device *pci );
extern int pci_find_capability ( struct pci_device *pci, int capability );
extern unsigned long pci_bar_size ( struct pci_device *pci, unsigned int reg );

/**
 * Initialise PCI device
 *
 * @v pci		PCI device
 * @v busdevfn		PCI bus:dev.fn address
 */
static inline void pci_init ( struct pci_device *pci, unsigned int busdevfn ) {
	pci->busdevfn = busdevfn;
}

/**
 * Set PCI driver
 *
 * @v pci		PCI device
 * @v driver		PCI driver
 * @v id		PCI device ID
 */
static inline void pci_set_driver ( struct pci_device *pci,
				    struct pci_driver *driver,
				    struct pci_device_id *id ) {
	pci->driver = driver;
	pci->id = id;
	pci->dev.driver_name = id->name;
}

/**
 * Set PCI driver-private data
 *
 * @v pci		PCI device
 * @v priv		Private data
 */
static inline void pci_set_drvdata ( struct pci_device *pci, void *priv ) {
	pci->priv = priv;
}

/**
 * Get PCI driver-private data
 *
 * @v pci		PCI device
 * @ret priv		Private data
 */
static inline void * pci_get_drvdata ( struct pci_device *pci ) {
	return pci->priv;
}

#endif	/* _IPXE_PCI_H */

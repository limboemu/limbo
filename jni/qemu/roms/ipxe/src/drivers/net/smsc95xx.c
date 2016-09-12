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

#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <byteswap.h>
#include <ipxe/ethernet.h>
#include <ipxe/usb.h>
#include <ipxe/usbnet.h>
#include <ipxe/profile.h>
#include <ipxe/base16.h>
#include <ipxe/smbios.h>
#include "smsc95xx.h"

/** @file
 *
 * SMSC LAN95xx USB Ethernet driver
 *
 */

/** Interrupt completion profiler */
static struct profiler smsc95xx_intr_profiler __profiler =
	{ .name = "smsc95xx.intr" };

/** Bulk IN completion profiler */
static struct profiler smsc95xx_in_profiler __profiler =
	{ .name = "smsc95xx.in" };

/** Bulk OUT profiler */
static struct profiler smsc95xx_out_profiler __profiler =
	{ .name = "smsc95xx.out" };

/******************************************************************************
 *
 * Register access
 *
 ******************************************************************************
 */

/**
 * Write register (without byte-swapping)
 *
 * @v smsc95xx		SMSC95xx device
 * @v address		Register address
 * @v value		Register value
 * @ret rc		Return status code
 */
static int smsc95xx_raw_writel ( struct smsc95xx_device *smsc95xx,
				 unsigned int address, uint32_t value ) {
	int rc;

	/* Write register */
	DBGCIO ( smsc95xx, "SMSC95XX %p [%03x] <= %08x\n",
		 smsc95xx, address, le32_to_cpu ( value ) );
	if ( ( rc = usb_control ( smsc95xx->usb, SMSC95XX_REGISTER_WRITE, 0,
				  address, &value, sizeof ( value ) ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not write %03x: %s\n",
		       smsc95xx, address, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Write register
 *
 * @v smsc95xx		SMSC95xx device
 * @v address		Register address
 * @v value		Register value
 * @ret rc		Return status code
 */
static inline __attribute__ (( always_inline )) int
smsc95xx_writel ( struct smsc95xx_device *smsc95xx, unsigned int address,
		  uint32_t value ) {
	int rc;

	/* Write register */
	if ( ( rc = smsc95xx_raw_writel ( smsc95xx, address,
					  cpu_to_le32 ( value ) ) ) != 0 )
		return rc;

	return 0;
}

/**
 * Read register (without byte-swapping)
 *
 * @v smsc95xx		SMSC95xx device
 * @v address		Register address
 * @ret value		Register value
 * @ret rc		Return status code
 */
static int smsc95xx_raw_readl ( struct smsc95xx_device *smsc95xx,
				unsigned int address, uint32_t *value ) {
	int rc;

	/* Read register */
	if ( ( rc = usb_control ( smsc95xx->usb, SMSC95XX_REGISTER_READ, 0,
				  address, value, sizeof ( *value ) ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not read %03x: %s\n",
		       smsc95xx, address, strerror ( rc ) );
		return rc;
	}
	DBGCIO ( smsc95xx, "SMSC95XX %p [%03x] => %08x\n",
		 smsc95xx, address, le32_to_cpu ( *value ) );

	return 0;
}

/**
 * Read register
 *
 * @v smsc95xx		SMSC95xx device
 * @v address		Register address
 * @ret value		Register value
 * @ret rc		Return status code
 */
static inline __attribute__ (( always_inline )) int
smsc95xx_readl ( struct smsc95xx_device *smsc95xx, unsigned int address,
		 uint32_t *value ) {
	int rc;

	/* Read register */
	if ( ( rc = smsc95xx_raw_readl ( smsc95xx, address, value ) ) != 0 )
		return rc;
	le32_to_cpus ( value );

	return 0;
}

/******************************************************************************
 *
 * EEPROM access
 *
 ******************************************************************************
 */

/**
 * Wait for EEPROM to become idle
 *
 * @v smsc95xx		SMSC95xx device
 * @ret rc		Return status code
 */
static int smsc95xx_eeprom_wait ( struct smsc95xx_device *smsc95xx ) {
	uint32_t e2p_cmd;
	unsigned int i;
	int rc;

	/* Wait for EPC_BSY to become clear */
	for ( i = 0 ; i < SMSC95XX_EEPROM_MAX_WAIT_MS ; i++ ) {

		/* Read E2P_CMD and check EPC_BSY */
		if ( ( rc = smsc95xx_readl ( smsc95xx, SMSC95XX_E2P_CMD,
					     &e2p_cmd ) ) != 0 )
			return rc;
		if ( ! ( e2p_cmd & SMSC95XX_E2P_CMD_EPC_BSY ) )
			return 0;

		/* Delay */
		mdelay ( 1 );
	}

	DBGC ( smsc95xx, "SMSC95XX %p timed out waiting for EEPROM\n",
	       smsc95xx );
	return -ETIMEDOUT;
}

/**
 * Read byte from EEPROM
 *
 * @v smsc95xx		SMSC95xx device
 * @v address		EEPROM address
 * @ret byte		Byte read, or negative error
 */
static int smsc95xx_eeprom_read_byte ( struct smsc95xx_device *smsc95xx,
				       unsigned int address ) {
	uint32_t e2p_cmd;
	uint32_t e2p_data;
	int rc;

	/* Wait for EEPROM to become idle */
	if ( ( rc = smsc95xx_eeprom_wait ( smsc95xx ) ) != 0 )
		return rc;

	/* Initiate read command */
	e2p_cmd = ( SMSC95XX_E2P_CMD_EPC_BSY | SMSC95XX_E2P_CMD_EPC_CMD_READ |
		    SMSC95XX_E2P_CMD_EPC_ADDR ( address ) );
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_E2P_CMD,
				      e2p_cmd ) ) != 0 )
		return rc;

	/* Wait for command to complete */
	if ( ( rc = smsc95xx_eeprom_wait ( smsc95xx ) ) != 0 )
		return rc;

	/* Read EEPROM data */
	if ( ( rc = smsc95xx_readl ( smsc95xx, SMSC95XX_E2P_DATA,
				     &e2p_data ) ) != 0 )
		return rc;

	return SMSC95XX_E2P_DATA_GET ( e2p_data );
}

/**
 * Read data from EEPROM
 *
 * @v smsc95xx		SMSC95xx device
 * @v address		EEPROM address
 * @v data		Data buffer
 * @v len		Length of data
 * @ret rc		Return status code
 */
static int smsc95xx_eeprom_read ( struct smsc95xx_device *smsc95xx,
				  unsigned int address, void *data,
				  size_t len ) {
	uint8_t *bytes;
	int byte;

	/* Read bytes */
	for ( bytes = data ; len-- ; address++, bytes++ ) {
		byte = smsc95xx_eeprom_read_byte ( smsc95xx, address );
		if ( byte < 0 )
			return byte;
		*bytes = byte;
	}

	return 0;
}

/******************************************************************************
 *
 * MAC address
 *
 ******************************************************************************
 */

/**
 * Fetch MAC address from EEPROM
 *
 * @v smsc95xx		SMSC95xx device
 * @v hw_addr		Hardware address to fill in
 * @ret rc		Return status code
 */
static int smsc95xx_fetch_mac_eeprom ( struct smsc95xx_device *smsc95xx,
				       uint8_t *hw_addr ) {
	int rc;

	/* Read MAC address from EEPROM */
	if ( ( rc = smsc95xx_eeprom_read ( smsc95xx, SMSC95XX_EEPROM_MAC,
					   hw_addr, ETH_ALEN ) ) != 0 )
		return rc;

	/* Check that EEPROM is physically present */
	if ( ! is_valid_ether_addr ( hw_addr ) ) {
		DBGC ( smsc95xx, "SMSC95XX %p has no EEPROM (%s)\n",
		       smsc95xx, eth_ntoa ( hw_addr ) );
		return -ENODEV;
	}

	DBGC ( smsc95xx, "SMSC95XX %p using EEPROM MAC %s\n",
	       smsc95xx, eth_ntoa ( hw_addr ) );
	return 0;
}

/**
 * Construct MAC address for Honeywell VM3
 *
 * @v smsc95xx		SMSC95xx device
 * @v hw_addr		Hardware address to fill in
 * @ret rc		Return status code
 */
static int smsc95xx_fetch_mac_vm3 ( struct smsc95xx_device *smsc95xx,
				    uint8_t *hw_addr ) {
	struct smbios_structure structure;
	struct smbios_system_information system;
	struct {
		char manufacturer[ 10 /* "Honeywell" + NUL */ ];
		char product[ 4 /* "VM3" + NUL */ ];
		char mac[ base16_encoded_len ( ETH_ALEN ) + 1 /* NUL */ ];
	} strings;
	int len;
	int rc;

	/* Find system information */
	if ( ( rc = find_smbios_structure ( SMBIOS_TYPE_SYSTEM_INFORMATION, 0,
					    &structure ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not find system "
		       "information: %s\n", smsc95xx, strerror ( rc ) );
		return rc;
	}

	/* Read system information */
	if ( ( rc = read_smbios_structure ( &structure, &system,
					    sizeof ( system ) ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not read system "
		       "information: %s\n", smsc95xx, strerror ( rc ) );
		return rc;
	}

	/* NUL-terminate all strings to be fetched */
	memset ( &strings, 0, sizeof ( strings ) );

	/* Fetch system manufacturer name */
	len = read_smbios_string ( &structure, system.manufacturer,
				   strings.manufacturer,
				   ( sizeof ( strings.manufacturer ) - 1 ) );
	if ( len < 0 ) {
		rc = len;
		DBGC ( smsc95xx, "SMSC95XX %p could not read manufacturer "
		       "name: %s\n", smsc95xx, strerror ( rc ) );
		return rc;
	}

	/* Fetch system product name */
	len = read_smbios_string ( &structure, system.product, strings.product,
				   ( sizeof ( strings.product ) - 1 ) );
	if ( len < 0 ) {
		rc = len;
		DBGC ( smsc95xx, "SMSC95XX %p could not read product name: "
		       "%s\n", smsc95xx, strerror ( rc ) );
		return rc;
	}

	/* Ignore non-VM3 devices */
	if ( ( strcmp ( strings.manufacturer, "Honeywell" ) != 0 ) ||
	     ( strcmp ( strings.product, "VM3" ) != 0 ) )
		return -ENOTTY;

	/* Find OEM strings */
	if ( ( rc = find_smbios_structure ( SMBIOS_TYPE_OEM_STRINGS, 0,
					    &structure ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not find OEM strings: %s\n",
		       smsc95xx, strerror ( rc ) );
		return rc;
	}

	/* Fetch MAC address */
	len = read_smbios_string ( &structure, SMSC95XX_VM3_OEM_STRING_MAC,
				   strings.mac, ( sizeof ( strings.mac ) - 1 ));
	if ( len < 0 ) {
		rc = len;
		DBGC ( smsc95xx, "SMSC95XX %p could not read OEM string: %s\n",
		       smsc95xx, strerror ( rc ) );
		return rc;
	}

	/* Sanity check */
	if ( len != ( ( int ) ( sizeof ( strings.mac ) - 1 ) ) ) {
		DBGC ( smsc95xx, "SMSC95XX %p invalid MAC address \"%s\"\n",
		       smsc95xx, strings.mac );
		return -EINVAL;
	}

	/* Decode MAC address */
	len = base16_decode ( strings.mac, hw_addr, ETH_ALEN );
	if ( len < 0 ) {
		rc = len;
		DBGC ( smsc95xx, "SMSC95XX %p invalid MAC address \"%s\"\n",
		       smsc95xx, strings.mac );
		return rc;
	}

	DBGC ( smsc95xx, "SMSC95XX %p using VM3 MAC %s\n",
	       smsc95xx, eth_ntoa ( hw_addr ) );
	return 0;
}

/**
 * Fetch MAC address
 *
 * @v smsc95xx		SMSC95xx device
 * @v hw_addr		Hardware address to fill in
 * @ret rc		Return status code
 */
static int smsc95xx_fetch_mac ( struct smsc95xx_device *smsc95xx,
				uint8_t *hw_addr ) {
	int rc;

	/* Read MAC address from EEPROM, if present */
	if ( ( rc = smsc95xx_fetch_mac_eeprom ( smsc95xx, hw_addr ) ) == 0 )
		return 0;

	/* Construct MAC address for Honeywell VM3, if applicable */
	if ( ( rc = smsc95xx_fetch_mac_vm3 ( smsc95xx, hw_addr ) ) == 0 )
		return 0;

	/* Otherwise, generate a random MAC address */
	eth_random_addr ( hw_addr );
	DBGC ( smsc95xx, "SMSC95XX %p using random MAC %s\n",
	       smsc95xx, eth_ntoa ( hw_addr ) );
	return 0;
}

/******************************************************************************
 *
 * MII access
 *
 ******************************************************************************
 */

/**
 * Wait for MII to become idle
 *
 * @v smsc95xx		SMSC95xx device
 * @ret rc		Return status code
 */
static int smsc95xx_mii_wait ( struct smsc95xx_device *smsc95xx ) {
	uint32_t mii_access;
	unsigned int i;
	int rc;

	/* Wait for MIIBZY to become clear */
	for ( i = 0 ; i < SMSC95XX_MII_MAX_WAIT_MS ; i++ ) {

		/* Read MII_ACCESS and check MIIBZY */
		if ( ( rc = smsc95xx_readl ( smsc95xx, SMSC95XX_MII_ACCESS,
					     &mii_access ) ) != 0 )
			return rc;
		if ( ! ( mii_access & SMSC95XX_MII_ACCESS_MIIBZY ) )
			return 0;

		/* Delay */
		mdelay ( 1 );
	}

	DBGC ( smsc95xx, "SMSC95XX %p timed out waiting for MII\n",
	       smsc95xx );
	return -ETIMEDOUT;
}

/**
 * Read from MII register
 *
 * @v mii		MII interface
 * @v reg		Register address
 * @ret value		Data read, or negative error
 */
static int smsc95xx_mii_read ( struct mii_interface *mii, unsigned int reg ) {
	struct smsc95xx_device *smsc95xx =
		container_of ( mii, struct smsc95xx_device, mii );
	uint32_t mii_access;
	uint32_t mii_data;
	int rc;

	/* Wait for MII to become idle */
	if ( ( rc = smsc95xx_mii_wait ( smsc95xx ) ) != 0 )
		return rc;

	/* Initiate read command */
	mii_access = ( SMSC95XX_MII_ACCESS_PHY_ADDRESS |
		       SMSC95XX_MII_ACCESS_MIIRINDA ( reg ) |
		       SMSC95XX_MII_ACCESS_MIIBZY );
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_MII_ACCESS,
				      mii_access ) ) != 0 )
		return rc;

	/* Wait for command to complete */
	if ( ( rc = smsc95xx_mii_wait ( smsc95xx ) ) != 0 )
		return rc;

	/* Read MII data */
	if ( ( rc = smsc95xx_readl ( smsc95xx, SMSC95XX_MII_DATA,
				     &mii_data ) ) != 0 )
		return rc;

	return SMSC95XX_MII_DATA_GET ( mii_data );
}

/**
 * Write to MII register
 *
 * @v mii		MII interface
 * @v reg		Register address
 * @v data		Data to write
 * @ret rc		Return status code
 */
static int smsc95xx_mii_write ( struct mii_interface *mii, unsigned int reg,
				unsigned int data ) {
	struct smsc95xx_device *smsc95xx =
		container_of ( mii, struct smsc95xx_device, mii );
	uint32_t mii_access;
	uint32_t mii_data;
	int rc;

	/* Wait for MII to become idle */
	if ( ( rc = smsc95xx_mii_wait ( smsc95xx ) ) != 0 )
		return rc;

	/* Write MII data */
	mii_data = SMSC95XX_MII_DATA_SET ( data );
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_MII_DATA,
				      mii_data ) ) != 0 )
		return rc;

	/* Initiate write command */
	mii_access = ( SMSC95XX_MII_ACCESS_PHY_ADDRESS |
		       SMSC95XX_MII_ACCESS_MIIRINDA ( reg ) |
		       SMSC95XX_MII_ACCESS_MIIWNR |
		       SMSC95XX_MII_ACCESS_MIIBZY );
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_MII_ACCESS,
				      mii_access ) ) != 0 )
		return rc;

	/* Wait for command to complete */
	if ( ( rc = smsc95xx_mii_wait ( smsc95xx ) ) != 0 )
		return rc;

	return 0;
}

/** MII operations */
static struct mii_operations smsc95xx_mii_operations = {
	.read = smsc95xx_mii_read,
	.write = smsc95xx_mii_write,
};

/**
 * Check link status
 *
 * @v smsc95xx		SMSC95xx device
 * @ret rc		Return status code
 */
static int smsc95xx_check_link ( struct smsc95xx_device *smsc95xx ) {
	struct net_device *netdev = smsc95xx->netdev;
	int intr;
	int rc;

	/* Read PHY interrupt source */
	intr = mii_read ( &smsc95xx->mii, SMSC95XX_MII_PHY_INTR_SOURCE );
	if ( intr < 0 ) {
		rc = intr;
		DBGC ( smsc95xx, "SMSC95XX %p could not get PHY interrupt "
		       "source: %s\n", smsc95xx, strerror ( rc ) );
		return rc;
	}

	/* Acknowledge PHY interrupt */
	if ( ( rc = mii_write ( &smsc95xx->mii, SMSC95XX_MII_PHY_INTR_SOURCE,
				intr ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not acknowledge PHY "
		       "interrupt: %s\n", smsc95xx, strerror ( rc ) );
		return rc;
	}

	/* Check link status */
	if ( ( rc = mii_check_link ( &smsc95xx->mii, netdev ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not check link: %s\n",
		       smsc95xx, strerror ( rc ) );
		return rc;
	}

	DBGC ( smsc95xx, "SMSC95XX %p link %s (intr %#04x)\n",
	       smsc95xx, ( netdev_link_ok ( netdev ) ? "up" : "down" ), intr );
	return 0;
}

/******************************************************************************
 *
 * Statistics (for debugging)
 *
 ******************************************************************************
 */

/**
 * Get RX statistics
 *
 * @v smsc95xx		SMSC95xx device
 * @v stats		Statistics to fill in
 * @ret rc		Return status code
 */
static int smsc95xx_get_rx_statistics ( struct smsc95xx_device *smsc95xx,
					struct smsc95xx_rx_statistics *stats ) {
	int rc;

	/* Get statistics */
	if ( ( rc = usb_control ( smsc95xx->usb, SMSC95XX_GET_STATISTICS, 0,
				  SMSC95XX_RX_STATISTICS, stats,
				  sizeof ( *stats ) ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not get RX statistics: "
		       "%s\n", smsc95xx, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Get TX statistics
 *
 * @v smsc95xx		SMSC95xx device
 * @v stats		Statistics to fill in
 * @ret rc		Return status code
 */
static int smsc95xx_get_tx_statistics ( struct smsc95xx_device *smsc95xx,
					struct smsc95xx_tx_statistics *stats ) {
	int rc;

	/* Get statistics */
	if ( ( rc = usb_control ( smsc95xx->usb, SMSC95XX_GET_STATISTICS, 0,
				  SMSC95XX_TX_STATISTICS, stats,
				  sizeof ( *stats ) ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not get TX statistics: "
		       "%s\n", smsc95xx, strerror ( rc ) );
		return rc;
	}

	return 0;
}

/**
 * Dump statistics (for debugging)
 *
 * @v smsc95xx		SMSC95xx device
 * @ret rc		Return status code
 */
static int smsc95xx_dump_statistics ( struct smsc95xx_device *smsc95xx ) {
	struct smsc95xx_rx_statistics rx;
	struct smsc95xx_tx_statistics tx;
	int rc;

	/* Do nothing unless debugging is enabled */
	if ( ! DBG_LOG )
		return 0;

	/* Get RX statistics */
	if ( ( rc = smsc95xx_get_rx_statistics ( smsc95xx, &rx ) ) != 0 )
		return rc;

	/* Get TX statistics */
	if ( ( rc = smsc95xx_get_tx_statistics ( smsc95xx, &tx ) ) != 0 )
		return rc;

	/* Dump statistics */
	DBGC ( smsc95xx, "SMSC95XX %p RX good %d bad %d crc %d und %d aln %d "
	       "ovr %d lat %d drp %d\n", smsc95xx, le32_to_cpu ( rx.good ),
	       le32_to_cpu ( rx.bad ), le32_to_cpu ( rx.crc ),
	       le32_to_cpu ( rx.undersize ), le32_to_cpu ( rx.alignment ),
	       le32_to_cpu ( rx.oversize ), le32_to_cpu ( rx.late ),
	       le32_to_cpu ( rx.dropped ) );
	DBGC ( smsc95xx, "SMSC95XX %p TX good %d bad %d pau %d sgl %d mul %d "
	       "exc %d lat %d und %d def %d car %d\n", smsc95xx,
	       le32_to_cpu ( tx.good ), le32_to_cpu ( tx.bad ),
	       le32_to_cpu ( tx.pause ), le32_to_cpu ( tx.single ),
	       le32_to_cpu ( tx.multiple ), le32_to_cpu ( tx.excessive ),
	       le32_to_cpu ( tx.late ), le32_to_cpu ( tx.underrun ),
	       le32_to_cpu ( tx.deferred ), le32_to_cpu ( tx.carrier ) );

	return 0;
}

/******************************************************************************
 *
 * Device reset
 *
 ******************************************************************************
 */

/**
 * Reset device
 *
 * @v smsc95xx		SMSC95xx device
 * @ret rc		Return status code
 */
static int smsc95xx_reset ( struct smsc95xx_device *smsc95xx ) {
	uint32_t hw_cfg;
	uint32_t led_gpio_cfg;
	int rc;

	/* Reset device */
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_HW_CFG,
				      SMSC95XX_HW_CFG_LRST ) ) != 0 )
		return rc;

	/* Wait for reset to complete */
	udelay ( SMSC95XX_RESET_DELAY_US );

	/* Check that reset has completed */
	if ( ( rc = smsc95xx_readl ( smsc95xx, SMSC95XX_HW_CFG,
				     &hw_cfg ) ) != 0 )
		return rc;
	if ( hw_cfg & SMSC95XX_HW_CFG_LRST ) {
		DBGC ( smsc95xx, "SMSC95XX %p failed to reset\n", smsc95xx );
		return -ETIMEDOUT;
	}

	/* Configure LEDs */
	led_gpio_cfg = ( SMSC95XX_LED_GPIO_CFG_GPCTL2_NSPD_LED |
			 SMSC95XX_LED_GPIO_CFG_GPCTL1_NLNKA_LED |
			 SMSC95XX_LED_GPIO_CFG_GPCTL0_NFDX_LED );
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_LED_GPIO_CFG,
				      led_gpio_cfg ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not configure LEDs: %s\n",
		       smsc95xx, strerror ( rc ) );
		/* Ignore error and continue */
	}

	return 0;
}

/******************************************************************************
 *
 * Endpoint operations
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
static void smsc95xx_intr_complete ( struct usb_endpoint *ep,
				     struct io_buffer *iobuf, int rc ) {
	struct smsc95xx_device *smsc95xx =
		container_of ( ep, struct smsc95xx_device, usbnet.intr );
	struct net_device *netdev = smsc95xx->netdev;
	struct smsc95xx_interrupt *intr;

	/* Profile completions */
	profile_start ( &smsc95xx_intr_profiler );

	/* Ignore packets cancelled when the endpoint closes */
	if ( ! ep->open )
		goto done;

	/* Record USB errors against the network device */
	if ( rc != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p interrupt failed: %s\n",
		       smsc95xx, strerror ( rc ) );
		DBGC_HDA ( smsc95xx, 0, iobuf->data, iob_len ( iobuf ) );
		netdev_rx_err ( netdev, NULL, rc );
		goto done;
	}

	/* Extract interrupt data */
	if ( iob_len ( iobuf ) != sizeof ( *intr ) ) {
		DBGC ( smsc95xx, "SMSC95XX %p malformed interrupt\n",
		       smsc95xx );
		DBGC_HDA ( smsc95xx, 0, iobuf->data, iob_len ( iobuf ) );
		netdev_rx_err ( netdev, NULL, rc );
		goto done;
	}
	intr = iobuf->data;

	/* Record interrupt status */
	smsc95xx->int_sts = le32_to_cpu ( intr->int_sts );
	profile_stop ( &smsc95xx_intr_profiler );

 done:
	/* Free I/O buffer */
	free_iob ( iobuf );
}

/** Interrupt endpoint operations */
static struct usb_endpoint_driver_operations smsc95xx_intr_operations = {
	.complete = smsc95xx_intr_complete,
};

/**
 * Complete bulk IN transfer
 *
 * @v ep		USB endpoint
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void smsc95xx_in_complete ( struct usb_endpoint *ep,
				   struct io_buffer *iobuf, int rc ) {
	struct smsc95xx_device *smsc95xx =
		container_of ( ep, struct smsc95xx_device, usbnet.in );
	struct net_device *netdev = smsc95xx->netdev;
	struct smsc95xx_rx_header *header;

	/* Profile completions */
	profile_start ( &smsc95xx_in_profiler );

	/* Ignore packets cancelled when the endpoint closes */
	if ( ! ep->open ) {
		free_iob ( iobuf );
		return;
	}

	/* Record USB errors against the network device */
	if ( rc != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p bulk IN failed: %s\n",
		       smsc95xx, strerror ( rc ) );
		goto err;
	}

	/* Sanity check */
	if ( iob_len ( iobuf ) < ( sizeof ( *header ) + 4 /* CRC */ ) ) {
		DBGC ( smsc95xx, "SMSC95XX %p underlength bulk IN\n",
		       smsc95xx );
		DBGC_HDA ( smsc95xx, 0, iobuf->data, iob_len ( iobuf ) );
		rc = -EINVAL;
		goto err;
	}

	/* Strip header and CRC */
	header = iobuf->data;
	iob_pull ( iobuf, sizeof ( *header ) );
	iob_unput ( iobuf, 4 /* CRC */ );

	/* Check for errors */
	if ( header->command & cpu_to_le32 ( SMSC95XX_RX_RUNT |
					     SMSC95XX_RX_LATE |
					     SMSC95XX_RX_CRC ) ) {
		DBGC ( smsc95xx, "SMSC95XX %p receive error (%08x):\n",
		       smsc95xx, le32_to_cpu ( header->command ) );
		DBGC_HDA ( smsc95xx, 0, iobuf->data, iob_len ( iobuf ) );
		rc = -EIO;
		goto err;
	}

	/* Hand off to network stack */
	netdev_rx ( netdev, iob_disown ( iobuf ) );

	profile_stop ( &smsc95xx_in_profiler );
	return;

 err:
	/* Hand off to network stack */
	netdev_rx_err ( netdev, iob_disown ( iobuf ), rc );
}

/** Bulk IN endpoint operations */
static struct usb_endpoint_driver_operations smsc95xx_in_operations = {
	.complete = smsc95xx_in_complete,
};

/**
 * Transmit packet
 *
 * @v smsc95xx		SMSC95xx device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int smsc95xx_out_transmit ( struct smsc95xx_device *smsc95xx,
				   struct io_buffer *iobuf ) {
	struct smsc95xx_tx_header *header;
	size_t len = iob_len ( iobuf );
	int rc;

	/* Profile transmissions */
	profile_start ( &smsc95xx_out_profiler );

	/* Prepend header */
	if ( ( rc = iob_ensure_headroom ( iobuf, sizeof ( *header ) ) ) != 0 )
		return rc;
	header = iob_push ( iobuf, sizeof ( *header ) );
	header->command = cpu_to_le32 ( SMSC95XX_TX_FIRST | SMSC95XX_TX_LAST |
					SMSC95XX_TX_LEN ( len ) );
	header->len = cpu_to_le32 ( len );

	/* Enqueue I/O buffer */
	if ( ( rc = usb_stream ( &smsc95xx->usbnet.out, iobuf, 0 ) ) != 0 )
		return rc;

	profile_stop ( &smsc95xx_out_profiler );
	return 0;
}

/**
 * Complete bulk OUT transfer
 *
 * @v ep		USB endpoint
 * @v iobuf		I/O buffer
 * @v rc		Completion status code
 */
static void smsc95xx_out_complete ( struct usb_endpoint *ep,
				    struct io_buffer *iobuf, int rc ) {
	struct smsc95xx_device *smsc95xx =
		container_of ( ep, struct smsc95xx_device, usbnet.out );
	struct net_device *netdev = smsc95xx->netdev;

	/* Report TX completion */
	netdev_tx_complete_err ( netdev, iobuf, rc );
}

/** Bulk OUT endpoint operations */
static struct usb_endpoint_driver_operations smsc95xx_out_operations = {
	.complete = smsc95xx_out_complete,
};

/******************************************************************************
 *
 * Network device interface
 *
 ******************************************************************************
 */

/**
 * Open network device
 *
 * @v netdev		Network device
 * @ret rc		Return status code
 */
static int smsc95xx_open ( struct net_device *netdev ) {
	struct smsc95xx_device *smsc95xx = netdev->priv;
	union smsc95xx_mac mac;
	int rc;

	/* Clear stored interrupt status */
	smsc95xx->int_sts = 0;

	/* Copy MAC address */
	memset ( &mac, 0, sizeof ( mac ) );
	memcpy ( mac.raw, netdev->ll_addr, ETH_ALEN );

	/* Configure bulk IN empty response */
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_HW_CFG,
				      SMSC95XX_HW_CFG_BIR ) ) != 0 )
		goto err_hw_cfg;

	/* Open USB network device */
	if ( ( rc = usbnet_open ( &smsc95xx->usbnet ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not open: %s\n",
		       smsc95xx, strerror ( rc ) );
		goto err_open;
	}

	/* Configure interrupt endpoint */
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_INT_EP_CTL,
				      ( SMSC95XX_INT_EP_CTL_RXDF_EN |
					SMSC95XX_INT_EP_CTL_PHY_EN ) ) ) != 0 )
		goto err_int_ep_ctl;

	/* Configure bulk IN delay */
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_BULK_IN_DLY,
				      SMSC95XX_BULK_IN_DLY_SET ( 0 ) ) ) != 0 )
		goto err_bulk_in_dly;

	/* Configure MAC */
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_MAC_CR,
				      ( SMSC95XX_MAC_CR_RXALL |
					SMSC95XX_MAC_CR_FDPX |
					SMSC95XX_MAC_CR_MCPAS |
					SMSC95XX_MAC_CR_PRMS |
					SMSC95XX_MAC_CR_PASSBAD |
					SMSC95XX_MAC_CR_TXEN |
					SMSC95XX_MAC_CR_RXEN ) ) ) != 0 )
		goto err_mac_cr;

	/* Configure transmit datapath */
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_TX_CFG,
				      SMSC95XX_TX_CFG_ON ) ) != 0 )
		goto err_tx_cfg;

	/* Write MAC address high register */
	if ( ( rc = smsc95xx_raw_writel ( smsc95xx, SMSC95XX_ADDRH,
					  mac.addr.h ) ) != 0 )
		goto err_addrh;

	/* Write MAC address low register */
	if ( ( rc = smsc95xx_raw_writel ( smsc95xx, SMSC95XX_ADDRL,
					  mac.addr.l ) ) != 0 )
		goto err_addrl;

	/* Enable PHY interrupts */
	if ( ( rc = mii_write ( &smsc95xx->mii, SMSC95XX_MII_PHY_INTR_MASK,
				( SMSC95XX_PHY_INTR_ANEG_DONE |
				  SMSC95XX_PHY_INTR_LINK_DOWN ) ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not set PHY interrupt "
		       "mask: %s\n", smsc95xx, strerror ( rc ) );
		goto err_phy_intr_mask;
	}

	/* Update link status */
	smsc95xx_check_link ( smsc95xx );

	return 0;

 err_phy_intr_mask:
 err_addrl:
 err_addrh:
 err_tx_cfg:
 err_mac_cr:
 err_bulk_in_dly:
 err_int_ep_ctl:
	usbnet_close ( &smsc95xx->usbnet );
 err_open:
 err_hw_cfg:
	smsc95xx_reset ( smsc95xx );
	return rc;
}

/**
 * Close network device
 *
 * @v netdev		Network device
 */
static void smsc95xx_close ( struct net_device *netdev ) {
	struct smsc95xx_device *smsc95xx = netdev->priv;

	/* Close USB network device */
	usbnet_close ( &smsc95xx->usbnet );

	/* Dump statistics (for debugging) */
	smsc95xx_dump_statistics ( smsc95xx );

	/* Reset device */
	smsc95xx_reset ( smsc95xx );
}

/**
 * Transmit packet
 *
 * @v netdev		Network device
 * @v iobuf		I/O buffer
 * @ret rc		Return status code
 */
static int smsc95xx_transmit ( struct net_device *netdev,
			       struct io_buffer *iobuf ) {
	struct smsc95xx_device *smsc95xx = netdev->priv;
	int rc;

	/* Transmit packet */
	if ( ( rc = smsc95xx_out_transmit ( smsc95xx, iobuf ) ) != 0 )
		return rc;

	return 0;
}

/**
 * Poll for completed and received packets
 *
 * @v netdev		Network device
 */
static void smsc95xx_poll ( struct net_device *netdev ) {
	struct smsc95xx_device *smsc95xx = netdev->priv;
	uint32_t int_sts;
	int rc;

	/* Poll USB bus */
	usb_poll ( smsc95xx->bus );

	/* Refill endpoints */
	if ( ( rc = usbnet_refill ( &smsc95xx->usbnet ) ) != 0 )
		netdev_rx_err ( netdev, NULL, rc );

	/* Do nothing more unless there are interrupts to handle */
	int_sts = smsc95xx->int_sts;
	if ( ! int_sts )
		return;

	/* Check link status if applicable */
	if ( int_sts & SMSC95XX_INT_STS_PHY_INT ) {
		smsc95xx_check_link ( smsc95xx );
		int_sts &= ~SMSC95XX_INT_STS_PHY_INT;
	}

	/* Record RX FIFO overflow if applicable */
	if ( int_sts & SMSC95XX_INT_STS_RXDF_INT ) {
		DBGC2 ( smsc95xx, "SMSC95XX %p RX FIFO overflowed\n",
			smsc95xx );
		netdev_rx_err ( netdev, NULL, -ENOBUFS );
		int_sts &= ~SMSC95XX_INT_STS_RXDF_INT;
	}

	/* Check for unexpected interrupts */
	if ( int_sts ) {
		DBGC ( smsc95xx, "SMSC95XX %p unexpected interrupt %#08x\n",
		       smsc95xx, int_sts );
		netdev_rx_err ( netdev, NULL, -ENOTTY );
	}

	/* Clear interrupts */
	if ( ( rc = smsc95xx_writel ( smsc95xx, SMSC95XX_INT_STS,
				      smsc95xx->int_sts ) ) != 0 )
		netdev_rx_err ( netdev, NULL, rc );
	smsc95xx->int_sts = 0;
}

/** SMSC95xx network device operations */
static struct net_device_operations smsc95xx_operations = {
	.open		= smsc95xx_open,
	.close		= smsc95xx_close,
	.transmit	= smsc95xx_transmit,
	.poll		= smsc95xx_poll,
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
static int smsc95xx_probe ( struct usb_function *func,
			    struct usb_configuration_descriptor *config ) {
	struct usb_device *usb = func->usb;
	struct net_device *netdev;
	struct smsc95xx_device *smsc95xx;
	int rc;

	/* Allocate and initialise structure */
	netdev = alloc_etherdev ( sizeof ( *smsc95xx ) );
	if ( ! netdev ) {
		rc = -ENOMEM;
		goto err_alloc;
	}
	netdev_init ( netdev, &smsc95xx_operations );
	netdev->dev = &func->dev;
	smsc95xx = netdev->priv;
	memset ( smsc95xx, 0, sizeof ( *smsc95xx ) );
	smsc95xx->usb = usb;
	smsc95xx->bus = usb->port->hub->bus;
	smsc95xx->netdev = netdev;
	usbnet_init ( &smsc95xx->usbnet, func, &smsc95xx_intr_operations,
		      &smsc95xx_in_operations, &smsc95xx_out_operations );
	usb_refill_init ( &smsc95xx->usbnet.intr, 0, 0,
			  SMSC95XX_INTR_MAX_FILL );
	usb_refill_init ( &smsc95xx->usbnet.in,
			  ( sizeof ( struct smsc95xx_tx_header ) -
			    sizeof ( struct smsc95xx_rx_header ) ),
			  SMSC95XX_IN_MTU, SMSC95XX_IN_MAX_FILL );
	mii_init ( &smsc95xx->mii, &smsc95xx_mii_operations );
	DBGC ( smsc95xx, "SMSC95XX %p on %s\n", smsc95xx, func->name );

	/* Describe USB network device */
	if ( ( rc = usbnet_describe ( &smsc95xx->usbnet, config ) ) != 0 ) {
		DBGC ( smsc95xx, "SMSC95XX %p could not describe: %s\n",
		       smsc95xx, strerror ( rc ) );
		goto err_describe;
	}

	/* Reset device */
	if ( ( rc = smsc95xx_reset ( smsc95xx ) ) != 0 )
		goto err_reset;

	/* Read MAC address */
	if ( ( rc = smsc95xx_fetch_mac ( smsc95xx, netdev->hw_addr ) ) != 0 )
		goto err_fetch_mac;

	/* Register network device */
	if ( ( rc = register_netdev ( netdev ) ) != 0 )
		goto err_register;

	usb_func_set_drvdata ( func, netdev );
	return 0;

	unregister_netdev ( netdev );
 err_register:
 err_fetch_mac:
 err_reset:
 err_describe:
	netdev_nullify ( netdev );
	netdev_put ( netdev );
 err_alloc:
	return rc;
}

/**
 * Remove device
 *
 * @v func		USB function
 */
static void smsc95xx_remove ( struct usb_function *func ) {
	struct net_device *netdev = usb_func_get_drvdata ( func );

	unregister_netdev ( netdev );
	netdev_nullify ( netdev );
	netdev_put ( netdev );
}

/** SMSC95xx device IDs */
static struct usb_device_id smsc95xx_ids[] = {
	{
		.name = "smsc9500",
		.vendor = 0x0424,
		.product = 0x9500,
	},
	{
		.name = "smsc9505",
		.vendor = 0x0424,
		.product = 0x9505,
	},
	{
		.name = "smsc9500a",
		.vendor = 0x0424,
		.product = 0x9e00,
	},
	{
		.name = "smsc9505a",
		.vendor = 0x0424,
		.product = 0x9e01,
	},
	{
		.name = "smsc9514",
		.vendor = 0x0424,
		.product = 0xec00,
	},
	{
		.name = "smsc9500-s",
		.vendor = 0x0424,
		.product = 0x9900,
	},
	{
		.name = "smsc9505-s",
		.vendor = 0x0424,
		.product = 0x9901,
	},
	{
		.name = "smsc9500a-s",
		.vendor = 0x0424,
		.product = 0x9902,
	},
	{
		.name = "smsc9505a-s",
		.vendor = 0x0424,
		.product = 0x9903,
	},
	{
		.name = "smsc9514-s",
		.vendor = 0x0424,
		.product = 0x9904,
	},
	{
		.name = "smsc9500a-h",
		.vendor = 0x0424,
		.product = 0x9905,
	},
	{
		.name = "smsc9505a-h",
		.vendor = 0x0424,
		.product = 0x9906,
	},
	{
		.name = "smsc9500-2",
		.vendor = 0x0424,
		.product = 0x9907,
	},
	{
		.name = "smsc9500a-2",
		.vendor = 0x0424,
		.product = 0x9908,
	},
	{
		.name = "smsc9514-2",
		.vendor = 0x0424,
		.product = 0x9909,
	},
	{
		.name = "smsc9530",
		.vendor = 0x0424,
		.product = 0x9530,
	},
	{
		.name = "smsc9730",
		.vendor = 0x0424,
		.product = 0x9730,
	},
	{
		.name = "smsc89530",
		.vendor = 0x0424,
		.product = 0x9e08,
	},
};

/** SMSC LAN95xx driver */
struct usb_driver smsc95xx_driver __usb_driver = {
	.ids = smsc95xx_ids,
	.id_count = ( sizeof ( smsc95xx_ids ) / sizeof ( smsc95xx_ids[0] ) ),
	.class = USB_CLASS_ID ( 0xff, 0x00, 0xff ),
	.score = USB_SCORE_NORMAL,
	.probe = smsc95xx_probe,
	.remove = smsc95xx_remove,
};

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <stdint.h>
#include <ipxe/pci.h>

static int pci_find_capability_common ( struct pci_device *pci,
					uint8_t pos, int cap ) {
	uint8_t id;
	int ttl = 48;

	while ( ttl-- && pos >= 0x40 ) {
		pos &= ~3;
		pci_read_config_byte ( pci, pos + PCI_CAP_ID, &id );
		DBG ( "PCI Capability: %d\n", id );
		if ( id == 0xff )
			break;
		if ( id == cap )
			return pos;
		pci_read_config_byte ( pci, pos + PCI_CAP_NEXT, &pos );
	}
	return 0;
}

/**
 * Look for a PCI capability
 *
 * @v pci		PCI device to query
 * @v cap		Capability code
 * @ret address		Address of capability, or 0 if not found
 *
 * Determine whether or not a device supports a given PCI capability.
 * Returns the address of the requested capability structure within
 * the device's PCI configuration space, or 0 if the device does not
 * support it.
 */
int pci_find_capability ( struct pci_device *pci, int cap ) {
	uint16_t status;
	uint8_t pos;
	uint8_t hdr_type;

	pci_read_config_word ( pci, PCI_STATUS, &status );
	if ( ! ( status & PCI_STATUS_CAP_LIST ) )
		return 0;

	pci_read_config_byte ( pci, PCI_HEADER_TYPE, &hdr_type );
	switch ( hdr_type & PCI_HEADER_TYPE_MASK ) {
	case PCI_HEADER_TYPE_NORMAL:
	case PCI_HEADER_TYPE_BRIDGE:
	default:
		pci_read_config_byte ( pci, PCI_CAPABILITY_LIST, &pos );
		break;
	case PCI_HEADER_TYPE_CARDBUS:
		pci_read_config_byte ( pci, PCI_CB_CAPABILITY_LIST, &pos );
		break;
	}
	return pci_find_capability_common ( pci, pos, cap );
}

/**
 * Look for another PCI capability
 *
 * @v pci		PCI device to query
 * @v pos		Address of the current capability
 * @v cap		Capability code
 * @ret address		Address of capability, or 0 if not found
 *
 * Determine whether or not a device supports a given PCI capability
 * starting the search at a given address within the device's PCI
 * configuration space. Returns the address of the next capability
 * structure within the device's PCI configuration space, or 0 if the
 * device does not support another such capability.
 */
int pci_find_next_capability ( struct pci_device *pci, int pos, int cap ) {
	uint8_t new_pos;

	pci_read_config_byte ( pci, pos + PCI_CAP_NEXT, &new_pos );
	return pci_find_capability_common ( pci, new_pos, cap );
}

/**
 * Find the size of a PCI BAR
 *
 * @v pci		PCI device
 * @v reg		PCI register number
 * @ret size		BAR size
 *
 * It should not be necessary for any Etherboot code to call this
 * function.
 */
unsigned long pci_bar_size ( struct pci_device *pci, unsigned int reg ) {
	uint16_t cmd;
	uint32_t start, size;

	/* Save the original command register */
	pci_read_config_word ( pci, PCI_COMMAND, &cmd );
	/* Save the original bar */
	pci_read_config_dword ( pci, reg, &start );
	/* Compute which bits can be set */
	pci_write_config_dword ( pci, reg, ~0 );
	pci_read_config_dword ( pci, reg, &size );
	/* Restore the original size */
	pci_write_config_dword ( pci, reg, start );
	/* Find the significant bits */
	/* Restore the original command register. This reenables decoding. */
	pci_write_config_word ( pci, PCI_COMMAND, cmd );
	if ( start & PCI_BASE_ADDRESS_SPACE_IO ) {
		size &= ~PCI_BASE_ADDRESS_IO_MASK;
	} else {
		size &= ~PCI_BASE_ADDRESS_MEM_MASK;
	}
	/* Find the lowest bit set */
	size = size & ~( size - 1 );
	return size;
}

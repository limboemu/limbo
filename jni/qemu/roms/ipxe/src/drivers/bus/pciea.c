/*
 * Copyright (C) 2016 Michael Brown <mbrown@fensystems.co.uk>.
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

#include <stdint.h>
#include <errno.h>
#include <ipxe/pci.h>
#include <ipxe/pciea.h>

/** @file
 *
 * PCI Enhanced Allocation
 *
 */

/**
 * Locate PCI Enhanced Allocation BAR equivalent entry
 *
 * @v pci		PCI device
 * @v bei		BAR equivalent indicator
 * @ret offset		PCI Enhanced Allocation entry offset, or negative error
 */
static int pciea_offset ( struct pci_device *pci, unsigned int bei ) {
	uint8_t entries;
	uint32_t desc;
	unsigned int i;
	int offset;

	/* Locate Enhanced Allocation capability */
	offset = pci_find_capability ( pci, PCI_CAP_ID_EA );
	if ( offset < 0 )
		return offset;

	/* Get number of entries */
	pci_read_config_byte ( pci, ( offset + PCIEA_ENTRIES ), &entries );
	entries &= PCIEA_ENTRIES_MASK;

	/* Locate first entry */
	offset += PCIEA_FIRST;

	/* Search for a matching entry */
	for ( i = 0 ; i < entries ; i++ ) {

		/* Read entry descriptor */
		pci_read_config_dword ( pci, offset, &desc );

		/* Check for a matching entry */
		if ( ( desc & PCIEA_DESC_ENABLED ) &&
		     ( bei == PCIEA_DESC_BEI ( desc ) ) )
			return offset;

		/* Move to next entry */
		offset += ( ( PCIEA_DESC_SIZE ( desc ) + 1 ) << 2 );
	}

	return -ENOENT;
}

/**
 * Read PCI Enhanced Allocation BAR equivalent value
 *
 * @v pci		PCI device
 * @v bei		BAR equivalent indicator
 * @v low_offset	Offset to low dword of value
 * @ret value		BAR equivalent value
 */
static unsigned long pciea_bar_value ( struct pci_device *pci, unsigned int bei,
				       unsigned int low_offset ) {
	uint32_t low;
	uint32_t high;
	int offset;

	/* Locate Enhanced Allocation offset for this BEI */
	offset = pciea_offset ( pci, bei );
	if ( offset < 0 )
		return 0;

	/* Read BAR equivalent */
	offset += low_offset;
	pci_read_config_dword ( pci, offset, &low );
	if ( low & PCIEA_LOW_ATTR_64BIT ) {
		offset += PCIEA_LOW_HIGH;
		pci_read_config_dword ( pci, offset, &high );
		if ( high ) {
			if ( sizeof ( unsigned long ) > sizeof ( uint32_t ) ) {
				return ( ( ( uint64_t ) high << 32 ) | low );
			} else {
				DBGC ( pci, PCI_FMT " unhandled 64-bit EA BAR "
				       "%08x%08x\n",
				       PCI_ARGS ( pci ), high, low );
				return 0;
			}
		}
	}
	return low;
}

/**
 * Find the start of a PCI Enhanced Allocation BAR equivalent
 *
 * @v pci		PCI device
 * @v bei		BAR equivalent indicator
 * @ret start		BAR start address
 *
 * If the address exceeds the size of an unsigned long (i.e. if a
 * 64-bit BAR has a non-zero high dword on a 32-bit machine), the
 * return value will be zero.
 */
unsigned long pciea_bar_start ( struct pci_device *pci, unsigned int bei ) {
	unsigned long base;

	base = pciea_bar_value ( pci, bei, PCIEA_LOW_BASE );
	return ( base & ~PCIEA_LOW_ATTR_MASK );
}

/**
 * Find the size of a PCI Enhanced Allocation BAR equivalent
 *
 * @v pci		PCI device
 * @v bei		BAR equivalent indicator
 * @ret size		BAR size
 */
unsigned long pciea_bar_size ( struct pci_device *pci, unsigned int bei ) {
	unsigned long limit;

	limit = pciea_bar_value ( pci, bei, PCIEA_LOW_LIMIT );
	return ( limit ? ( ( limit | PCIEA_LOW_ATTR_MASK ) + 1 ) : 0 );
}

#ifndef _IPXE_PCIEA_H
#define _IPXE_PCIEA_H

/** @file
 *
 * PCI Enhanced Allocation
 *
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <ipxe/pci.h>

/** Number of entries */
#define PCIEA_ENTRIES 2
#define PCIEA_ENTRIES_MASK 0x3f

/** First entry */
#define PCIEA_FIRST 4

/** Entry descriptor */
#define PCIEA_DESC 0

/** Entry size */
#define PCIEA_DESC_SIZE(desc) ( ( (desc) >> 0 ) & 0x7 )

/** BAR equivalent indicator */
#define PCIEA_DESC_BEI(desc) ( ( (desc) >> 4 ) & 0xf )

/** BAR equivalent indicators */
enum pciea_bei {
	PCIEA_BEI_BAR_0 = 0,		/**< Standard BAR 0 */
	PCIEA_BEI_BAR_1 = 1,		/**< Standard BAR 1 */
	PCIEA_BEI_BAR_2 = 2,		/**< Standard BAR 2 */
	PCIEA_BEI_BAR_3 = 3,		/**< Standard BAR 3 */
	PCIEA_BEI_BAR_4 = 4,		/**< Standard BAR 4 */
	PCIEA_BEI_BAR_5 = 5,		/**< Standard BAR 5 */
	PCIEA_BEI_ROM = 8,		/**< Expansion ROM BAR */
	PCIEA_BEI_VF_BAR_0 = 9,		/**< Virtual function BAR 0 */
	PCIEA_BEI_VF_BAR_1 = 10,	/**< Virtual function BAR 1 */
	PCIEA_BEI_VF_BAR_2 = 11,	/**< Virtual function BAR 2 */
	PCIEA_BEI_VF_BAR_3 = 12,	/**< Virtual function BAR 3 */
	PCIEA_BEI_VF_BAR_4 = 13,	/**< Virtual function BAR 4 */
	PCIEA_BEI_VF_BAR_5 = 14,	/**< Virtual function BAR 5 */
};

/** Entry is enabled */
#define PCIEA_DESC_ENABLED 0x80000000UL

/** Base address low dword */
#define PCIEA_LOW_BASE 4

/** Limit low dword */
#define PCIEA_LOW_LIMIT 8

/** BAR is 64-bit */
#define PCIEA_LOW_ATTR_64BIT 0x00000002UL

/** Low dword attribute bit mask */
#define PCIEA_LOW_ATTR_MASK 0x00000003UL

/** Offset to high dwords */
#define PCIEA_LOW_HIGH 8

extern unsigned long pciea_bar_start ( struct pci_device *pci,
				       unsigned int bei );
extern unsigned long pciea_bar_size ( struct pci_device *pci,
				      unsigned int bei );

#endif /* _IPXE_PCIEA_H */

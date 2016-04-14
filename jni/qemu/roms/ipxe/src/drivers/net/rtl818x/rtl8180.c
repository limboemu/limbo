/* Realtek 8180 card: rtl818x driver + rtl8180 RF modules */

FILE_LICENCE(GPL2_OR_LATER);

#include <ipxe/pci.h>

REQUIRE_OBJECT(rtl818x);
REQUIRE_OBJECT(rtl8180_grf5101);
REQUIRE_OBJECT(rtl8180_max2820);
REQUIRE_OBJECT(rtl8180_sa2400);

static struct pci_device_id rtl8180_nics[] __unused = {
	PCI_ROM(0x10ec, 0x8180, "rtl8180", "Realtek 8180", 0),
	PCI_ROM(0x1799, 0x6001, "f5d6001", "Belkin F5D6001", 0),
	PCI_ROM(0x1799, 0x6020, "f5d6020", "Belkin F5D6020", 0),
	PCI_ROM(0x1186, 0x3300, "dwl510",  "D-Link DWL-510", 0),
};

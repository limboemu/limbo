/* Realtek 8185 card: rtl818x driver + rtl8185_rtl8225 RF module */

FILE_LICENCE(GPL2_OR_LATER);

#include <ipxe/pci.h>

REQUIRE_OBJECT(rtl818x);
REQUIRE_OBJECT(rtl8185_rtl8225);

static struct pci_device_id rtl8185_nics[] __unused = {
	PCI_ROM(0x10ec, 0x8185, "rtl8185", "Realtek 8185", 0),
	PCI_ROM(0x1799, 0x700f, "f5d7000", "Belkin F5D7000", 0),
	PCI_ROM(0x1799, 0x701f, "f5d7010", "Belkin F5D7010", 0),
};

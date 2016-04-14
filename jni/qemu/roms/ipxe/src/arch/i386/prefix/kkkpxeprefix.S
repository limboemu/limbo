/*****************************************************************************
 * PXE prefix that keeps the whole PXE stack present and provides an exit hook
 *
 * This prefix is essentially intended solely for the case of ipxelinux.0
 *****************************************************************************
 */

FILE_LICENCE ( GPL2_OR_LATER )

/* Since we have the whole stack, we can use cached DHCP information */
REQUIRE_OBJECT ( pxeparent_dhcp )

/* Provide the PXENV_FILE_EXIT_HOOK API call */
REQUIRE_OBJECT ( pxe_exit_hook )

#define PXELOADER_KEEP_UNDI
#define PXELOADER_KEEP_PXE
#define _pxe_start _kkkpxe_start
#include "pxeprefix.S"
